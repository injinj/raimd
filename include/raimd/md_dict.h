#ifndef __rai_raimd__md_dict_h__
#define __rai_raimd__md_dict_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

/* The MDDict is a fid -> fname / type map, using 4 tables (+enums):
 *   * type table,  uint32_t size,
 *       Each element indictes size and type of field,
 *       The lower 5 bits is the type, the upper 27 bits is the size
 *         el[ type_id ] = ( field_size << 27 ) | ( field_type )
 *       The number of elements in the table is ( 1 << type_shft )
 *       The type_id is encoded into the fid table to indicate the type
 *       This table enumerates all of the types used.. this is usually
 *       less than 64 types (56 in my dictionary).
 *   * fid  table,  bit array
 *       This is an array of bits indexed by fid - min_fid
 *       The string of bits used at this offset is tab_bits
 *       The bits are
 *         el[ fid bit offset ] = ( type_idx << N ) | field_offset
 *       Where N is ( fname_shft - fname_algn )
 *   * fname table, byte array
 *       This is an array of bytes for each field name prefixed by the
 *       length of the field name string length.
 *       The size of the table is ( 1 << fname_shft ), and each field
 *       is aligned on the ( 1 << fname_algn ) boundary.
 *       The index stored in the fid table is ( byte_index >> fname_algn )
 *   * fname hash, bit array
 *       This hashes the fnames to fids, it is a linear hash table where each
 *       slot is a bit string big enough (usually 14 bits) to fit ( fid -
 *       min_fid + 1 ) or zero when empty.
 *       The hash used is MDDict::dict_hash( fname, fnamelen )
 *       The size of the table is a power of 2 greater than
 *       2 * entry count.
 *       Entry count is the count of fnames, it may be less than the number of
 *       fids if a fname maps to more than one fid (a common warning that occurs
 *       in many live systems).
 *   * enum tables, array of tables, one for each enum definition
 *       The tables are indexed by the type of the field.  The field_size
 *       is a table number, so a enum type is encoded as:
 *         fid type = ( ( map num + 1 ) << 27 ) | MD_ENUM 
 *       Each map table is a MDEnumMap structure, which is either an array
 *       of strings or a sorted array of enum values to strings when enum
 *       mappings are sparse.  The enum strings are a fixed size.
 *         A dense map of a 3 char enum string would be
 *           [ "   ", "ASE", "NYS", "BOS", etc ] where 0 = "   ", 1 = "ASE"
 *         A sparse map 
 *           [ 0, 4, 8, 12 ], [ "   ", "AFA", "ALL", "DZD" ] where 4 = "AFA"
 */
struct MDDict {
  /*
   * [ type table (uint32 elem) ]  = offset 0 (sizeof(MDDict))
   * [ fid table (tab bits elem) ] = offset tab_off (+ntypes * uint32)
   * [ fname table (string elem) ] = offset fname_off (+fid_bits * nfids)
   * [ fname ht (tab bits ht) ]    = offset ht_off (+fnamesz ht_off - fname_off)
   * [ enum off (uint32_t off) ]   = offset map_off (+fid_bits * ht_size)
   */
  MDDict * next; /* list of multiple dictionaries (usually just 2, cfile&RDM )*/
  char     dict_type[ 8 ]; /* cfile or RDM(app_a) */
  MDFid    min_fid,     /* minumum fid indexed */
           max_fid;     /* maximum fid indexed, inclusive */
  uint32_t tab_off,     /* offset of bit array table */
           tab_size,    /* size in bytes of bit array table */
           ht_off,      /* offset of fname hash */
           ht_size,     /* size in fids of ht */
           entry_count, /* number of fids in the table */
           fname_off,   /* offset of fname start */
           map_off,     /* offset of enum map index */
           map_count,   /* count of enum maps */
           dict_size;   /* sizeof this + tables */
  uint8_t  type_shft,   /* how many bits used for the type */
           fname_shft,  /* how many bits used for fname offset */
           fname_algn,  /* what byte alignment used for fname */
           tab_bits,    /* = type_shft + fname_shft - fname_algn*/
           fid_bits,    /* number of bits needed for a fid - min_fid + 1 */
           pad[ 3 ];    /* pad to an int32 size */

  /* used by get() below */
  static uint32_t dict_hash( const void *key,  size_t len,
                             uint32_t seed ) noexcept;
  /* lookup fid, return {type, size, name/len} if found */
  bool lookup( MDFid fid,  MDType &ftype,  uint32_t &fsize,
               uint8_t &flags,  uint8_t &fnamelen,  const char *&fname ) const {
    if ( fid < this->min_fid || fid > this->max_fid )
      return false;

    uint32_t bits = this->tab_bits, /* fname offset + type bits */
             mask = ( 1U << bits ) - 1,
             i    = ( (uint32_t) fid - (uint32_t) this->min_fid ) * bits,
             off  = i / 8,  /* byte offset for fid entry */
             shft = i % 8,  /* shift from byte offset */
             fname_bits = ( this->fname_shft - this->fname_algn ),
             fname_mask = ( 1U << fname_bits ) - 1,
             type_idx,
             fname_idx;
    uint64_t val;   /* bits at offset copied into val */
    /* get the bits assoc with fid at bit index tab[ ( fid-min_fid ) * bits ] */
    const uint8_t * tab = &((const uint8_t *) (void *) this)[ this->tab_off ];
    val = tab[ off ] | ( tab[ off + 1 ] << 8 ) | ( tab[ off + 2 ] << 16 )
        | ( tab[ off + 3 ] << 24 );
    for ( off += 4; off * 8 < bits + shft; off++ )
      val |= (uint64_t) tab[ off ] << ( off * 8 );
    /* the type of the fid is first, then the fname index */
    type_idx  = ( val >> shft ) & mask;
    if ( type_idx == 0 ) /* ftype is never zero (MD_NONE), zero is not found */
      return false;
    fname_idx = ( type_idx & fname_mask ) << this->fname_algn;
    type_idx  = type_idx >> fname_bits;
    /* get the type/size using the type_idx from the type table */
    const uint32_t * ttab = (const uint32_t *) (void *) &this[ 1 ];
    ftype     = (MDType) ( ttab[ type_idx ] & 0x1f );
    flags     = ( ttab[ type_idx ] >> 5 ) & 0x3;
    fsize     = ttab[ type_idx ] >> 7;
    /* get the field name */
    const uint8_t * fntab =
      &((const uint8_t *) (void *) this)[ this->fname_off ];
    fnamelen  = fntab[ fname_idx ];
    fname     = (const char *) &fntab[ fname_idx + 1 ];
    return true;
  }

  /* get a field by name, return {fid, type, size} if found */
  bool get( const char *fname,  uint8_t fnamelen,  MDFid &fid,  MDType &ftype,
            uint32_t &fsize,  uint8_t &flags ) const {
    uint32_t h = dict_hash( fname, fnamelen, 0 ) & ( this->ht_size - 1 );

    uint32_t bits = this->fid_bits, /* size in bits of each hash entry */
             mask = ( 1U << bits ) - 1, /* mask for bits */
             val;
    /* the hash table */
    const uint8_t * tab = &((const uint8_t *) (void *) this)[ this->ht_off ];
    const char * fn; /* field name */
    uint8_t len;     /* field name length */
    /* scan the linear hash table until found or zero (not found) */
    for ( ; ; h = ( h + 1 ) & ( this->ht_size - 1 ) ) {
      uint32_t i    = h * bits,
               off  = i / 8,
               shft = i % 8;

      val = tab[ off ] | ( tab[ off + 1 ] << 8 ) | ( tab[ off + 2 ] << 16 )
          | ( tab[ off + 3 ] << 24 );
      val = ( val >> shft ) & mask;
      if ( val == 0 ) /* if end of hash chain */
        return false;
      val = val + this->min_fid - 1;
      /* val is the fid contained at this position */
      if ( this->lookup( val, ftype, fsize, flags, len, fn ) ) {
        if ( len == fnamelen && ::memcmp( fn, fname, len ) == 0 ) {
          fid = val;
          return true;
        }
      }
    }
  }
  /* get display text using fid & value */
  bool get_enum_text( MDFid fid,  uint16_t val,  const char *&disp,
                      size_t &disp_len ) noexcept;
  /* get display text using map number & value */
  bool get_enum_map_text( uint32_t map_num,  uint16_t val,  const char *&disp,
                          size_t &disp_len ) noexcept;
  /* get value using fid & display text (reverse lookup) */
  bool get_enum_val( MDFid fid,  const char *disp,  size_t disp_len,  
                     uint16_t &val ) noexcept;
  /* get value using map num & display text (reverse lookup) */
  bool get_enum_map_val( uint32_t map_num,  const char *disp,  size_t disp_len,  
                         uint16_t &val ) noexcept;
};

/* The following structures are used to build the MDDict, only used when
 * building the index */
struct MDDictIdx;
struct MDDictEntry;
struct MDDictBuild {
  MDDictIdx * idx;

  MDDictBuild() : idx( 0 ) {}
  ~MDDictBuild() noexcept;

  MDDictIdx *get_dict_idx( void ) noexcept;
  int add_entry( MDFid fid,  uint32_t fsize,  MDType ftype,  uint8_t flags,
                 const char *fname,  const char *name,  const char *ripple,
                 const char *filename,  uint32_t lineno,
                 MDDictEntry **eret = NULL ) noexcept;
  int add_enum_map( uint32_t map_num,  uint16_t max_value,  uint32_t value_cnt,
                    uint16_t *value,  uint16_t val_len, uint8_t *map ) noexcept;
  int index_dict( const char *dtype,  MDDict *&dict ) noexcept;
  void clear_build( void ) noexcept;
};

template <class LIST>
struct MDQueue { /* list used for fields and files */
  LIST * hd, * tl;

  MDQueue() : hd( 0 ), tl( 0 ) {}
  void init( void ) {
    this->hd = this->tl = NULL;
  }
  bool is_empty( void ) const {
    return this->hd == NULL;
  }
  void push_hd( LIST *p ) {
    p->next = this->hd;
    if ( this->hd == NULL )
      this->tl = p;
    this->hd = p;
  }
  void push_tl( LIST *p ) {
    if ( this->tl == NULL )
      this->hd = p;
    else
      this->tl->next = p;
    this->tl = p;
  }
  LIST * pop( void ) {
    LIST * p = this->hd;
    if ( p != NULL ) {
      if ( (this->hd = p->next) == NULL )
        this->tl = NULL;
      p->next = NULL;
    }
    return p;
  }
};

struct MDEntryData { /* buffers for structures */
  MDEntryData * next;
  size_t        off;
  uint8_t       data[ 320 * 1024 - 64 ]; /* should only need one of these */
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDEntryData() : next( 0 ), off( 0 ) {}

  void * alloc( size_t sz ) {
    size_t len = ( sz + 7 ) & ~(size_t) 7,
           i   = this->off;
    if ( this->off + len > sizeof( this->data ) )
      return NULL;
    this->off += len;
    return (void *) &this->data[ i ];
  }
};

static const size_t DICT_BUF_PAD = 4;

struct MDDictEntry {
  MDDictEntry * next;
  MDFid         fid;       /* fid of field */
  uint32_t      fsize,     /* size of element */
                fno,       /* line and filename index of field definition */
                hash;      /* hash of field name (for dup fid checking) */
  MDType        ftype;     /* type of field */
  uint8_t       fnamelen,  /* len of fname (including nul char) */
                namelen,   /* len of name */
                ripplelen, /* len of ripple */
                enum_len,  /* len of enum string */
                mf_type,   /* MF_TIME, MF_ALPHANUMERIC, .. */
                rwf_type,  /* RWF_REAL, RWF_ENUM, ... */
                fld_flags; /* is_primitive, is_fixed */
  uint16_t      mf_len;    /* size of field mf data (string length) */
  uint32_t      rwf_len;   /* size of field rwf data (binary coded) */
  char          buf[ DICT_BUF_PAD ]; /* fname, name, ripple */
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDDictEntry() : next( 0 ), fid( 0 ), fsize( 0 ), fno( 0 ), hash( 0 ),
                  ftype( MD_NODATA ), fnamelen( 0 ), namelen( 0 ),
                  ripplelen( 0 ), enum_len( 0 ), mf_type( 0 ), rwf_type( 0 ),
                  fld_flags( 0 ), mf_len( 0 ), rwf_len( 0 ) {}
  const char *fname( void ) const {
    return this->buf;
  }
  const char *name( void ) const {
    return &(this->fname())[ this->fnamelen ];
  }
  const char *ripple( void ) const {
    return &(this->name())[ this->namelen ];
  }
};

struct MDDupHash {
  uint8_t ht1[ 64 * 1024 / 8 ],
          ht2[ 64 * 1024 / 8 ];
  void * operator new( size_t, void *ptr ) { return ptr; }
  void zero( void ) {
    ::memset( this->ht1, 0, sizeof( this->ht1 ) );
    ::memset( this->ht2, 0, sizeof( this->ht2 ) );
  }
  MDDupHash() { this->zero(); }
  bool test_set( uint32_t h ) noexcept;
};

struct MDTypeHash {
  uint32_t htshft,  /* hash of types used by fields, tab size = 1 << htshft */
           htsize,  /* 1 << htshft */
           htcnt,   /* count of elements used */
           ht[ 1 ]; /* each element is ( size << 5 ) | field type */
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDTypeHash() : htshft( 0 ), htsize( 0 ), htcnt( 0 ) {}
  static uint32_t alloc_size( uint32_t shft ) {
    uint32_t htsz = ( 1U << shft );
    return sizeof( MDTypeHash ) + ( ( htsz - 1 ) * sizeof( uint32_t ) );
  }
  void init( uint32_t shft ) {
    uint32_t htsz = ( 1U << shft );
    this->htshft = shft;
    this->htsize = htsz;
    ::memset( this->ht, 0, sizeof( uint32_t ) * htsz ); /* zero is empty */
  }
  bool insert( uint32_t x ) noexcept; /* insert unique, if full, return false */

  uint32_t find( uint32_t x ) noexcept; /* find element, return index of type */

  bool insert( MDType ftype,  uint32_t fsize,  uint8_t flags ) {
    return this->insert( ( fsize << 7 ) | ( (uint32_t) ( flags & 3 ) << 5 ) |
                                            (uint32_t) ftype );
  }
  uint32_t find( MDType ftype,  uint32_t fsize,  uint8_t flags ) {
    return this->find( ( fsize << 7 ) | ( (uint32_t) ( flags & 3 ) << 5 ) |
                                          (uint32_t) ftype );
  }
};

struct MDFilename {
  MDFilename * next;
  uint32_t     id;  /* filename + line number, location for dict errors */
  char         filename[ DICT_BUF_PAD ];
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDFilename() : next( 0 ), id( 0 ) {}
};

struct MDEnumMap {
  uint32_t map_num,   /* map type number */
           value_cnt; /* count of values, n -> max_value+1 */
  uint16_t max_value, /* enum from 0 -> max_value, inclusive */
           map_len;   /* size of the enum value, same for each entry */

  static size_t map_sz( size_t value_cnt,  size_t max_value,  size_t map_len ) {
    size_t sz = ( map_len * value_cnt + 3U ) & ~3U;
    if ( value_cnt == max_value + 1 )
      return sz;
    return sz + sizeof( uint16_t ) * ( ( value_cnt + 1U ) & ~1U );
  }
  size_t map_sz( void ) const {
    return map_sz( this->value_cnt, this->max_value, this->map_len );
  }
  uint16_t *value( void ) {
    if ( this->value_cnt == (uint32_t) this->max_value + 1 )
      return NULL;
    return (uint16_t *) (void *) &this[ 1 ];
  }
  uint8_t *map( void ) {
    uint32_t off = ( ( this->value_cnt == (uint32_t) this->max_value + 1 ) ?
                     0 : ( ( this->value_cnt + 1U ) & ~1U ) );
    uint16_t * value_cp = (uint16_t *) (void *) &this[ 1 ];
    return (uint8_t *) (void *) &value_cp[ off ];
  }
};

struct MDEnumList {
  MDEnumList * next;
  MDEnumMap    map;
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDEnumList() : next( 0 ) {}
};

struct MDDictIdx {
  MDQueue< MDEntryData > data_q;  /* data buffers */
  MDQueue< MDDictEntry > entry_q; /* list of all fields */
  MDQueue< MDEnumList >  enum_q;  /* list of all enum mappings */
  MDQueue< MDFilename >  file_q;  /* list of all dict files */

  MDDupHash  * dup_hash;  /* duplicate filter, doesn't store value, just hash */
  MDTypeHash * type_hash; /* hash of all fid types */
  MDFid        min_fid,   /* minimum fid seen, updated as fields are processed*/
               max_fid;   /* maximum fid seen */
  size_t       entry_count, /* cnt of field names, fids may map to same name */
               map_cnt,   /* count of enum maps */
               map_size;  /* size of all enum maps */

  static const size_t ENUM_HTSZ = 1024;
  MDDictEntry * enum_ht[ ENUM_HTSZ ];

  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  MDDictIdx() : dup_hash( 0 ), type_hash( 0 ), min_fid( 0 ), max_fid( 0 ),
                entry_count( 0 ), map_cnt( 0 ), map_size( 0 ) {
    ::memset( this->enum_ht, 0, sizeof( this->enum_ht ) );
  }
  ~MDDictIdx() noexcept;
  uint32_t file_lineno( const char *filename,  uint32_t lineno ) noexcept;
  uint32_t check_dup( const char *fname,  uint8_t fnamelen,
                      bool &xdup ) noexcept;
  size_t total_size( void ) noexcept;
  size_t fname_size( uint8_t &shft,  uint8_t &align ) noexcept;
  template< class TYPE > TYPE * alloc( size_t sz ) noexcept;
};

struct DictParser {
  DictParser * next;       /* list of files pushed after included */
  FILE       * fp;         /* fopen of file */
  const char * str_input;  /* when input from a string */
  size_t       str_size,   /* size left of input string */
               off,        /* index into buf[] where reading */
               len,        /* length of buf[] filled */
               tok_sz;     /* length of tok_buf[] token */
  int          tok,        /* token kind enumerated */
               lineno,     /* current line of input */
               col,        /* current column of input */
               br_level;   /* brace level == 0, none > 1 ( deep ) */
  bool         is_eof;     /* consumed all of file */
  char         buf[ 1024 ],        /* current input block */
               tok_buf[ 1024 ],    /* current token */
               fname[ 1024 ];      /* current file name */
  static const int EOF_CHAR = 256; /* int value signals end of file */
  const int    int_tok,    /* int value of token */
               ident_tok,  /* token enum for identifier */
               error_tok;  /* token enum for error */

  DictParser( const char *p,  int intk,  int identk,  int errk ) :
      next( 0 ), fp( 0 ), str_input( 0 ), str_size( 0 ), off( 0 ), len( 0 ),
      tok_sz( 0 ), col( 0 ), br_level( 0 ), is_eof( false ),
      int_tok( intk ), ident_tok( identk ), error_tok( errk ) {
    size_t len = 0;
    if ( p != NULL ) {
      len = ::strlen( p );
      if ( len > sizeof( this->fname ) - 1 )
        len = sizeof( this->fname ) - 1;
    }
    ::memcpy( this->fname, p, len );
    this->fname[ len ] = '\0';
    this->tok    = -1;
    this->lineno = 1;
  }
  void open( void ) {
    this->fp = fopen( this->fname, "r" );
  }
  void close( void ) {
    this->is_eof = true;
    if ( this->fp != NULL ) {
      ::fclose( this->fp );
      this->fp = NULL;
    }
  }
  bool fillbuf( void ) noexcept;
  bool get_char( size_t i,  int &c ) noexcept;
  int eat_white( void ) noexcept;
  void eat_comment( void ) noexcept;
  bool match( const char *s,  size_t sz ) noexcept;
  int consume_tok( int k,  size_t sz ) noexcept;
  int consume_int_tok( void ) noexcept;
  int consume_ident_tok( void ) noexcept;
  int consume_string_tok( void ) noexcept;
  static bool find_file( const char *path,  const char *filename,
                         size_t file_sz,  char *path_found ) noexcept;
};

}
}
#endif
