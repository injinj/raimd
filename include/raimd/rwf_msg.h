#ifndef __rai_raimd__rwf_msg_h__
#define __rai_raimd__rwf_msg_h__

#include <raimd/md_msg.h>
#include <raimd/omm_msg.h>

namespace rai {
namespace md {

bool rwf_type_size_to_md_type( uint8_t type,  MDType &ftype,  uint32_t &fsize ) noexcept;
bool rwf_primitive_to_md_type( uint8_t type,  MDType &ftype ) noexcept;
struct RwfMsg;
uint16_t rwf_to_sass_msg_type( RwfMsg &rwf ) noexcept;
uint16_t rwf_code_to_sass_rec_status( RwfMsg &rwf ) noexcept;

struct RwfFieldListSet {
  uint16_t count,
           set_id;
  struct FieldEntry {
    MDFid   fid;
    uint8_t type;
  } entry[ 1 ];
  static size_t alloc_size( uint16_t count ) {
    return sizeof( RwfFieldListSet ) +
           sizeof( FieldEntry ) * ( count - 1 );
  }
  uint8_t get_fid( uint32_t idx,  MDType &ftype,  uint32_t &fsize,  MDFid &fid ) {
    FieldEntry & e = this->entry[ idx ];
    fid = e.fid;
    if ( ! rwf_type_size_to_md_type( e.type, ftype, fsize ) )
      return 0;
    return e.type;
  }
};

enum RwfNextKind {
  RWF_NO_MORE          = 0,
  RWF_NORMAL_ENTRY     = 1,
  RWF_FIELD_DEFN_ENTRY = 2,
  RWF_FIRST            = 4,
  RWF_FIRST_ENTRY      = RWF_NORMAL_ENTRY | RWF_FIRST,
  RWF_FIRST_FIELD_DEFN = RWF_FIELD_DEFN_ENTRY | RWF_FIRST
};

struct RwfFieldListHdr : public RwfBase {
  enum {
    HAS_FIELD_LIST_INFO = 1,
    HAS_SET_DATA        = 2,
    HAS_SET_ID          = 4,
    HAS_STANDARD_DATA   = 8
  };
  uint8_t  flags, /* field list flags above */
           dict_id;
  int16_t  flist,
           set_id;
  uint32_t field_cnt;
  RwfFieldListSet * field_set;

  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  int next_kind( uint32_t field_idx ) const {
    if ( this->field_set == NULL ) {
      if ( field_idx >= this->field_cnt )
        return RWF_NO_MORE;
      return ( field_idx == 0 ? RWF_FIRST_ENTRY : RWF_NORMAL_ENTRY );
    }
    if ( field_idx >= this->field_cnt + this->field_set->count )
      return RWF_NO_MORE;
    if ( field_idx >= this->field_set->count )
      return ( field_idx == this->field_set->count ? RWF_FIRST_ENTRY : RWF_NORMAL_ENTRY );
    return ( field_idx == 0 ? RWF_FIRST_FIELD_DEFN : RWF_FIELD_DEFN_ENTRY );
  }
};

enum RwfMapAction {
  MAP_NONE         = 0,
  MAP_UPDATE_ENTRY = 1,
  MAP_ADD_ENTRY    = 2,
  MAP_DELETE_ENTRY = 3,
  MAP_SUMMARY      = 255
};
struct RwfMapHdr : public RwfBase {
  enum {
    HAS_SET_DEFS     = 1,
    HAS_SUMMARY_DATA = 2,
    HAS_PERM_DATA    = 4,
    HAS_COUNT_HINT   = 8,
    HAS_KEY_FID      = 16
  };
  uint8_t  flags, /* map flags above */
           key_type,
           container_type; /* container */
  int16_t  key_fid;      /* if has key fid */
  uint32_t hint_cnt,     /* if has count hint */
           entry_cnt,    /* actual count encoded */
           summary_size, /* summary container size */
           summary_start;/* start of summary */
  const char * key_fname; /* if key_fid is set */
  uint8_t  key_fnamelen;
  MDType   key_ftype;
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool has_next( uint32_t field_idx ) const {
    if ( field_idx >= ( this->entry_cnt + ( this->summary_size != 0 ) ) )
      return false;
    return true;
  }
};

struct RwfElementListSet {
  uint16_t count,
           set_id;
  struct ElemEntry {
    char   * name;
    uint16_t name_len;
    uint8_t  type;
  } entry[ 1 ];
  static size_t alloc_size( uint16_t count ) {
    return sizeof( RwfElementListSet ) +
           sizeof( ElemEntry ) * ( count - 1 );
  }
  bool get_elem( uint32_t idx,  MDType &ftype,  uint32_t &fsize,
                 const char *&fname,  uint16_t &fname_len ) {
    ElemEntry & e = this->entry[ idx ];
    fname     = e.name;
    fname_len = e.name_len;
    return rwf_type_size_to_md_type( e.type, ftype, fsize );
  }
};

struct RwfElementListHdr : public RwfBase {
  enum {
    HAS_ELEMENT_LIST_INFO = 1,
    HAS_SET_DATA          = 2,
    HAS_SET_ID            = 4,
    HAS_STANDARD_DATA     = 8
  };
  uint8_t  flags;
  uint16_t list_num,
           set_id;
  uint32_t item_cnt;
  RwfElementListSet * field_set;

  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  int next_kind( uint32_t field_idx ) const {
    if ( this->field_set == NULL ) {
      if ( field_idx >= this->item_cnt )
        return RWF_NO_MORE;
      return ( field_idx == 0 ? RWF_FIRST_ENTRY : RWF_NORMAL_ENTRY );
    }
    if ( field_idx >= this->item_cnt + this->field_set->count )
      return RWF_NO_MORE;
    if ( field_idx >= this->field_set->count )
      return ( field_idx == this->field_set->count ? RWF_FIRST_ENTRY : RWF_NORMAL_ENTRY );
    return ( field_idx == 0 ? RWF_FIRST_FIELD_DEFN : RWF_FIELD_DEFN_ENTRY );
  }
};

enum RwfFilterAction {
  FILTER_NONE         = 0,
  FILTER_UPDATE_ENTRY = 1,
  FILTER_SET_ENTRY    = 2,
  FILTER_CLEAR_ENTRY  = 3
};
struct RwfFilterListHdr : public RwfBase {
  enum {
    HAS_PERM_DATA  = 1,
    HAS_COUNT_HINT = 2
  };
  enum {
    ENTRY_HAS_PERM_DATA      = 1,
    ENTRY_HAS_CONTAINER_TYPE = 2
  };
  uint8_t  flags,
           container_type;
  uint32_t hint_cnt,
           item_cnt;
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool has_next( uint32_t field_idx ) const {
    if ( field_idx >= this->item_cnt )
      return false;
    return true;
  }
};

struct RwfSeriesHdr : public RwfBase {
  enum {
    HAS_SET_DEFS     = 1,
    HAS_SUMMARY_DATA = 2,
    HAS_COUNT_HINT   = 4
  };
  uint8_t  flags,
           container_type;
  uint32_t summary_size, /* summary container size */
           summary_start,/* start of summary */
           hint_cnt,
           item_cnt;
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool has_next( uint32_t field_idx ) const {
    if ( field_idx >= ( this->item_cnt + ( this->summary_size != 0 ) ) )
      return false;
    return true;
  }
};

enum RwfVectorAction {
  VECTOR_NONE         = 0,
  VECTOR_UPDATE_ENTRY = 1,
  VECTOR_SET_ENTRY    = 2,
  VECTOR_CLEAR_ENTRY  = 3,
  VECTOR_INSERT_ENTRY = 4,
  VECTOR_DELETE_ENTRY = 5,
  VECTOR_SUMMARY      = 255
};
struct RwfVectorHdr : public RwfBase {
  enum {
    HAS_SET_DEFS     = 1,
    HAS_SUMMARY_DATA = 2,
    HAS_PERM_DATA    = 4,
    HAS_COUNT_HINT   = 8,
    SUPPORTS_SORTING = 16
  };
  uint8_t  flags,
           container_type;
  uint32_t summary_size,
           summary_start,
           hint_cnt,
           item_cnt;
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool has_next( uint32_t field_idx ) const {
    if ( field_idx >= ( this->item_cnt + ( this->summary_size != 0 ) ) )
      return false;
    return true;
  }
};

static const uint32_t RWF_FIELD_LIST_TYPE_ID   = 0x25cdabca,
                      RWF_MAP_TYPE_ID          = 0x0d20015c,
                      RWF_ELEMENT_LIST_TYPE_ID = 0xfb5beb1f,
                      RWF_FILTER_LIST_TYPE_ID  = 0xb3685beb,
                      RWF_SERIES_TYPE_ID       = 0x0e396966,
                      RWF_VECTOR_TYPE_ID       = 0x5c8b2bf3,
                      RWF_MSG_TYPE_ID          = 0xd13463b7,
                      RWF_MSG_KEY_TYPE_ID      = 0xa5a60b23;
struct MDDictBuild;
struct RwfMsg : public MDMsg {
  union {
    RwfBase           base;
    RwfFieldListHdr   fields;
    RwfMapHdr         map;
    RwfElementListHdr elist;
    RwfFilterListHdr  flist;
    RwfSeriesHdr      series;
    RwfVectorHdr      vector;
    RwfMsgHdr         msg;
    RwfMsgKey         msg_key;
  };
  RwfMsg * parent;
  void * operator new( size_t, void *ptr ) { return ptr; }

  RwfMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, end, d, m ), parent( 0 ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  RwfMsg * unpack_sub_msg( uint8_t type,  size_t start,  size_t end ) noexcept;
  RwfMsg * get_msg_key_attributes( void ) noexcept;
  RwfMsg * get_container_msg( void ) noexcept;
  RwfMsg * get_summary_msg( void ) noexcept;
  virtual int get_sub_msg( MDReference &mref,  MDMsg *&msg,
                           MDFieldIter *iter ) noexcept final;
  bool get_field_defn_db( void ) noexcept;
  virtual int get_array_ref( MDReference &mref, size_t i,
                             MDReference &aref ) noexcept;
  static bool is_rwf_field_list( void *bb,  size_t off,  size_t end,
                                 uint32_t h ) noexcept;
  static bool is_rwf_map( void *bb,  size_t off,  size_t end,
                          uint32_t h ) noexcept;
  static bool is_rwf_element_list( void *bb,  size_t off,  size_t end,
                                   uint32_t h ) noexcept;
  static bool is_rwf_filter_list( void *bb,  size_t off,  size_t end,
                                  uint32_t h ) noexcept;
  static bool is_rwf_series( void *bb,  size_t off,  size_t end,
                             uint32_t h ) noexcept;
  static bool is_rwf_vector( void *bb,  size_t off,  size_t end,
                             uint32_t h ) noexcept;
  static bool is_rwf_message( void *bb,  size_t off,  size_t end,
                              uint32_t h ) noexcept;
  static bool is_rwf_msg_key( void *bb,  size_t off,  size_t end,
                              uint32_t h ) noexcept;

  static RwfMsg *unpack_field_list( void *bb,  size_t off,  size_t end,
                                uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_map( void *bb,  size_t off,  size_t end,  uint32_t h,
                             MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_element_list( void *bb,  size_t off,  size_t end,
                                uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_filter_list( void *bb,  size_t off,  size_t end,
                                uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_series( void *bb,  size_t off,  size_t end,
                                uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_vector( void *bb,  size_t off,  size_t end,
                                uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_message( void *bb,  size_t off,  size_t end,
                                 uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static RwfMsg *unpack_msg_key( void *bb,  size_t off,  size_t end,
                                 uint32_t h,  MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
  static int decode_field_dictionary( MDDictBuild &dict_build,
                                      RwfMsg &series ) noexcept;
  static int decode_enum_dictionary( MDDictBuild &dict_build,
                                     RwfMsg &series ) noexcept;
};

struct RwfMapIter {
  uint8_t      flags;
  RwfMapAction action;
  uint16_t     keylen;
  void       * key;
  RwfPerm      perm;
};
struct RwfSeriesIter {
  bool is_summary;
};
struct RwfVectorIter {
  uint8_t         flags;
  RwfVectorAction action;
  uint32_t        index;
  RwfPerm         perm;
};
struct RwfFilterListIter {
  uint8_t         flags,
                  id,
                  type;
  RwfFilterAction action;
  RwfPerm         perm;
};
struct RwfFieldListIter {
  MDFid        fid;
  const char * fname;
  uint16_t     fnamelen;
  uint8_t      rwf_type;
};
struct RwfElementListIter {
  uint8_t      type;
  uint16_t     namelen;
  const char * name;
};
struct RwfMessageIter {
  const char * name;
  uint16_t     namelen;
  uint8_t      field_count;
  uint8_t      index[ 24 ]; /* fields that have a flags bit set */
};
struct RwfMsgKeyIter {
  const char * name;
  uint16_t     namelen;
  uint8_t      field_count;
  uint8_t      index[ 24 ]; /* shouldn't be more than 17 */
};

struct RwfFieldIter : public MDFieldIter {
  MDType       ftype;     /* field type from dictionary */
  uint32_t     fsize,     /* length of string, size of int */
               field_idx; /* current field index */
  uint16_t     position;  /* partial offset / escape position */
  MDValue      val;       /* union of temp values */
  MDDecimal    dec;       /* if price */
  MDTime       time;      /* if time field */
  MDDate       date;      /* if data field */
  size_t       data_start;/* position after header where data starts */
  uint8_t    * msg_fptr;  /* the interator field fptr when field is in header */

  template<class T>
  void set_uint( T &uval ) {
    this->ftype    = MD_UINT;
    this->msg_fptr = (uint8_t *) &uval;
    this->fsize    = sizeof( T );
  }
  template<class T>
  void set_opaque( T &opaque ) {
    this->ftype    = MD_OPAQUE;
    this->msg_fptr = (uint8_t *) opaque.buf;
    this->fsize    = opaque.len;
  }
  void set_string( const char *p,  size_t len ) {
    this->ftype    = MD_STRING;
    this->msg_fptr = (uint8_t *) p;
    this->fsize    = len;
  }
  void set_string( const char *p ) {
    this->set_string( p, ::strlen( p ) );
  }
  void alloc_string( const char *p ) {
    this->set_string( p, ::strlen( p ) );
    this->msg_fptr = (uint8_t *)
      this->iter_msg.mem->stralloc( this->fsize, (char *) this->msg_fptr );
  }
  union {
    RwfFieldListIter   field;
    RwfMapIter         map;
    RwfSeriesIter      series;
    RwfFilterListIter  flist;
    RwfElementListIter elist;
    RwfVectorIter      vector;
    RwfMessageIter     msg;
    RwfMsgKeyIter      msg_key;
  } u;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RwfFieldIter( MDMsg &m ) : MDFieldIter( m ),
      ftype( MD_NODATA ), fsize( 0 ), field_idx( 0 ), position( 0 ),
      data_start( 0 ), msg_fptr( 0 ) {
    ::memset( &this->u, 0, sizeof( this->u ) );
  }

  void lookup_fid( void ) noexcept;
  virtual int get_name( MDName &name ) noexcept final;
  virtual int get_enum( MDReference &mref,  MDEnum &enu ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  int get_real_ref( MDReference &mref ) noexcept;
  int decode_ref( MDReference &mref ) noexcept;
  virtual int get_hint_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack_field_list_entry( void ) noexcept;
  int unpack_field_list_defn( void ) noexcept;
  int unpack_map_entry( void ) noexcept;
  int unpack_filter_list_entry( void ) noexcept;
  int unpack_element_list_entry( void ) noexcept;
  int unpack_element_list_defn( void ) noexcept;
  int unpack_series_entry( void ) noexcept;
  int unpack_vector_entry( void ) noexcept;
  int unpack_message_entry( void ) noexcept;
  int unpack_msg_key_entry( void ) noexcept;
};

int8_t rwf_to_md_decimal_hint( uint8_t hint ) noexcept;
uint8_t md_to_rwf_decimal_hint( int8_t hint ) noexcept;
uint8_t md_type_to_rwf_primitive_type( MDType type ) noexcept;

}
} // namespace rai

#include <raimd/rwf_writer.h>

#endif
