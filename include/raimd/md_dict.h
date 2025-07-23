#ifndef __rai_raimd__md_dict_h__
#define __rai_raimd__md_dict_h__

#include <raimd/md_types.h>
#include <raimd/md_hash_tab.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  MD_HAS_RIPPLE = 1 << 4, /* flags */
  MD_HAS_NAME   = 1 << 5
};

typedef struct MDLookup_s {
  const char * fname;
  MDFid        fid;
  uint32_t     fsize;
  MDType       ftype;
  uint8_t      fname_len,
               flags,
               rwflen,
               mflen,
               enumlen;
  uint16_t     map_num;
} MDLookup_t;

bool md_lookup_create_by_fid( MDLookup_t **p, MDFid fid );
bool md_lookup_create_by_name( MDLookup_t **p, const char *fn, size_t fn_len );
void md_lookup_destroy( MDLookup_t *p );
void md_lookup_init_by_fid( MDLookup_t *p, MDFid fid );
void md_lookup_init_by_name( MDLookup_t *p, const char *fn, size_t fn_len );
void md_lookup_mf_type( MDLookup_t *p, uint8_t *mf_type, uint32_t *mf_len, uint32_t *enum_len );
void md_lookup_rwf_type( MDLookup_t *p, uint8_t *rwf_type, uint32_t *rwf_len );
int md_lookup_get_name_ripple( MDLookup_t *p, const char **name, uint8_t *name_len, const char **ripple, uint8_t *ripple_len );

typedef struct MDFormEntry_s {
  uint16_t fid,
           foffset;
} MDFormEntry_t;

typedef struct MDDict_s {
  struct MDDict_s * next;  /* list of multiple dictionaries)*/
  char   dict_type[ 8 ];   /* cfile or RDM(app_a) */
} MDDict_t;

typedef struct MDFormClass_s MDFormClass_t;

uint32_t md_dict_hash( const char *key, size_t len );
bool md_dict_equals( const char *fname,  size_t len, const char *fname2,  size_t len2 );
bool md_dict_lookup( const MDDict_t *dict, MDLookup_t *p );
bool md_dict_get( const MDDict_t *dict, MDLookup_t *p );
bool md_dict_get_enum_text( const MDDict_t *dict, MDFid fid, uint16_t val, const char **disp, size_t *disp_len );
bool md_dict_get_enum_map_text( const MDDict_t *dict, uint32_t map_num, uint16_t val, const char **disp, size_t *disp_len );
bool md_dict_get_enum_val( const MDDict_t *dict, MDFid fid, const char *disp, size_t disp_len, uint16_t *val );
bool md_dict_get_enum_map_val( const MDDict_t *dict, uint32_t map_num, const char *disp, size_t disp_len, uint16_t *val );
MDFormClass_t *md_dict_get_form_class( const MDDict_t *dict, MDLookup_t *fc );

#ifdef __cplusplus
}

namespace rai {
namespace md {

/* The MDDict is a fid -> fname / type map, using 4 tables (+enums):
 *   * type table,  uint32_t size,
 *       Each element indictes size, flags, and type of field,
 *       The lower 5 bits is the type, the middle 4 are th flags,
 *       the upper 23 are the size.
 *         el[ type_id ] = md_dict_hash_compose( ftype, fsize_ flags )
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

enum {
  MD_HAS_RIPPLE = 1 << 4, /* flags */
  MD_HAS_NAME   = 1 << 5
};

struct MDLookup : public MDLookup_s {
  MDLookup() {}
  MDLookup( MDFid f ) { this->id( f ); }
  MDLookup( const char *fn,  size_t fn_len ) { this->nm( fn, fn_len ); }
  MDLookup &id( MDFid f ) { this->fid = f; return *this; }
  MDLookup &nm( const char *fn,  size_t fn_len ) {
    this->fname = fn; this->fname_len = fn_len; return *this; }
  void mf_type( uint8_t &mf_type,  uint32_t &mf_len,
                uint32_t &enum_len ) noexcept;
  void rwf_type( uint8_t &rwf_type,  uint32_t &rwf_len ) noexcept;

  int get_name_ripple( const char *&name,  uint8_t &namelen,
                       const char *&ripple,  uint8_t &ripplelen ) {
    name      = NULL;
    ripple    = NULL;
    namelen   = 0;
    ripplelen = 0;

    bool has_ripple = ( this->flags & MD_HAS_RIPPLE ) != 0,
         has_name   = ( this->flags & MD_HAS_NAME ) != 0;

    if ( ( has_name | has_ripple ) == true ) {
      namelen = (uint8_t) this->fname[ this->fname_len ];
      name    = &this->fname[ this->fname_len + 1 ];
      if ( ( has_name & has_ripple ) == true ) {
        ripplelen = (uint8_t) name[ namelen ];
        ripple    = &name[ namelen + 1 ];
        return 2;
      }
      if ( ! has_name ) {
        ripplelen = namelen; namelen = 0;
        ripple    = name;    name    = NULL;
      }
      return 1;
    }
    return 0;
  }
};

static const int MF_LEN_BITS  = 8,
                 RWF_LEN_BITS = 8;
struct MDTypeHashBits {
  union {
    uint64_t val;
    struct {
      uint64_t fsize   : 20, /* size, overrides below when larger than 255 */
               rwflen  : 8,  /* type encode length */
               mflen   : 8,  /* digit length, string len */
               enumlen : 5,  /* string length of enum */
               map_num : 12, /* which enum table */
               flags   : 6,  /* type mapping quirks */
               ftype   : 5;  /* md type */
    } b;
  };
};

inline void md_dict_hash_decompose( uint64_t v,  MDLookup &by ) {
  MDTypeHashBits x;
  x.val = v;
  by.fsize   = x.b.fsize;
  by.rwflen  = x.b.rwflen;
  by.mflen   = x.b.mflen;
  by.enumlen = x.b.enumlen;
  by.map_num = x.b.map_num;
  by.flags   = x.b.flags;
  by.ftype   = (MDType) x.b.ftype;
}

struct MDEnumMap;
struct MDFormMap;
struct MDDict;
struct MDFormEntry : public MDFormEntry_s {};

struct MDFormKey {
  MDFid fid;
  static size_t hash( MDFid fid ) noexcept;
  size_t hash( void ) const { return hash( this->fid ); }
  bool equals( const MDFormKey &k ) const { return this->fid == k.fid; }
  MDFormKey( MDFid f ) : fid( f ) {}
};
struct MDFormClass : public MDFormKey {
  MDDict      & dict;
  MDFormMap   & map;
  MDFormEntry * entries;
  uint16_t    * ht;
  uint32_t      nentries,
                htsize,
                form_size;
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  MDFormClass( MDFid fid, MDDict &d,  MDFormMap &m,  MDFormEntry *e,
               uint16_t *h,  uint32_t n,  uint32_t hsz )
    : MDFormKey( fid ), dict( d ), map( m ), entries( e ), ht( h ),
      nentries( n ), htsize( hsz ), form_size( 0 ) {}
  static MDFormClass *make_form_class( MDDict &d,  MDFid fid,
                                       MDFormMap &map ) noexcept;
  const MDFormEntry *lookup( MDLookup &by ) const;
  const MDFormEntry *get( MDLookup &by ) const;
  const MDFormEntry *get_entry( MDFid fid ) const;
};

typedef struct MDHashTabT<MDFormKey, MDFormClass> MDFormClassHT;

struct MDDict : public MDDict_s {
  /*
   * [ type table (uint32 elem) ]  = offset 0 (sizeof(MDDict))
   * [ fid table (tab bits elem) ] = offset tab_off (+ntypes * uint32)
   * [ fname table (string elem) ] = offset fname_off (+fid_bits * nfids)
   * [ fname ht (tab bits ht) ]    = offset ht_off (+fnamesz ht_off - fname_off)
   * [ enum off (uint32_t off) ]   = offset map_off (+fid_bits * ht_size)
   */
  MDFormClassHT form_class_ht; /* form classes, created on demand */
  MDFid         min_fid,     /* minumum fid indexed */
                max_fid;     /* maximum fid indexed, inclusive */
  uint32_t      tab_off,     /* offset of bit array table */
                tab_size,    /* size in bytes of bit array table */
                ht_off,      /* offset of fname hash */
                ht_size,     /* size in fids of ht */
                entry_count, /* number of fids in the table */
                fname_off,   /* offset of fname start */
                map_off,     /* offset of enum map index */
                map_count,   /* count of enum maps */
                form_off,    /* offset of form map index */
                form_count,  /* count of forms */
                tag_off,     /* offset of tags */
                dict_size;   /* sizeof this + tables */
  uint8_t       type_shft,   /* how many bits used for the type */
                fname_shft,  /* how many bits used for fname offset */
                fname_algn,  /* what byte alignment used for fname */
                tab_bits,    /* = type_shft + fname_shft - fname_algn*/
                fid_bits,    /* number of bits needed for a fid - min_fid + 1 */
                pad[ 3 ];    /* tag strings */

  MDDict * get_next( void ) const { return (MDDict *) this->next; }
  /* used by get() below */
  static uint32_t dict_hash( const char *key,  size_t len ) noexcept;
  /* equals allow nul termination in the fnames */
  static bool     dict_equals( const char *fname,  size_t len,
                               const char *fname2,  size_t len2 ) noexcept;
  /* lookup fid, return {type, size, name/len} if found */
  bool lookup( MDLookup &by ) const {
    if ( by.fid < this->min_fid || by.fid > this->max_fid )
      return false;

    uint32_t bits = this->tab_bits, /* fname offset + type bits */
             mask = ( 1U << bits ) - 1,
             i    = ( (uint32_t) by.fid - (uint32_t) this->min_fid ) * bits,
             off  = i / 8,  /* byte offset for fid entry */
             shft = i % 8,  /* shift from byte offset */
             fname_bits = ( this->fname_shft - this->fname_algn ),
             fname_mask = ( 1U << fname_bits ) - 1,
             type_idx,
             fname_idx;
    uint64_t val;   /* bits at offset copied into val */
    /* get the bits assoc with fid at bit index tab[ ( fid-min_fid ) * bits ] */
    const uint8_t * tab = &((const uint8_t *) (void *) this)[ this->tab_off ];
    val = ( (uint64_t) tab[ off ] ) |
          ( (uint64_t) tab[ off + 1 ] << 8 ) |
          ( (uint64_t) tab[ off + 2 ] << 16 ) |
          ( (uint64_t) tab[ off + 3 ] << 24 ) |
          ( (uint64_t) tab[ off + 4 ] << 32 );
    for ( off += 5; off * 8 < bits + shft; off++ )
      val |= (uint64_t) tab[ off ] << ( off * 8 );
    /* the type of the fid is first, then the fname index */
    type_idx  = ( val >> shft ) & mask;
    fname_idx = ( type_idx & fname_mask ) << this->fname_algn;
    type_idx  = type_idx >> fname_bits;
    if ( fname_idx == 0 ) /* first slot empty */
      return false;
    /* get the type/size using the type_idx from the type table */
    const uint64_t * ttab = (const uint64_t *) (void *) &this[ 1 ];
    md_dict_hash_decompose( ttab[ type_idx ], by );
    /* get the field name */
    const uint8_t * fntab =
      &((const uint8_t *) (void *) this)[ this->fname_off ];
    by.fname_len = fntab[ fname_idx ];
    by.fname     = (const char *) &fntab[ fname_idx + 1 ];

    return true;
  }

  /* get a field by name, return {fid, type, size} if found */
  bool get( MDLookup &by ) const {
    const char * fname     = by.fname;
    uint8_t      fname_len = by.fname_len;
    uint32_t h = dict_hash( fname, fname_len ) & ( this->ht_size - 1 );

    uint32_t bits = this->fid_bits, /* size in bits of each hash entry */
             mask = ( 1U << bits ) - 1, /* mask for bits */
             val;
    /* the hash table */
    const uint8_t * tab = &((const uint8_t *) (void *) this)[ this->ht_off ];
    /* scan the linear hash table until found or zero (not found) */
    for ( ; ; h = ( h + 1 ) & ( this->ht_size - 1 ) ) {
      uint32_t i    = h * bits,
               off  = i / 8,
               shft = i % 8;

      val = ( (uint64_t) tab[ off ] ) |
            ( (uint64_t) tab[ off + 1 ] << 8 ) |
            ( (uint64_t) tab[ off + 2 ] << 16 ) |
            ( (uint64_t) tab[ off + 3 ] << 24 ) |
            ( (uint64_t) tab[ off + 4 ] << 32 );
      val = ( val >> shft ) & mask;
      if ( val == 0 ) /* if end of hash chain */
        return false;
      val = val + this->min_fid - 1;
      /* val is the fid contained at this position */
      by.fid = val;
      if ( this->lookup( by ) ) {
        if ( dict_equals( fname, fname_len, by.fname, by.fname_len ) )
          return true;
/*        if ( fname_len == by.fname_len &&
             ::memcmp( fname, by.fname, fname_len ) == 0 )
          return true;*/
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
  MDEnumMap *get_enum_map( uint32_t map_num ) noexcept;
  MDFormClass *get_form_class( MDFid fid ) noexcept;
  MDFormClass *get_form_class( MDLookup &by ) noexcept;
  MDFormMap *get_form_map( uint32_t map_num ) noexcept;
  bool first_tag( const char *&tag,  size_t &tag_sz ) noexcept;
  bool next_tag( const char *&tag,  size_t &tag_sz ) noexcept;
  bool find_tag( const char *name,  const char *&val, size_t &val_sz ) noexcept;
};

enum MDDictDebugFlags {
  MD_DICT_NONE = 0,
  MD_DICT_PRINT_FILES = 1
};

/* The following structures are used to build the MDDict, only used when
 * building the index */
struct MDDictIdx;
struct MDDictEntry;
struct MDFormBuild;

struct MDDictAdd {
  MDFid         fid;      /* fid -> fname map */
  MDType        ftype;    /* type of fid */
  uint8_t       flags,    /* type translations */
                mf_type,  /* marketfeed type */
                rwf_type; /* rwf type */
  uint32_t      fsize,    /* sass size */
                mf_len,   /* marketfeed len */
                rwf_len,  /* rwf length */
                enum_len; /* enum str len */
  const char  * fname,    /* acro */
              * name,     /* acro_dde */
              * ripple,   /* ripples_to */
              * filename; /* filename declared */
  uint32_t      lineno;   /* line of filename */
  MDFid         ripplefid;
  MDDictEntry * entry;    /* entry found or created */

  MDDictAdd() {
    ::memset( (void *) this, 0, sizeof( *this ) );
  }
};

struct MDEnumAdd {
  uint32_t   map_num,   /* add a enum at map_num */
             value_cnt; /* count of value[] */
  uint16_t   max_value, /* max_value ? */
             max_len;   /* length of enum string */
  uint16_t * value;     /* integer mappings */
  uint8_t  * map;       /* array of strings */

  MDEnumAdd() {
    ::memset( (void *) this, 0, sizeof( *this ) );
  }
};

struct MDDictBuild {
  MDDictIdx * idx;
  int         debug_flags;

  MDDictBuild() : idx( 0 ), debug_flags( 0 ) {}
  ~MDDictBuild() noexcept;

  MDDictIdx *get_dict_idx( void ) noexcept;
  int add_entry( MDDictAdd &a ) noexcept;
  int add_enum_map( MDEnumAdd &a ) noexcept;
  int add_form_build( MDFormBuild &fb ) noexcept;
  int update_entry_enum( MDFid fid,  uint32_t map_num,
                         uint16_t enum_len ) noexcept;
  int update_entry_form( MDFid fid,  uint32_t map_num ) noexcept;
  void add_tag( const char *tag,  uint32_t tag_sz ) noexcept;
  void add_tag( const char *name,  uint32_t name_sz,
                const char *val,  uint32_t val_sz ) noexcept;
  int index_dict( const char *dtype,  MDDict *&dict ) noexcept;
  void clear_build( void ) noexcept;

  int add_rwf_entry( const char *acro,  const char *acro_dde,
                     MDFid fid,  MDFid ripplefid,  uint8_t mf_type,
                     uint32_t length,  uint32_t enum_len,
                     uint8_t rwf_type,  uint32_t rwf_len,
                     const char *filename,  uint32_t lineno ) noexcept;
  void add_rwf_enum_map( int16_t *fids,  uint32_t num_fids,
                         uint16_t *values,  uint16_t num_values,
                         char *display,  uint32_t disp_len,
                         uint32_t map_num ) noexcept;
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
  uint32_t      rwf_len,   /* size of field rwf data (binary coded) */
                map_num,   /* enum map num */
                tt_pos;
  char          buf[ 4 ]; /* fname, name, ripple */
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDDictEntry() : next( 0 ), fid( 0 ), fsize( 0 ), fno( 0 ), hash( 0 ),
    ftype( MD_NODATA ), fnamelen( 0 ), namelen( 0 ),
    ripplelen( 0 ), enum_len( 0 ), mf_type( 0 ), rwf_type( 0 ),
    fld_flags( 0 ), mf_len( 0 ), rwf_len( 0 ), map_num( 0 ), tt_pos( 0 ) {}
  const char *fname( void ) const {
    return this->buf;
  }
  const char *name( void ) const {
    return &(this->fname())[ this->fnamelen ];
  }
  const char *ripple( void ) const {
    return &(this->name())[ this->namelen ];
  }
  void update( MDDictAdd &a,  uint32_t h ) noexcept;
};

struct MDTypeHash {
  uint32_t htshft,  /* hash of types used by fields, tab size = 1 << htshft */
           htcnt;   /* count of elements used */
  uint64_t ht[ 1 ]; /* each element is ( size << 5 ) | field type */
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDTypeHash() : htshft( 0 ), htcnt( 0 ) {}
  static uint32_t alloc_size( uint32_t shft ) {
    uint32_t htsz = ( 1U << shft );
    return sizeof( MDTypeHash ) + ( ( htsz - 1 ) * sizeof( uint64_t ) );
  }
  void init( uint32_t shft ) {
    this->htshft = shft;
    ::memset( this->ht, 0, sizeof( uint64_t ) * this->htsize() ); /* zero is empty */
  }
  uint32_t htsize( void ) const {
    return ( 1U << this->htshft );
  }
};

inline uint64_t md_dict_hash_compose( MDDictEntry &entry ) {
  MDTypeHashBits x;
  x.val = 0;
  x.b.fsize   = entry.fsize;
  x.b.rwflen  = entry.rwf_len;
  x.b.mflen   = entry.mf_len;
  x.b.enumlen = entry.enum_len;
  x.b.map_num = entry.map_num;
  x.b.flags   = entry.fld_flags;
  x.b.ftype   = (uint32_t) entry.ftype;
  return x.val;
}

struct MDFilename {
  MDFilename * next;
  uint32_t     id;  /* filename + line number, location for dict errors */
  char         filename[ 4 ];
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDFilename() : next( 0 ), id( 0 ) {}
};

static inline size_t
md_align( size_t sz,  size_t val ) {
  return ( val + sz - 1 ) & ~( sz - 1 );
}

struct MDEnumMap {
  uint32_t map_num,   /* map type number */
           value_cnt; /* count of values, n -> max_value+1 */
  uint16_t max_value, /* enum from 0 -> max_value, inclusive */
           max_len;   /* size of the enum value, same for each entry */

  static size_t map_sz( size_t value_cnt,  size_t max_value,  size_t max_len ) {
    size_t sz = md_align( 4, max_len * value_cnt );
    if ( value_cnt == max_value + 1 )
      return sz;
    return sz + sizeof( uint16_t ) * md_align( 2, value_cnt );
  }
  size_t map_sz( void ) const {
    return map_sz( this->value_cnt, this->max_value, this->max_len );
  }
  uint16_t *value( void ) {
    if ( this->value_cnt == (uint32_t) this->max_value + 1 )
      return NULL;
    return (uint16_t *) (void *) &this[ 1 ];
  }
  uint8_t *map( void ) {
    uint32_t off = ( ( this->value_cnt == (uint32_t) this->max_value + 1 ) ?
                     0 : md_align( 2, this->value_cnt ) );
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

struct MDPendingEnum {
  MDPendingEnum * next;
  MDFid           fid;
  uint32_t        map_num;
  uint16_t        enum_len;
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDPendingEnum() : next( 0 ) {}
};

struct MDFormMap {
  uint32_t map_num,   /* map type number */
           code_size;

  static size_t map_sz( size_t code_size ) {
    return sizeof( uint16_t ) * md_align( 2, code_size );
  }
  size_t map_sz( void ) const {
    return map_sz( this->code_size );
  }
  uint16_t *code( void ) {
    uint16_t * code_cp = (uint16_t *) (void *) &this[ 1 ];
    return code_cp;
  }
  uint32_t fid_count( void ) noexcept;
  uint32_t get_fids( uint16_t *fids ) noexcept;
  bool     fid_is_member( uint16_t fid ) noexcept;
};

struct MDFormList {
  MDFormList * next;
  MDFormMap    map;
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDFormList() : next( 0 ) {}
};

struct MDTag {
  MDTag  * next;
  uint32_t len;
  char     val[ 4 ];
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDTag() : next( 0 ), len( 0 ) {}
};

struct MDDictIdx {
  MDQueue< MDEntryData >   data_q;  /* data buffers */
  MDQueue< MDDictEntry >   entry_q; /* list of all fields */
  MDQueue< MDEnumList >    enum_q;  /* list of all enum mappings */
  MDQueue< MDPendingEnum > pending_enum_q;  /* list of enums not yet mapped */
  MDQueue< MDFilename >    file_q;  /* list of all dict files */
  MDQueue< MDTag >         tag_q;
  MDQueue< MDFormList >    form_q;

  MDTypeHash * type_hash; /* hash of all fid types */
  MDFid        min_fid,   /* minimum fid seen, updated as fields are processed*/
               max_fid;   /* maximum fid seen */
  size_t       entry_count, /* cnt of field names, fids may map to same name */
               map_cnt,   /* count of enum maps */
               map_size,  /* size of all enum maps */
               form_cnt,  /* count of all forms */
               form_size; /* size of all forms */

  MDDictEntry ** fid_index; /* index by fids */
  size_t         fid_index_size;
  MDDictEntry ** fname_index; /* index by fname */
  size_t         fname_index_size;

  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  MDDictIdx() : type_hash( 0 ), min_fid( 0 ), max_fid( 0 ),
                entry_count( 0 ), map_cnt( 0 ), map_size( 0 ),
                form_cnt( 0 ), form_size( 0 ), fid_index( 0 ),
                fid_index_size( 0 ), fname_index( 0 ), fname_index_size( 0 ) {}
  ~MDDictIdx() noexcept;
  uint32_t file_lineno( const char *filename,  uint32_t lineno ) noexcept;
  size_t total_size( void ) noexcept;
  MDDictEntry * get_fid_entry( MDFid fid ) noexcept;
  MDDictEntry * get_fname_entry( const char *fname,  size_t fnamelen,
                                 uint32_t h ) noexcept;
  void add_fname_entry( MDDictEntry *entry ) noexcept;
  template< class TYPE > TYPE * alloc( size_t sz ) noexcept;
  void type_hash_insert( MDDictEntry &entry ) noexcept;
  uint32_t type_hash_find( MDDictEntry &entry ) noexcept;
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
               error_tok,  /* token enum for error */
               debug_flags;/* verbosity of parser debugging */
  const char * dict_kind;  /* what kind of dict this is */

  DictParser( const char *p,  int intk,  int identk,  int errk,  int fl,
              const char *k ) :
      next( 0 ), fp( 0 ), str_input( 0 ), str_size( 0 ), off( 0 ), len( 0 ),
      tok_sz( 0 ), col( 0 ), br_level( 0 ), is_eof( false ),
      int_tok( intk ), ident_tok( identk ), error_tok( errk ),
      debug_flags( fl ), dict_kind( k ) {
    size_t len = 0;
    if ( p != NULL ) {
      len = ::strlen( p );
      if ( len > sizeof( this->fname ) - 1 )
        len = sizeof( this->fname ) - 1;
      ::memcpy( this->fname, p, len );
    }
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
  size_t match_tag( const char *s,  size_t sz ) noexcept;
  int consume_tok( int k,  size_t sz ) noexcept;
  int consume_int_tok( void ) noexcept;
  int consume_ident_tok( void ) noexcept;
  int consume_string_tok( void ) noexcept;
  static bool find_file( const char *path,  const char *filename,
                         size_t file_sz,  char *path_found ) noexcept;
};

struct MDFormBuild {
  uint16_t fids[ 4 * 1024 ],
           code[ 4 * 1024 ];
  uint32_t nfids,
           code_size,
           map_num;

  MDFormBuild() : nfids( 0 ), code_size( 0 ), map_num( 0 ) {}

  bool add( uint16_t fid ) {
    if ( this->nfids == sizeof( this->fids ) / sizeof( this->fids[ 0 ] ) )
      return false;
    this->fids[ this->nfids++ ] = fid;
    return true;
  }
  bool compress( void ) noexcept;
};

}
}
#endif
#endif
