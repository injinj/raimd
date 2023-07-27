#include <stdio.h>
#include <ctype.h>
#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <raimd/md_dict.h>
#include <raimd/hash32.h>

using namespace rai;
using namespace md;

/* crc_c */
uint32_t
MDDict::dict_hash( const char *key,  size_t len ) noexcept
{
  if ( len > 0 && key[ len - 1 ] == '\0' )
    len--;
  return hash32( key, len, 0 );
}

bool
MDDict::dict_equals( const char *fname,  size_t len,
                     const char *fname2,  size_t len2 ) noexcept
{
  /* allow fnames to include nul terminating char */
  if ( len > 0 && fname[ len - 1 ] == '\0' )
    len--;
  if ( len2 > 0 && fname2[ len2 - 1 ] == '\0' )
    len2--;
  return len == len2 && ::memcmp( fname, fname2, len ) == 0;
}

uint32_t
MDDictIdx::file_lineno( const char *filename,  uint32_t lineno ) noexcept
{
  MDFilename * fn;
  if ( this->file_q.is_empty() ||
       ::strcmp( filename, this->file_q.hd->filename ) != 0 ) {
    size_t fn_len = ::strlen( filename ),
           len    = sizeof( MDFilename ) - sizeof( fn->filename ) + fn_len + 1;
    fn = this->alloc<MDFilename>( len );
    fn->id = ( this->file_q.hd == NULL ? 0 : this->file_q.hd->id + 1 );
    ::memcpy( fn->filename, filename, fn_len + 1 );
    this->file_q.push_hd( fn );
  }
  return ( this->file_q.hd->id << 24 ) | lineno;
}

size_t
MDDictIdx::total_size( void ) noexcept
{
  size_t sz = 0;
  for ( MDEntryData *d = this->data_q.hd; d != NULL; d = d->next )
    sz += d->off;
  return sz;
}

template< class TYPE >
TYPE *
MDDictIdx::alloc( size_t sz ) noexcept
{
  void * ptr;
  if ( this->data_q.is_empty() ||
       (ptr = this->data_q.hd->alloc( sz )) == NULL ) {
    ptr = ::malloc( sizeof( MDEntryData ) );
    if ( ptr == NULL )
      return NULL;
    this->data_q.push_hd( new ( ptr ) MDEntryData() );
    ptr = this->data_q.hd->alloc( sz );
  }
  return new ( ptr ) TYPE();
}

MDDictIdx::~MDDictIdx() noexcept
{
  MDEntryData *p;
  while ( (p = this->data_q.pop()) != NULL )
    ::free( p );
  this->entry_q.init();
  this->file_q.init();
  this->type_hash = NULL;
  if ( this->fid_index != NULL )
    ::free( this->fid_index );
  this->fid_index = NULL;
  this->fid_index_size = 0;
  if ( this->fname_index != NULL )
    ::free( this->fname_index );
  this->fname_index = NULL;
  this->fname_index_size = 0;
}

static inline uint64_t rotl( uint64_t x, int r ) {
  return ((x << r) | (x >> (64 - r)));
}
static inline uint64_t hash_int( uint64_t x ) {
  return ( rotl( x, 41 ) * (uint64_t) 0x5bd1e995 ) ^ x ^
         ( rotl( x, 9 ) * (uint64_t) 0x97cb3127 );
}

void
MDDictIdx::type_hash_insert( MDDictEntry &entry ) noexcept
{
  MDTypeHash * ht = this->type_hash;
  uint32_t i,
           htsize = ( ht != NULL ? ht->htsize() : 0 ),
           htmask = htsize - 1;

  if ( ht == NULL || ht->htcnt >= htmask ) {
    MDTypeHash * old = ht;
    uint32_t htshft = ( old != NULL ? old->htshft + 1 : 6 ),
             htcnt  = ( old != NULL ? old->htcnt : 0 );
    ht = this->alloc<MDTypeHash>( MDTypeHash::alloc_size( htshft ) );
    ht->init( htshft );
    htsize = ht->htsize();
    htmask = htsize - 1;
    if ( htcnt != 0 ) {
      uint32_t old_htsize = old->htsize();
      for ( uint32_t z = 0; z < old_htsize; z++ ) {
        if ( old->ht[ z ] != 0 ) {
          i = hash_int( old->ht[ z ] ) & htmask;
          for ( ; ; i = ( i + 1 ) & htmask ) {
            if ( ht->ht[ i ] == 0 )
              break;
          }
          ht->ht[ i ] = old->ht[ z ];
        }
      }
      ht->htcnt = htcnt;
    }
    this->type_hash = ht;
  }

  uint64_t x = md_dict_hash_compose( entry );
  i = hash_int( x ) & htmask;

  for ( ; ; i = ( i + 1 ) & htmask ) {
    if ( ht->ht[ i ] == 0 || ht->ht[ i ] == x )
      break;
  }
  if ( ht->ht[ i ] == 0 ) {
    entry.tt_pos = i;
    ht->ht[ i ] = x;
    ht->htcnt++;
  }
}

uint32_t
MDDictIdx::type_hash_find( MDDictEntry &entry ) noexcept
{
  MDTypeHash * ht = this->type_hash;
  uint64_t x      = md_dict_hash_compose( entry );
  uint32_t htmask = ht->htsize() - 1,
           i      = hash_int( x ) & htmask;
  for ( ; ; i = ( i + 1 ) & htmask ) {
    if ( ht->ht[ i ] == x )
      break;
  }
  entry.tt_pos = i;
  return i;
}


MDDictIdx *
MDDictBuild::get_dict_idx( void ) noexcept
{
  MDDictIdx * dict = this->idx;
  if ( dict == NULL ) {
    void * ptr = ::malloc( sizeof( MDDictIdx ) );
    if ( ptr == NULL )
      return NULL;
    dict = new ( ptr ) MDDictIdx();
    this->idx = dict;
  }
  return dict;
}

int
MDDictBuild::add_enum_map( MDEnumAdd &a ) noexcept
{
  MDEnumList * emap;
  uint16_t   * value_cp;
  uint8_t    * map_cp;
  size_t       map_sz = MDEnumMap::map_sz( a.value_cnt, a.max_value, a.max_len),
               sz     = sizeof( MDEnumList ) + map_sz;
  MDDictIdx * dict = this->get_dict_idx();
  if ( dict == NULL )
    return Err::ALLOC_FAIL;
  emap = dict->alloc<MDEnumList>( sz );
  emap->map.map_num   = a.map_num;
  emap->map.max_value = a.max_value;
  emap->map.value_cnt = a.value_cnt;
  emap->map.max_len   = a.max_len;
  value_cp = emap->map.value();
  map_cp   = emap->map.map();
  if ( value_cp != NULL )
    ::memcpy( value_cp, a.value, sizeof( uint16_t ) * a.value_cnt );
  ::memcpy( map_cp, a.map, (size_t) a.max_len * (size_t) a.value_cnt );
  dict->enum_q.push_tl( emap );
  if ( a.map_num >= dict->map_cnt )
    dict->map_cnt = a.map_num + 1;
  dict->map_size += sizeof( MDEnumMap ) + map_sz;

  return 0;
}

MDDictEntry *
MDDictIdx::get_fid_entry( MDFid fid ) noexcept
{
  if ( fid < this->min_fid || fid > this->max_fid )
    return NULL;
  size_t off = (size_t) ( fid - this->min_fid );
  if ( off >= this->fid_index_size || this->fid_index[ off ] == NULL ) {
    this->fid_index_size = ( this->max_fid - this->min_fid ) + 1;
    size_t sz = sizeof( this->fid_index[ 0 ] ) * this->fid_index_size;
    this->fid_index = (MDDictEntry **) ::realloc( this->fid_index, sz );
    ::memset( this->fid_index, 0, sz );
    for ( MDDictEntry *entry = this->entry_q.hd; entry != NULL;
          entry = entry->next ) {
      size_t i = (size_t) ( entry->fid - this->min_fid );
      this->fid_index[ i ] = entry;
    }
  }
  return this->fid_index[ off ];
}

MDDictEntry *
MDDictIdx::get_fname_entry( const char *fname,  size_t fnamelen,
                            uint32_t h ) noexcept
{
  if ( this->fname_index_size == 0 )
    return NULL;
  size_t off = h & ( this->fname_index_size - 1 );
  for ( ; this->fname_index[ off ] != NULL;
        off = ( off + 1 ) & ( this->fname_index_size - 1 ) ) {
    if ( this->fname_index[ off ]->hash == h &&
         MDDict::dict_equals( fname, fnamelen,
                              this->fname_index[ off ]->fname(),
                              this->fname_index[ off ]->fnamelen ) )
      break;
  }
  return this->fname_index[ off ];
}

void
MDDictIdx::add_fname_entry( MDDictEntry *entry ) noexcept
{
  size_t off;
  if ( this->fname_index_size <= this->entry_count * 2 ) {
    if ( (this->fname_index_size = this->entry_count * 4) < 1024 )
      this->fname_index_size = 1024;

    size_t sz = sizeof( this->fname_index[ 0 ] ) * this->fname_index_size;
    this->fname_index = (MDDictEntry **) ::realloc( this->fname_index, sz );
    ::memset( this->fname_index, 0, sz );
    for ( MDDictEntry *ex = this->entry_q.hd; ex != NULL; ex = ex->next ) {
      off = ex->hash & ( this->fname_index_size - 1 );
      for ( ; this->fname_index[ off ] != NULL;
            off = ( off + 1 ) & ( this->fname_index_size - 1 ) )
        ;
      this->fname_index[ off ] = ex;
    }
  }
  off = entry->hash & ( this->fname_index_size - 1 );
  for ( ; this->fname_index[ off ] != NULL;
        off = ( off + 1 ) & ( this->fname_index_size - 1 ) )
    ;
  this->fname_index[ off ] = entry;
  this->entry_q.push_tl( entry );
  this->entry_count++;
}

int
MDDictBuild::update_entry_enum( MDFid fid,  uint32_t map_num,
                                uint16_t enum_len ) noexcept
{
  MDDictIdx   * dict = this->get_dict_idx();
  MDDictEntry * entry;
  if ( dict == NULL )
    return Err::ALLOC_FAIL;
  if ( (entry = dict->get_fid_entry( fid )) == NULL ) {
    MDPendingEnum * p = dict->alloc<MDPendingEnum>( sizeof( MDPendingEnum ) );
    p->fid      = fid;
    p->map_num  = map_num;
    p->enum_len = enum_len;

    MDPendingEnum * el = dict->pending_enum_q.tl;
    if ( el == NULL || fid >= el->fid ) {
      if ( el == NULL || fid != el->fid )
        dict->pending_enum_q.push_tl( p );
    }
    else {
      el = dict->pending_enum_q.hd;
      if ( fid <= el->fid ) {
        if ( fid != el->fid )
          dict->pending_enum_q.push_hd( p );
      }
      else {
        while ( fid > el->next->fid )
          el = el->next;
        if ( fid != el->next->fid ) {
          p->next = el->next;
          el->next = p;
        }
      }
    }
  }
  else {
    if ( entry->ftype != MD_ENUM )
      return Err::NO_ENUM;
    entry->map_num = map_num;
    if ( enum_len != 0 )
      entry->enum_len = enum_len;
  }
  return 0;
}

void
MDDictEntry::update( MDDictAdd &a,  uint32_t h ) noexcept
{
  this->fid        = a.fid;
  this->fsize      = a.fsize;
  this->ftype      = a.ftype;
  this->fld_flags |= a.flags;
  if ( a.mf_len != 0 )
    this->mf_len = a.mf_len;
  if ( a.rwf_len != 0 )
    this->rwf_len = a.rwf_len;
  if ( a.enum_len != 0 )
    this->enum_len = a.enum_len;
  this->mf_type    = a.mf_type;
  this->rwf_type   = a.rwf_type;
  this->hash       = h;
  if ( a.name != NULL )
    this->fld_flags |= MD_HAS_NAME;
  if ( a.ripple != NULL )
    this->fld_flags |= MD_HAS_RIPPLE;
}

int
MDDictBuild::add_entry( MDDictAdd &a ) noexcept
{
  MDDictIdx   * dict = this->get_dict_idx();
  MDDictEntry * entry;
  size_t        fnamelen,
                namelen,
                ripplelen;
  char        * ptr;
  uint32_t      h;
  bool          xdup = false;

  if ( dict == NULL )
    return Err::ALLOC_FAIL;
  size_t sz = sizeof( MDDictEntry ) - sizeof( entry->buf ) +
              ( fnamelen  = ( a.fname  ? ::strlen( a.fname )  + 1 : 0 ) ) +
              ( namelen   = ( a.name   ? ::strlen( a.name )   + 1 : 0 ) ) +
              ( ripplelen = ( a.ripple ? ::strlen( a.ripple ) + 1 : 0 ) );

  if ( (fnamelen | namelen | ripplelen) > 0xff )
    return Err::BAD_NAME;

  h = MDDict::dict_hash( a.fname, fnamelen );
  entry = dict->get_fname_entry( a.fname, fnamelen, h );
  if ( entry != NULL ) {
    if ( a.fid == entry->fid && a.ftype == entry->ftype &&
         a.fsize == entry->fsize ) {
      entry->update( a, h );
      a.entry = entry;
      return 0; /* already declared */
    }
    /* if change in size of type with same fid, print error */
    if ( a.fid == entry->fid ) {
      if ( a.fsize != entry->fsize || a.ftype != entry->ftype ) {
        MDFilename *xfn = dict->file_q.hd;
        while ( xfn != NULL && xfn->id != entry->fno )
          xfn = xfn->next;
        if ( xfn != NULL ) {
          fprintf( stderr,
                   "\"%s\":%u and \"%s\":%u "
                   "redeclared: %s (fid %u/%u) fsize (%u/%u) ftype(%u/%u)\n",
                  a.filename, a.lineno, xfn->filename, xfn->id & 0xffffff,
                  a.fname, a.fid, entry->fid, a.fsize, entry->fsize, a.ftype,
                  entry->ftype );
        }
      }
      /* another declaration with same name */
    }
    /* same decl, different fid (fine) */
    xdup = true;
  }
  entry = dict->alloc<MDDictEntry>( sz );
  if ( entry == NULL )
    return Err::ALLOC_FAIL;

  entry->update( a, h );
  if ( ! xdup )
    entry->fno = dict->file_lineno( a.filename, a.lineno );
  else
    entry->fno = 0;

  ptr = entry->buf;
  if ( (entry->fnamelen = (uint8_t) fnamelen) > 0 ) {
    ::memcpy( ptr, a.fname, fnamelen );
    ptr = &ptr[ fnamelen ];
  }
  if ( (entry->namelen = (uint8_t) namelen) > 0 ) {
    ::memcpy( ptr, a.name, namelen );
    ptr = &ptr[ namelen ];
  }
  if ( (entry->ripplelen = (uint8_t) ripplelen) > 0 ) {
    ::memcpy( ptr, a.ripple, ripplelen );
    ptr = &ptr[ ripplelen ];
  }
  if ( entry->ftype == MD_ENUM ) {
    MDPendingEnum * last = NULL,
                  * el = dict->pending_enum_q.hd;
    while ( el != NULL ) {
      if ( entry->fid >= el->fid )
        break;
      last = el;
      el = el->next;
    }
    if ( el != NULL && el->fid == entry->fid ) {
      entry->map_num = el->map_num;
      if ( el->enum_len != 0 )
        entry->enum_len = el->enum_len;
      if ( last == NULL )
        dict->pending_enum_q.pop();
      else if ( (last->next = el->next) == NULL )
        dict->pending_enum_q.tl = last;
    }
  }
  dict->add_fname_entry( entry );

  a.entry = entry;
  if ( dict->entry_count == 1 || a.fid > dict->max_fid )
    dict->max_fid = a.fid;
  if ( dict->entry_count == 1 || a.fid < dict->min_fid )
    dict->min_fid = a.fid;

  return 0;
}

void
MDDictBuild::add_tag( const char *tag,  uint32_t tag_sz ) noexcept
{
  MDDictIdx * dict = this->get_dict_idx();
  MDTag     * t    = dict->alloc<MDTag>( sizeof( MDTag ) + tag_sz );
  char      * p    = t->val;
  uint32_t    i, nm_sz = 0, val_sz = 0;

  for ( i = 0; i < tag_sz; ) {
    *p = tag[ i++ ];
    if ( isspace( *p++ ) )
      break;
    nm_sz++;
  }
  for ( ; i < tag_sz; i++ ) {
    if ( ! isspace( tag[ i ] ) )
      break;
  }
  val_sz = tag_sz - i;
  if ( nm_sz == 0 || val_sz == 0 )
    return;
  ::memcpy( p, &tag[ i ], val_sz );
  p = &p[ val_sz ];
  t->len = p - t->val;
  if ( t->len > 0xff )
    return;
  dict->tag_q.push_tl( t );
}

void
MDDictBuild::add_tag( const char *name,  uint32_t name_sz,
                      const char *val,  uint32_t val_sz ) noexcept
{
  MDDictIdx * dict   = this->get_dict_idx();
  size_t      tag_sz = name_sz + val_sz + 1;
  MDTag     * t      = dict->alloc<MDTag>( sizeof( MDTag ) + tag_sz );
  char      * p      = t->val;

  if ( tag_sz > 0xff || name_sz == 0 || val_sz == 0 )
    return;
  ::memcpy( p, name, name_sz );
  p = &p[ name_sz ];
  *p++ = ' ';
  ::memcpy( p, val, val_sz );
  p = &p[ val_sz ];

  t->len = p - t->val;
  dict->tag_q.push_tl( t );
}

static size_t
mkpow2( size_t i ) noexcept
{
  for ( size_t n = i >> 1; ; n >>= 1 ) {
    i |= ( n | 1 );
    if ( ( ( i + 1 ) & i ) == 0 )
      return i + 1;
  }
}

static size_t
mkshft( size_t sz ) noexcept
{
  size_t n = 1;
  for ( ; ; n++ ) {
    size_t sz_pow2 = (size_t) 1 << n;
    if ( sz_pow2 >= sz )
      break;
  }
  return n;
}

int
MDDictBuild::index_dict( const char *dtype,  MDDict *&dict ) noexcept
{
  MDDict * next = dict;
  MDDictIdx * dict_idx = this->idx;
  if ( dict_idx == NULL )
    return Err::NO_DICTIONARY;

  size_t fnamesz = 4;
  for ( MDDictEntry *d = dict_idx->entry_q.hd; d != NULL; d = d->next ) {
    size_t n = ( d->fnamelen + 1 );
    if ( d->namelen != 0 )
      n += d->namelen + 1;
    if ( d->ripplelen != 0 )
      n += d->ripplelen + 1;
    fnamesz += ( n + 3 ) & ~3;
  }
  uint8_t fn_shft = mkshft( fnamesz ),
          fn_algn = 2,
          fn_bits = fn_shft - fn_algn,
          tab_bits,
          ttshft;

  MDDictEntry * fp;
  size_t  sz      = sizeof( MDDict ),
          nfids   = ( dict_idx->max_fid - dict_idx->min_fid ) + 1,
          httabsz = mkpow2( dict_idx->entry_count * 2 ),
          fidbits,
          tabsz,
          typetabsz,
          map_off,
          tag_off = 0,
          ntags   = 0,
          tagsz   = 1;
  MDTag * t;
  for ( t = dict_idx->tag_q.hd; t != NULL; t = t->next ) {
    tagsz += ( t->len & 0xff ) + 1;
    ntags++;
  }

  for ( fp = dict_idx->entry_q.hd; fp != NULL; fp = fp->next )
    dict_idx->type_hash_insert( *fp );

  for ( fidbits = 2; ( (size_t) 1U << fidbits ) < ( nfids + 1 ); fidbits++ )
    ;
  ttshft    = dict_idx->type_hash->htshft;
  tab_bits  = ttshft + fn_bits;
  tabsz     = ( (size_t) tab_bits * nfids + 7 ) / 8; /* fid index */
  typetabsz = (size_t) dict_idx->type_hash->htsize() * sizeof( uint64_t );
  sz       += typetabsz;  /* type table size */
  sz       += tabsz;   /* fid table size */
  sz       += fnamesz; /* fname buffers size */
  sz       += ( ( httabsz * fidbits ) + 7 ) / 8; /* fname hash table size */
  sz        = ( sz + 3U ) & ~3U;  /* align int32 */
  map_off   = sz;      /* enum map offset */
  sz       += dict_idx->map_cnt * sizeof( uint32_t ) +
              dict_idx->map_size; /* enum map size */
  if ( ntags > 0 ) {
    tag_off = sz;
    sz     += tagsz;
  }
  sz       += 4;       /* end marker */
  void * ptr = ::malloc( sz );
  if ( ptr == NULL )
    return Err::ALLOC_FAIL;
  ::memset( ptr, 0, sz );
  dict   = (MDDict *) ptr;
  /* label the dict kind: cfile or app_a */
  size_t dlen = ::strlen( dtype );
  if ( dlen > sizeof( dict->dict_type ) - 1 )
    dlen = sizeof( dict->dict_type ) - 1;
  ::memcpy( dict->dict_type, dtype, dlen );
  dict->dict_type[ dlen ] = '\0';
  dict->next        = next;
  dict->min_fid     = dict_idx->min_fid;
  dict->max_fid     = dict_idx->max_fid;
  dict->type_shft   = dict_idx->type_hash->htshft;
  dict->fname_shft  = fn_shft;
  dict->fname_algn  = fn_algn;
  dict->tab_bits    = dict->type_shft + fn_bits;
  dict->tab_off     = (uint32_t) ( typetabsz + sizeof( dict[ 0 ] ) );
  dict->entry_count = (uint32_t) dict_idx->entry_count;
  dict->fname_off   = (uint32_t) ( dict->tab_off + tabsz );
  dict->tab_size    = (uint32_t) tabsz;
  dict->ht_off      = (uint32_t) ( dict->fname_off + fnamesz );
  dict->ht_size     = (uint32_t) httabsz;
  dict->map_off     = (uint32_t) map_off;
  dict->map_count   = (uint32_t) dict_idx->map_cnt;
  dict->fid_bits    = (uint8_t) fidbits;
  dict->tag_off     = (uint32_t) tag_off;
  dict->dict_size   = (uint32_t) sz; /* write( fd, dict, sz ) does save/replicate dict */

  uint64_t * typetab   = (uint64_t *) &((uint8_t *) ptr)[ sizeof( dict[ 0 ] ) ];
  uint32_t * maptab    = (uint32_t *) &((uint8_t *) ptr)[ map_off ];
  uint8_t  * tab       = (uint8_t *) &typetab[ dict_idx->type_hash->htsize() ],
           * fntab     = &tab[ tabsz ],
           * httab     = &fntab[ fnamesz ],
           * mapdata   = (uint8_t *) (void *) &maptab[ dict_idx->map_cnt ];
  uint32_t   fname_off    = 0,
             fn_algn_mask = ( 1U << fn_algn ) - 1,
             fid_mask     = ( 1U << fidbits ) - 1,
             fid_off,
             j, h;
  uint64_t   val;

  while ( fname_off < 4 ) /* pad head */
    fntab[ fname_off++ ] = 0;
  ::memcpy( typetab, dict_idx->type_hash->ht, typetabsz );
  for ( MDDictEntry *fp = dict_idx->entry_q.hd; fp != NULL; fp = fp->next ) {
    uint32_t fname_start = fname_off;
    fntab[ fname_off++ ] = fp->fnamelen;
    ::memcpy( &fntab[ fname_off ], fp->fname(), fp->fnamelen );
    fname_off += fp->fnamelen;

    if ( fp->namelen != 0 ) {
      fntab[ fname_off++ ] = fp->namelen;
      ::memcpy( &fntab[ fname_off ], fp->name(), fp->namelen );
      fname_off += fp->namelen;
    }
    if ( fp->ripplelen != 0 ) {
      fntab[ fname_off++ ] = fp->ripplelen;
      ::memcpy( &fntab[ fname_off ], fp->ripple(), fp->ripplelen );
      fname_off += fp->ripplelen;
    }
    while ( ( fname_off & fn_algn_mask ) != 0 )
      fntab[ fname_off++ ] = 0;

    val     = (uint64_t) dict_idx->type_hash_find( *fp ) << fn_bits;
    val    |= ( fname_start >> fn_algn );

    fid_off = (uint32_t) tab_bits * ( fp->fid - dict_idx->min_fid );
    val   <<= ( fid_off % 8 );

    for ( j = fid_off / 8; val != 0; j++ ) {
      tab[ j ] |= ( val & 0xffU );
      val >>= 8;
    }

    h = fp->hash & ( httabsz - 1 );
    for ( ; ; h = ( h + 1 ) & ( httabsz - 1 ) ) {
      fid_off = (uint32_t) ( h * fidbits );
      j       = fid_off / 8;
      val     = ( (uint64_t) httab[ j ] ) |
                ( (uint64_t) httab[ j + 1 ] << 8 ) |
                ( (uint64_t) httab[ j + 2 ] << 16 ) |
                ( (uint64_t) httab[ j + 3 ] << 24 ) |
                ( (uint64_t) httab[ j + 4 ] << 32 );
      val   >>= fid_off % 8;
      val    &= fid_mask;
      if ( val == 0 )
        break;
    }
    
    val   = fp->fid - dict_idx->min_fid + 1;
    val <<= fid_off % 8;

    for ( j = fid_off / 8; val != 0; j++ ) {
      httab[ j ] |= ( val & 0xffU );
      val >>= 8;
    }
  }
  for ( MDEnumList *ep = dict_idx->enum_q.hd; ep != NULL; ep = ep->next ) {
    maptab[ ep->map.map_num ] = /* maptab[ maptab[ j ] ] */
      (uint32_t) ( (uint32_t *) mapdata - maptab );
    size_t mapsz = ep->map.map_sz() + sizeof( MDEnumMap );
    ::memcpy( mapdata, &ep->map, mapsz );
    mapdata = &mapdata[ mapsz ];
  }
  if ( ntags > 0 ) {
    uint8_t * p = &((uint8_t *) ptr)[ tag_off ];
    for ( t = dict_idx->tag_q.hd; t != NULL; t = t->next ) {
      p[ 0 ] = ( t->len & 0xff );
      ::memcpy( &p[ 1 ], t->val, p[ 0 ] );
      p = &p[ 1 + p[ 0 ] ];
    }
    p[ 0 ] = 0;
  }
  ((char *) ptr)[ sz - 4 ] = 'z'; /* mark end 1MD0 */
  ((char *) ptr)[ sz - 3 ] = 'M';
  ((char *) ptr)[ sz - 2 ] = 'D';
  ((char *) ptr)[ sz - 1 ] = '0';

  return 0;
}

bool
MDDict::get_enum_text( MDFid fid,  uint16_t val,  const char *&disp,
                       size_t &disp_len ) noexcept
{
  MDLookup by( fid );
  if ( ! this->lookup( by ) )
    return false;
  if ( by.ftype != MD_ENUM )
    return false;
  return this->get_enum_map_text( by.enummap, val, disp, disp_len );
}

bool
MDDict::get_enum_map_text( uint32_t map_num,  uint16_t val,  const char *&disp,
                           size_t &disp_len ) noexcept
{
  if ( map_num >= this->map_count )
    return false;
  uint32_t * maptab = (uint32_t *) (void *)
                      &((uint8_t *) (void *) this)[ this->map_off ];
  if ( maptab[ map_num ] == 0 )
    return false;
  MDEnumMap * map  = (MDEnumMap *) (void *) &maptab[ maptab[ map_num ] ];
  uint16_t  * vidx = map->value();
  uint8_t   * vmap = map->map();
  if ( val > map->max_value )
    return false;
  disp_len = map->max_len; /* all enum text is the same size */
  if ( vidx == NULL ) {
    disp = (char *) &vmap[ val * disp_len ];
    return true;
  }
  else {
    uint32_t size = map->value_cnt, k = 0, piv;
    for (;;) {
      if ( size == 0 )
        break;
      piv = size / 2;
      if ( val <= vidx[ k + piv ] ) {
        size = piv;
      }
      else {
        size -= piv + 1;
        k    += piv + 1;
      }
    }
    if ( vidx[ k ] == val ) {
      disp = (char *) &vmap[ k * disp_len ];
      return true;
    }
  }
  disp = NULL;
  return false;
}

bool
MDDict::get_enum_val( MDFid fid,  const char *disp,  size_t disp_len,
                      uint16_t &val ) noexcept
{
  MDLookup by( fid );
  if ( ! this->lookup( by ) )
    return false;
  if ( by.ftype != MD_ENUM )
    return false;
  return this->get_enum_map_val( by.enummap, disp, disp_len, val );
}

bool
MDDict::get_enum_map_val( uint32_t map_num,  const char *disp,  size_t disp_len,
                          uint16_t &val ) noexcept
{
  if ( map_num >= this->map_count )
    return false;
  uint32_t * maptab = (uint32_t *) (void *)
                      &((uint8_t *) (void *) this)[ this->map_off ];
  if ( maptab[ map_num ] == 0 )
    return false;
  MDEnumMap * map  = (MDEnumMap *) (void *) &maptab[ maptab[ map_num ] ];
  uint16_t  * vidx = map->value();
  uint8_t   * vmap = map->map();
  uint32_t    len  = map->max_len,
              dlen = ( (uint32_t) disp_len < len ? (uint32_t) disp_len : len );
  for ( uint32_t i = 0; i < map->value_cnt; i++ ) {
    for ( uint32_t j = 0; j < dlen; j++ ) {
      if ( vmap[ i * len + j ] != disp[ j ] )
        goto not_equal;
    }
    if ( vidx == NULL )
      val = (uint16_t) i;
    else
      val = vidx[ i ];
    return true;
  not_equal:;
  }
  val = 0;
  return false;
}

MDEnumMap *
MDDict::get_enum_map( uint32_t map_num ) noexcept
{
  if ( map_num >= this->map_count )
    return NULL;
  uint32_t * maptab = (uint32_t *) (void *)
                      &((uint8_t *) (void *) this)[ this->map_off ];
  if ( maptab[ map_num ] == 0 )
    return NULL;
  return (MDEnumMap *) (void *) &maptab[ maptab[ map_num ] ];
}

bool
MDDict::first_tag( const char *&tag,  size_t &tag_sz ) noexcept
{
  if ( this->tag_off == 0 )
    return false;
  uint8_t * tptr = &((uint8_t *) (void *) this)[ this->tag_off ];
  if ( tptr[ 0 ] == 0 )
    return false;
  tag_sz = tptr[ 0 ];
  tag    = (char *) &tptr[ 1 ];
  return true;
}

bool
MDDict::find_tag( const char *name,  const char *&val,
                  size_t &val_sz ) noexcept
{
  const char * tag;
  size_t       tag_sz;
  size_t       name_len = ::strlen( name );
  bool         b = this->first_tag( tag, tag_sz );
  while ( b ) {
    if ( tag_sz > name_len ) {
      if ( ::strncasecmp( name, tag, name_len ) == 0 &&
           tag[ name_len ] == ' ' ) {
        val = &tag[ name_len + 1 ];
        val_sz = tag_sz - ( name_len + 1 );
        return true;
      }
    }
    b = this->next_tag( tag, tag_sz );
  }
  return false;
}

bool
MDDict::next_tag( const char *&tag,  size_t &tag_sz ) noexcept
{
  tag = &tag[ tag_sz ];
  if ( tag[ 0 ] == 0 )
    return false;
  tag_sz = ((uint8_t *) tag)[ 0 ];
  tag = &tag[ 1 ];
  return true;
}

MDDictBuild::~MDDictBuild() noexcept
{
  if ( this->idx != NULL )
    delete this->idx;
}

void
MDDictBuild::clear_build( void ) noexcept
{
  if ( this->idx != NULL ) {
    delete this->idx;
    this->idx = NULL;
  }
}

bool
DictParser::find_file( const char *path,  const char *filename,
                       size_t file_sz,  char *path_found ) noexcept
{
  const char *next = NULL, * e;
  size_t path_sz;
  char path2[ 1024 ];

  if ( path == NULL )
    path = ".";
  while ( path != NULL ) {
#ifdef _MSC_VER
    e = (const char *) ::strchr( path, ';' );
#else
    e = (const char *) ::strchr( path, ':' );
#endif
    if ( e == NULL ) {
      e = &path[ ::strlen( path ) ];
      next = NULL;
    }
    else {
      next = &e[ 1 ];
    }
    path_sz = (size_t) ( e - path );
    if ( path_sz > 0 && path_sz + file_sz + 2 < sizeof( path2 ) ) {
      ::memcpy( path2, path, path_sz );
#ifdef _MSC_VER
      if ( path_sz > 0 && path2[ path_sz - 1 ] != '/' &&
           path2[ path_sz - 1 ] != '\\' )
        path2[ path_sz++ ] = '/';
#else
      if ( path_sz > 0 && path2[ path_sz - 1 ] != '/' )
        path2[ path_sz++ ] = '/';
#endif
      ::memcpy( &path2[ path_sz ], filename, file_sz );
      path2[ path_sz + file_sz ] = '\0';
      int status;
#ifdef _MSC_VER
      status = _access( path2, 04 );
#else
      status = ::access( path2, R_OK );
#endif
      if ( status == 0 ) {
        size_t end = path_sz + file_sz + 1;
        ::memcpy( path_found, path2, end );
        return true;
      }
    }
    path = next;
  }
  return false;
}

bool
DictParser::fillbuf( void ) noexcept
{
  size_t x = this->len - this->off, y = 0;
  if ( x > 0 )
    ::memmove( this->buf, &this->buf[ this->off ], x );
  this->off = 0;
  this->len = x;
  if ( ! this->is_eof ) {
    if ( this->fname[ 0 ] != '\0' ) {
      if ( this->fp == NULL ) {
        if ( (this->fp = ::fopen( this->fname, "rb" )) == NULL ) {
          perror( this->fname );
          this->is_eof = true;
          return false;
        }
        if ( ( this->debug_flags & MD_DICT_PRINT_FILES ) != 0 ) {
          fprintf( stderr, "Loading %s: \"%s\"\n", this->dict_kind,
                   this->fname );
        }
      }
      y = ::fread( &this->buf[ x ], 1, sizeof( buf ) - x, this->fp );
    }
    else if ( this->str_size != 0 ) {
      y = sizeof( buf ) - x;
      if ( y > this->str_size )
        y = this->str_size;
      ::memcpy( &this->buf[ x ], this->str_input, y );
      this->str_input += y;
      this->str_size  -= y;
    }
    else {
      this->is_eof = true;
      return false;
    }
    this->len += y;
  }
  return y > 0;
}

bool
DictParser::get_char( size_t i,  int &c ) noexcept
{
  if ( this->off + i >= this->len ) {
    if ( ! this->fillbuf() ) {
      this->close();
      return false;
    }
  }
  c = (int) (uint8_t) this->buf[ this->off + i ];
  return true;
}

int
DictParser::eat_white( void ) noexcept
{
  int c;
  while ( this->get_char( 0, c ) ) {
    if ( ! isspace( c ) )
      return c;
    if ( c == '\n' ) {
      this->lineno++;
      this->col = 0;
    }
    this->off++;
  }
  return EOF_CHAR;
}

void
DictParser::eat_comment( void ) noexcept
{
  int c;
  if ( ! this->get_char( 0, c ) || c == '\n' )
    return;
  while ( this->get_char( 1, c ) ) {
    this->off++;
    if ( c == '\n' )
      break;
  }
}

bool
DictParser::match( const char *s,  size_t sz ) noexcept
{
  while ( this->off + sz > this->len )
    if ( ! this->fillbuf() )
      break;
  return this->off + sz <= this->len &&
         ::strncasecmp( s, &this->buf[ this->off ], sz ) == 0;
}

size_t
DictParser::match_tag( const char *s,  size_t sz ) noexcept
{
  if ( ! this->match( s, sz ) )
    return 0;
  this->off += sz;
  for (;;) {
    char *eol = (char *)
      ::memchr( &this->buf[ this->off ], '\n', this->len - this->off );
    if ( eol != NULL ) {
      for ( ; &this->buf[ this->off ] < eol; this->off++ ) {
        if ( ! isspace( this->buf[ this->off ] ) )
          break;
      }
      sz = eol - &this->buf[ this->off ];
      while ( sz > 0 && isspace( eol[ -1 ] ) ) {
        eol--;
        sz--;
      }
      return sz;
    }
    if ( ! this->fillbuf() )
      return 0;
  }
}

int
DictParser::consume_tok( int k,  size_t sz ) noexcept
{
  ::memcpy( this->tok_buf, &this->buf[ this->off ], sz );
  this->tok_sz = sz;
  this->off += sz;
  this->tok = k;
  return k;
}

int
DictParser::consume_int_tok( void ) noexcept
{
  size_t sz;
  int c;
  for ( sz = 1; ; sz++ ) {
    if ( ! this->get_char( sz, c ) )
      break;
    if ( ! isdigit( c ) ) {
      if ( ! isalpha( c ) && c != '_' )
        break;
      return this->consume_ident_tok();
    }
  }
  return this->consume_tok( this->int_tok, sz );
}

int
DictParser::consume_ident_tok( void ) noexcept
{
  size_t sz;
  int c;
  for ( sz = 1; ; sz++ ) {
    if ( ! this->get_char( sz, c ) )
      break;
    if ( isspace( c ) || c == ';' )
      break;
  }
  return this->consume_tok( this->ident_tok, sz );
}

int
DictParser::consume_string_tok( void ) noexcept
{
  size_t sz;
  int c;
  for ( sz = 1; ; sz++ ) {
    if ( ! this->get_char( sz, c ) )
      return this->consume_tok( this->error_tok, sz );
    if ( c == '\"' )
      break;
  }
  ::memcpy( this->tok_buf, &this->buf[ this->off + 1 ], sz - 1 );
  this->tok_sz = sz - 1;
  this->off += sz + 1;
  this->tok = this->ident_tok;
  return this->ident_tok;
}

