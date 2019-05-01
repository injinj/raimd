#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <raimd/md_dict.h>

using namespace rai;
using namespace md;

/* murmur */
uint32_t
MDDict::dict_hash( const void *key,  size_t len,  uint32_t seed )
{
  const uint32_t m = 0x5bd1e995;
  const uint8_t * data = (const uint8_t *) key;
  uint32_t h = seed ^ len;

  while ( len >= 4 ) {
    uint32_t k = ((uint32_t *) data)[ 0 ];
    k *= m;
    k ^= k >> 24;
    k *= m;
    h *= m;
    h ^= k;
    data = &data[ 4 ];
    len -= 4;
  }
  switch( len ) {
    case 3: h ^= data[ 2 ] << 16;
    case 2: h ^= data[ 1 ] << 8;
    case 1: h ^= data[ 0 ];
            h *= m;
  };
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

uint32_t
MDDictIdx::file_lineno( const char *filename,  uint32_t lineno )
{
  if ( this->file_q.is_empty() ||
       ::strcmp( filename, this->file_q.hd->filename ) != 0 ) {
    size_t len = sizeof( MDFilename ) - DICT_BUF_PAD + ::strlen( filename ) + 1;
    MDFilename * fn = this->alloc<MDFilename>( len );
    fn->id = ( this->file_q.hd == NULL ? 0 : this->file_q.hd->id + 1 );
    ::strcpy( fn->filename, filename );
    this->file_q.push_hd( fn );
  }
  return ( this->file_q.hd->id << 24 ) | lineno;
}

size_t
MDDictIdx::total_size( void )
{
  size_t sz = 0;
  for ( MDEntryData *d = this->data_q.hd; d != NULL; d = d->next )
    sz += d->off;
  return sz;
}

size_t
MDDictIdx::fname_size( uint8_t &shft,  uint8_t &align )
{
  size_t sz = 0, sz2 = 0, sz4 = 0, sz8 = 0;
  uint8_t n;
  for ( MDDictEntry *d = this->entry_q.hd; d != NULL; d = d->next ) {
    n    = ( d->fnamelen + 1 );
    sz  += n;
    sz2 += ( n + 1 ) & ~1;
    sz4 += ( n + 3 ) & ~3;
    sz8 += ( n + 7 ) & ~7;
  }
  for ( n = 1; ( 1U << n ) < sz; n++ )
    ;
  shft = n;
  if ( sz8 < ( 1U << n ) ) {
    align = 3;
    return sz8;
  }
  if ( sz4 < ( 1U << n ) ) {
    align = 2;
    return sz4;
  }
  if ( sz2 < ( 1U << n ) ) {
    align = 1;
    return sz2;
  }
  align = 0;
  return sz;
}

bool
MDDupHash::test_set( uint32_t h )
{
  size_t    i = ( h & ( 64 * 1024 - 1 ) ),
            j = ( ( h >> 16 ) & ( 64 * 1024 - 1 ) );
  uint8_t & w = this->ht1[ i / 8 ],
          & x = this->ht2[ j / 8 ],
            y = ( 1U << ( i & 7 ) ),
            z = ( 1U << ( j & 7 ) );
  bool b = ( w & y ) != 0 && ( x & z ) != 0;
  w |= y; x |= z;
  return b;
}

uint32_t
MDDictIdx::check_dup( const char *fname,  uint8_t fnamelen,  bool &xdup )
{
  if ( this->dup_hash == NULL )
    this->dup_hash = this->alloc<MDDupHash>( sizeof( MDDupHash ) );
  uint32_t h = MDDict::dict_hash( fname, fnamelen, 0 );
  xdup = this->dup_hash->test_set( h );
  return h;
}

template< class TYPE >
TYPE *
MDDictIdx::alloc( size_t sz )
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

MDDictIdx::~MDDictIdx()
{
  MDEntryData *p;
  while ( (p = this->data_q.pop()) != NULL )
    ::free( p );
  this->entry_q.init();
  this->file_q.init();
  this->dup_hash = NULL;
  this->type_hash = NULL;
}

bool
MDTypeHash::insert( uint32_t x )
{
  uint32_t h = ( ( ( x >> 5 ) * 0x5bd1e995 ) ^ (uint32_t) ( x & 0x1f ) ), i;

  if ( this->htcnt >= this->htsize - 1 )
    return false;
  for ( i = h & ( this->htsize - 1 ); ; i = ( i + 1 ) % ( this->htsize - 1 ) ) {
    if ( this->ht[ i ] == x )
      break;
    if ( this->ht[ i ] == 0 ) {
      this->ht[ i ] = x;
      this->htcnt++;
      break;
    }
  }
  return true;
}

uint32_t
MDTypeHash::find( uint32_t x )
{
  uint32_t h = ( ( ( x >> 5 ) * 0x5bd1e995 ) ^ (uint32_t) ( x & 0x1f ) ), i;

  for ( i = h & ( this->htsize - 1 ); ; i = ( i + 1 ) % ( this->htsize - 1 ) ) {
    if ( this->ht[ i ] == x )
      return i;
  }
}

MDDictIdx *
MDDictBuild::get_dict_idx( void )
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
MDDictBuild::add_enum_map( uint32_t map_num,  uint16_t max_value,
                           uint32_t value_cnt,  uint16_t *value,
                           uint16_t map_len,  uint8_t *map )
{
  MDEnumList * emap;
  uint16_t   * value_cp;
  uint8_t    * map_cp;
  size_t       map_sz = MDEnumMap::map_sz( value_cnt, max_value, map_len ),
               sz     = sizeof( MDEnumList ) + map_sz;
  MDDictIdx * dict = this->get_dict_idx();
  if ( dict == NULL )
    return Err::ALLOC_FAIL;
  emap = dict->alloc<MDEnumList>( sz );
  emap->map.map_num   = map_num;
  emap->map.max_value = max_value;
  emap->map.value_cnt = value_cnt;
  emap->map.map_len   = map_len;
  value_cp = emap->map.value();
  map_cp   = emap->map.map();
  if ( value_cp != NULL )
    ::memcpy( value_cp, value, sizeof( uint16_t ) * value_cnt );
  ::memcpy( map_cp, map, (size_t) map_len * (size_t) value_cnt );
  dict->enum_q.push_tl( emap );
  if ( map_num >= dict->map_cnt )
    dict->map_cnt = map_num + 1;
  dict->map_size += sizeof( MDEnumMap ) + map_sz;

  return 0;
}

int
MDDictBuild::add_entry( MDFid fid,  uint32_t fsize,  MDType ftype,
                        const char *fname,  const char *name,
                        const char *ripple,  const char *filename,
                        uint32_t lineno,  MDDictEntry **eret )
{
  MDDictEntry * entry;
  size_t        fnamelen,
                namelen,
                ripplelen;
  char        * ptr;
  uint32_t      h;
  bool          xdup;

  size_t sz = sizeof( MDDictEntry ) - DICT_BUF_PAD +
              ( fnamelen  = ( fname  ? ::strlen( fname )  + 1 : 0 ) ) +
              ( namelen   = ( name   ? ::strlen( name )   + 1 : 0 ) ) +
              ( ripplelen = ( ripple ? ::strlen( ripple ) + 1 : 0 ) );

  if ( eret != NULL )
    *eret = NULL;
  if ( (fnamelen | namelen | ripplelen) > 0xff )
    return Err::BAD_NAME;

  MDDictIdx * dict = this->get_dict_idx();
  if ( dict == NULL )
    return Err::ALLOC_FAIL;
  h = dict->check_dup( fname, fnamelen, xdup ); /* bloom filter of existing */
  if ( xdup ) {
    xdup = false;
    entry = NULL;
    /* enums are declared twice, in RDM and enumtype.def; these are hashed */
    if ( ftype == MD_ENUM ) {
      entry = dict->enum_ht[ h % MDDictIdx::ENUM_HTSZ ];
      if ( entry != NULL ) {
        if ( entry->hash == h &&
             entry->fnamelen == fnamelen &&
             ::memcmp( entry->fname(), fname, fnamelen ) == 0 )
          goto found_enum_fname;
        entry = NULL;
      }
    }
    /* not enum, start at head and scan for duplicates (should be rare) */
    if ( entry == NULL )
      entry = dict->entry_q.hd;
    for ( ; entry != NULL; entry = entry->next ) {
      /* if matches existing entry */
      if ( entry->hash == h &&
           entry->fnamelen == fnamelen &&
           ::memcmp( entry->fname(), fname, fnamelen ) == 0 ) {
      found_enum_fname:;
        if ( fid == entry->fid && ftype == entry->ftype ) {
          /* no change in size or fid */
          if ( fsize == entry->fsize ) {
            /*fprintf( stderr, "dup decl: %s\n", fname );*/
            if ( eret != NULL )
              *eret = entry;
            return 0; /* already declared */
          }
          /* if defining a map number for an existing enum */
          if ( ftype == MD_ENUM && entry->fsize < 10 ) {
            entry->fsize = fsize;
            goto add_to_type_hash;
          }
        }
        /* if change in size of type with same fid, print error */
        if ( fid == entry->fid &&
             ( fsize != entry->fsize || ftype != entry->ftype ) ) {
          MDFilename *xfn = dict->file_q.hd;
          while ( xfn != NULL && xfn->id != entry->fno )
            xfn = xfn->next;
          if ( xfn != NULL ) {
            fprintf( stderr,
                     "\"%s\":%u and \"%s\":%u "
                     "redeclared: %s (fid %u/%u) fsize (%u/%u) ftype(%u/%u)\n",
                    filename, lineno, xfn->filename, xfn->id & 0xffffff,
                    fname, fid, entry->fid, fsize, entry->fsize, ftype,
                    entry->ftype );
          }
          /* another declaration with same name */
        }
        /* same decl, different fid (fine) */
        xdup = true;
      }
    }
  }
  entry = dict->alloc<MDDictEntry>( sz );
  if ( entry == NULL )
    return Err::ALLOC_FAIL;

  entry->fid   = fid;
  entry->fsize = fsize;
  entry->ftype = ftype;
  entry->hash  = h;
  if ( ! xdup )
    entry->fno = dict->file_lineno( filename, lineno );
  else
    entry->fno = 0;

  ptr = entry->buf;
  if ( (entry->fnamelen = fnamelen) > 0 ) {
    ::memcpy( ptr, fname, fnamelen );
    ptr = &ptr[ fnamelen ];
  }
  if ( (entry->namelen = namelen) > 0 ) {
    ::memcpy( ptr, name, namelen );
    ptr = &ptr[ namelen ];
  }
  if ( (entry->ripplelen = ripplelen) > 0 ) {
    ::memcpy( ptr, ripple, ripplelen );
    ptr = &ptr[ ripplelen ];
  }
  if ( ftype == MD_ENUM ) {
    if ( dict->enum_ht[ h % MDDictIdx::ENUM_HTSZ ] == NULL )
      dict->enum_ht[ h % MDDictIdx::ENUM_HTSZ ] = entry;
  }
  dict->entry_q.push_tl( entry );
  dict->entry_count++;
add_to_type_hash:;
  if ( eret != NULL )
    *eret = entry;
  if ( dict->max_fid == 0 || fid > dict->max_fid )
    dict->max_fid = fid;
  if ( dict->min_fid == 0 || fid < dict->min_fid )
    dict->min_fid = fid;

  if ( dict->type_hash == NULL ) {
    dict->type_hash = dict->alloc<MDTypeHash>( MDTypeHash::alloc_size( 6 ) );
    if ( dict->type_hash == NULL )
      return Err::ALLOC_FAIL;
    dict->type_hash->init( 6 );
  }
  if ( ! dict->type_hash->insert( ftype, fsize ) ) {
    MDTypeHash * sav = dict->type_hash;
    dict->type_hash =
      dict->alloc<MDTypeHash>( MDTypeHash::alloc_size( sav->htshft + 1 ) );
    if ( dict->type_hash == NULL )
      return Err::ALLOC_FAIL;
    dict->type_hash->init( sav->htshft + 1 );
    for ( uint32_t i = 0; i < sav->htsize; i++ ) {
      if ( sav->ht[ i ] != 0 )
        dict->type_hash->insert( sav->ht[ i ] );
    }
    dict->type_hash->insert( ftype, fsize );
  }

  return 0;
}

static size_t
mkpow2( size_t i )
{
  for ( size_t n = i >> 1; ; n >>= 1 ) {
    i |= ( n | 1 );
    if ( ( ( i + 1 ) & i ) == 0 )
      return i + 1;
  }
}

int
MDDictBuild::index_dict( const char *dtype,  MDDict *&dict )
{
  MDDict * next = dict;
  MDDictIdx * dict_idx = this->idx;
  if ( dict_idx == NULL )
    return Err::NO_DICTIONARY;

  uint8_t shft, algn, bits, ttshft;
  size_t  sz      = sizeof( MDDict ),
          fnamesz = dict_idx->fname_size( shft, algn ),
          nfids   = ( dict_idx->max_fid - dict_idx->min_fid ) + 1,
          httabsz = mkpow2( dict_idx->entry_count * 2 ),
          fidbits,
          tabsz,
          typetabsz,
          map_off;

  for ( fidbits = 2; ( 1U << fidbits ) < ( nfids + 1 ); fidbits++ )
    ;
  ttshft    = dict_idx->type_hash->htshft;
  bits      = ttshft + ( shft - algn );
  tabsz     = ( (size_t) bits * nfids + 7 ) / 8; /* fid index */
  typetabsz = dict_idx->type_hash->htsize * sizeof( uint32_t ); /* type hash */
  sz       += typetabsz;  /* type table size */
  sz       += tabsz;   /* fid table size */
  sz       += fnamesz; /* fname buffers size */
  sz       += ( ( httabsz * fidbits ) + 7 ) / 8; /* fname hash table size */
  sz        = ( sz + 3U ) & ~3U;  /* align int32 */
  map_off   = sz;      /* enum map offset */
  sz       += dict_idx->map_cnt * sizeof( uint32_t ) +
              dict_idx->map_size; /* enum map size */
  sz       += 4;       /* end marker */
  void * ptr = ::malloc( sz );
  if ( ptr == NULL )
    return Err::ALLOC_FAIL;
  ::memset( ptr, 0, sz );
  dict   = (MDDict *) ptr;
  /* label the dict kind: cfile or app_a */
  ::strncpy( dict->dict_type, dtype, sizeof( dict->dict_type ) - 1 );
  dict->next        = next;
  dict->min_fid     = dict_idx->min_fid;
  dict->max_fid     = dict_idx->max_fid;
  dict->type_shft   = dict_idx->type_hash->htshft;
  dict->fname_shft  = shft;
  dict->fname_algn  = algn;
  dict->tab_bits    = dict->type_shft + shft - algn;
  dict->tab_off     = typetabsz + sizeof( dict[ 0 ] ); /* from this */
  dict->entry_count = dict_idx->entry_count;
  dict->fname_off   = dict->tab_off + tabsz;
  dict->tab_size    = tabsz;
  dict->ht_off      = dict->tab_off + tabsz + fnamesz;
  dict->ht_size     = httabsz;
  dict->map_off     = map_off;
  dict->map_count   = dict_idx->map_cnt;
  dict->fid_bits    = fidbits;
  dict->dict_size   = sz; /* write( fd, dict, sz ) does save/replicate dict */

  uint32_t * typetab   = (uint32_t *) &((uint8_t *) ptr)[ sizeof( dict[ 0 ] ) ],
           * maptab    = (uint32_t *) &((uint8_t *) ptr)[ map_off ];
  uint8_t  * tab       = (uint8_t *) &typetab[ dict_idx->type_hash->htsize ],
           * fntab     = &tab[ tabsz ],
           * httab     = &fntab[ fnamesz ],
           * mapdata   = (uint8_t *) (void *) &maptab[ dict_idx->map_cnt ];
  uint32_t   fname_off = 0,
             algn_mask = ( 1U << algn ) - 1,
             fid_mask  = ( 1U << fidbits ) - 1,
             fid_off,
             j, h;
  uint64_t   val;

  ::memcpy( typetab, dict_idx->type_hash->ht, typetabsz );
  for ( MDDictEntry *fp = dict_idx->entry_q.hd; fp != NULL; fp = fp->next ) {
    val = dict_idx->type_hash->find( fp->ftype, fp->fsize );
    val = ( val << ( shft - algn ) ) | ( fname_off >> algn );

    fntab[ fname_off++ ] = fp->fnamelen;
    ::memcpy( &fntab[ fname_off ], fp->fname(), fp->fnamelen );

    fname_off = ( ( fname_off + fp->fnamelen ) + algn_mask ) & ~algn_mask;
    fid_off   = bits * ( fp->fid - dict_idx->min_fid );
    val     <<= ( fid_off % 8 );

    for ( j = fid_off / 8; val != 0; j++ ) {
      tab[ j ] |= ( val & 0xffU );
      val >>= 8;
    }

    h = fp->hash & ( httabsz - 1 );
    for ( ; ; h = ( h + 1 ) & ( httabsz - 1 ) ) {
      fid_off = h * fidbits;
      j       = fid_off / 8;
      val     = httab[ j ] | ( httab[ j + 1 ] << 8 ) |
                ( httab[ j + 2 ] << 16 ) | ( httab[ j + 3 ] << 24 );
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
  size_t testsz = 0;
  for ( MDEnumList *ep = dict_idx->enum_q.hd; ep != NULL; ep = ep->next ) {
    maptab[ ep->map.map_num ] = /* maptab[ maptab[ j ] ] */
      (uint32_t *) mapdata - maptab;
    size_t mapsz = ep->map.map_sz() + sizeof( MDEnumMap );
    ::memcpy( mapdata, &ep->map, mapsz );
    mapdata = &mapdata[ mapsz ];
    testsz += mapsz;
  }
  ((char *) ptr)[ sz - 4 ] = 'z'; /* mark end 1MD0 */
  ((char *) ptr)[ sz - 3 ] = 'M';
  ((char *) ptr)[ sz - 2 ] = 'D';
  ((char *) ptr)[ sz - 1 ] = '0';

  return 0;
}

bool
MDDict::get_enum_text( MDFid fid,  uint16_t val,  const char *&disp,
                       size_t &disp_len )
{
  const char * fname;
  uint32_t     fsize;
  MDType       ftype;
  uint8_t      fnamelen;

  if ( ! this->lookup( fid, ftype, fsize, fnamelen, fname ) )
    return false;
  if ( ftype != MD_ENUM )
    return false;
  return this->get_enum_map_text( fsize, val, disp, disp_len );
}

bool
MDDict::get_enum_map_text( uint32_t map_num,  uint16_t val,  const char *&disp,
                           size_t &disp_len )
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
  disp_len = map->map_len; /* all enum text is the same size */
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
                      uint16_t &val )
{
  const char * fname;
  uint32_t     fsize;
  MDType       ftype;
  uint8_t      fnamelen;

  if ( ! this->lookup( fid, ftype, fsize, fnamelen, fname ) )
    return false;
  if ( ftype != MD_ENUM )
    return false;
  return this->get_enum_map_val( fsize, disp, disp_len, val );
}

bool
MDDict::get_enum_map_val( uint32_t map_num,  const char *disp,  size_t disp_len,
                          uint16_t &val )
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
  uint32_t    len  = map->map_len,
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

MDDictBuild::~MDDictBuild()
{
  if ( this->idx != NULL )
    delete this->idx;
}

void
MDDictBuild::clear_build( void )
{
  if ( this->idx != NULL ) {
    delete this->idx;
    this->idx = NULL;
  }
}

bool
DictParser::find_file( const char *path,  const char *filename,
                       size_t file_sz,  char *path_found )
{
  const char *next = NULL, * e;
  size_t path_sz;
  char path2[ 1024 ];

  if ( path == NULL )
    path = ".";
  while ( path != NULL ) {
    e = (const char *) ::strchr( path, ':' );
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

      if ( path_sz > 0 && path2[ path_sz - 1 ] != '/' )
        path2[ path_sz++ ] = '/';
      ::memcpy( &path2[ path_sz ], filename, file_sz );
      path2[ path_sz + file_sz ] = '\0';
      if ( ::access( path2, R_OK ) == 0 ) {
        ::strcpy( path_found, path2 );
        return true;
      }
    }
    path = next;
  }
  return false;
}

bool
DictParser::fillbuf( void )
{
  size_t x = this->len - this->off, y = 0;
  if ( x > 0 )
    ::memmove( this->buf, &this->buf[ this->off ], x );
  this->off = 0;
  this->len = x;
  if ( ! this->is_eof ) {
    if ( this->fp == NULL ) {
      if ( (this->fp = ::fopen( this->fname, "r" )) == NULL ) {
        this->is_eof = true;
        return false;
      }
    }
    y = ::fread( &this->buf[ x ], 1, sizeof( buf ) - x, this->fp );
    this->len += y;
  }
  return y > 0;
}

bool
DictParser::get_char( size_t i,  int &c )
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
DictParser::eat_white( void )
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
DictParser::eat_comment( void )
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
DictParser::match( const char *s,  size_t sz )
{
  while ( this->off + sz > this->len )
    if ( ! this->fillbuf() )
      break;
  return this->off + sz <= this->len &&
         ::strncasecmp( s, &this->buf[ this->off ], sz ) == 0;
}

int
DictParser::consume_tok( int k,  size_t sz )
{
  ::memcpy( this->tok_buf, &this->buf[ this->off ], sz );
  this->tok_sz = sz;
  this->off += sz;
  this->tok = k;
  return k;
}

int
DictParser::consume_int_tok( void )
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
DictParser::consume_ident_tok( void )
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
DictParser::consume_string_tok( void )
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

