#ifndef __rai_raimd__rv_msg_h__
#define __rai_raimd__rv_msg_h__

#include <raimd/md_msg.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t RVMSG_TYPE_ID = 0xebf946be;
#ifndef __cplusplus
#define RVMSG_TYPE_ID 0xebf946beU
#endif

MDMsg_t * rv_msg_unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict_t *d,  MDMsgMem_t *m );
MDMsgWriter_t * rv_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,
                                      void *buf_ptr, size_t buf_sz );
#ifdef __cplusplus
}

namespace rai {
namespace md {

struct RvMsg : public MDMsg {
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg,
                           MDFieldIter *iter ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  virtual int get_array_ref( MDReference &mref,  size_t i,
                             MDReference &aref ) noexcept;
  virtual int time_to_string( MDReference &mref,  char *&buf,
                              size_t &len ) noexcept final;
  virtual int xml_to_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept;
  /* may return tibmsg, sass qform or rv */
  static bool is_rvmsg( void *bb,  size_t off,  size_t end,
                        uint32_t h ) noexcept;
  static RvMsg *unpack_rv( void *bb,  size_t off,  size_t end,  uint32_t h,
                           MDDict *d,  MDMsgMem &m ) noexcept;
  static MDMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
  static MDMsg *opaque_extract( uint8_t *bb,  size_t off,  size_t end,
                                MDDict *d,  MDMsgMem &m ) noexcept;
};

struct RvFieldIter : public MDFieldIter {
  uint32_t size;
  uint8_t  type,
           name_len;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvFieldIter( MDMsg &m )
    : MDFieldIter( m ), size( 0 ), type( 0 ), name_len( 0 ) {}

  void dup_rv( RvFieldIter &i ) {
    i.size     = this->size;
    i.type     = this->type;
    i.name_len = this->name_len;
    this->MDFieldIter::dup_iter( i );
  }
  bool is_named( const char *name,  size_t name_len ) noexcept;
  virtual MDFieldIter *copy( void ) noexcept final;
  virtual int get_name( MDName &name ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  virtual int update( MDReference &mref ) noexcept final;
  int unpack( void ) noexcept;
};

enum {
  RV_BADDATA   = 0,
  RV_RVMSG     = 1,
  RV_SUBJECT   = 2,
  RV_DATETIME  = 3,
  RV_OPAQUE    = 7,
  RV_STRING    = 8,
  RV_BOOLEAN   = 9,
  RV_IPDATA    = 10,  /* 0a */
  RV_INT       = 11,  /* 0b */
  RV_UINT      = 12,  /* 0c */
  RV_REAL      = 13,  /* 0d */
  RV_ENCRYPTED = 32,  /* 20 */
  RV_ARRAY_I8  = 34,  /* 22 */
  RV_ARRAY_U8  = 35,  /* 23 */
  RV_ARRAY_I16 = 36,  /* 24 */
  RV_ARRAY_U16 = 37,  /* 25 */
  RV_ARRAY_I32 = 38,  /* 26 */
  RV_ARRAY_U32 = 39,  /* 27 */
  RV_ARRAY_I64 = 40,  /* 28 */
  RV_ARRAY_U64 = 41,  /* 29 */
  RV_ARRAY_F32 = 44,  /* 2c */
  RV_ARRAY_F64 = 45,  /* 2d */
  RV_XML       = 47,  /* 2f */
  RV_ARRAY_STR = 48,  /* 30 */
  RV_ARRAY_MSG = 49
};

static const uint8_t RV_TINY_SIZE  = 120; /* 78 */
static const uint8_t RV_SHORT_SIZE = 121; /* 79 */
static const uint8_t RV_LONG_SIZE  = 122; /* 7a */
static const size_t MAX_RV_SHORT_SIZE = 0x7530; /* 30000 */

static inline size_t
rv_size_bytes( uint32_t fsize )
{
  if ( fsize < RV_TINY_SIZE )
    return 1;
  if ( fsize < MAX_RV_SHORT_SIZE )
    return 3;
  return 5;
}
static inline size_t
pack_rv_size( uint8_t *ptr,  uint32_t fsize, size_t szbytes )
{
  if ( szbytes == 1 ) {
    ptr[ 0 ] = (uint8_t) fsize;
  }
  else if ( szbytes == 3 ) {
    ptr[ 0 ] = RV_SHORT_SIZE;
    ptr[ 1 ] = ( ( fsize + 2 ) >> 8 ) & 0xffU;
    ptr[ 2 ] = ( fsize + 2 ) & 0xffU;
  }
  else {
    ptr[ 0 ] = RV_LONG_SIZE;
    ptr[ 1 ] = ( ( fsize + 4 ) >> 24 ) & 0xffU;
    ptr[ 2 ] = ( ( fsize + 4 ) >> 16 ) & 0xffU;
    ptr[ 3 ] = ( ( fsize + 4 ) >> 8 ) & 0xffU;
    ptr[ 4 ] = ( fsize + 4 ) & 0xffU;
  }
  return szbytes;
}
static inline uint32_t
append_rv_field_hdr( uint8_t *buf,  const char *fname,  size_t fname_len,
                     uint32_t msg_len,  uint32_t msg_enc )
{
  uint32_t msg_off = 0;
  buf[ msg_off++ ] = (uint8_t) fname_len;
  ::memcpy( &buf[ msg_off ], fname, fname_len );
  msg_off += fname_len;
  buf[ msg_off++ ] = ( msg_enc == MD_STRING ) ? 8 : 7/*RV_OPAQUE*/;
  msg_off += pack_rv_size( &buf[ msg_off ], msg_len, rv_size_bytes( msg_len ) );
  return msg_off;
}

struct RvMsgWriter : public MDMsgWriterBase {
  RvMsgWriter * parent;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RvMsgWriter( MDMsgMem &m,  void *bb,  size_t len ) {
    this->msg_mem = &m;
    this->buf     = (uint8_t *) bb;
    this->off     = 8;
    this->wr_type = RVMSG_TYPE_ID;
    this->err     = 0;
    this->buflen  = len;
    this->parent  = NULL;
  }
  RvMsgWriter & error( int status ) {
    if ( this->err == 0 )
      this->err = status;
    if ( this->parent != NULL )
      this->parent->error( status );
    return *this;
  }
  void reset( void ) {
    this->off = 8;
    this->err = 0;
  }
  RvMsgWriter & append_ref( const char *fname,  size_t fname_len,
                            MDReference &mref,  MDReference & ) {
    return this->append_ref( fname, fname_len, mref );
  }
  RvMsgWriter & append_ref( const char *fname,  size_t fname_len,
                            MDReference &mref ) noexcept;
  bool has_space( size_t len ) {
    bool b = ( this->off + len <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool resize( size_t len ) noexcept;
  size_t update_hdr( void ) {
    if ( this->buflen == 0 )
      this->resize( 8 );
    this->buf[ 0 ] = ( this->off >> 24 ) & 0xffU;
    this->buf[ 1 ] = ( this->off >> 16 ) & 0xffU;
    this->buf[ 2 ] = ( this->off >> 8 ) & 0xffU;
    this->buf[ 3 ] = this->off & 0xffU;
    this->buf[ 4 ] = 0x99;
    this->buf[ 5 ] = 0x55;
    this->buf[ 6 ] = 0xee;
    this->buf[ 7 ] = 0xaa;
    return this->off;
  }
  /* usage of submsg appending:
   * msg.append_msg( "m", 2, submsg );
   * submsg.append_int<int32_t>( "i", 2, 100 );
   * len = msg.update_hdr( submsg ); */
  RvMsgWriter & append_msg( const char *fname,  size_t fname_len,
                            RvMsgWriter &submsg ) noexcept;
  RvMsgWriter & append_msg_elem( RvMsgWriter &submsg ) noexcept;
  size_t update_hdr( RvMsgWriter &submsg,  uint32_t suf_len = 0 ) {
    this->off += submsg.update_hdr() + suf_len;
    return this->update_hdr();
  }
  RvMsgWriter & append_msg_array( const char *fname,  size_t fname_len,
                                  size_t &aroff ) noexcept;
  void update_array_hdr( size_t aroff,  uint32_t num ) {
    size_t sz = this->off - aroff;
    this->buf[ aroff++ ] = ( sz >> 24 ) & 0xffU;
    this->buf[ aroff++ ] = ( sz >> 16 ) & 0xffU;
    this->buf[ aroff++ ] = ( sz >> 8 ) & 0xffU;
    this->buf[ aroff++ ] = sz & 0xffU;
    this->buf[ aroff++ ] = ( num >> 24 ) & 0xffU;
    this->buf[ aroff++ ] = ( num >> 16 ) & 0xffU;
    this->buf[ aroff++ ] = ( num >> 8 ) & 0xffU;
    this->buf[ aroff++ ] = num & 0xffU;
  }

  template< class T >
  RvMsgWriter & append_type( const char *fname,  size_t fname_len,  T val,
                             MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian; /* machine endian */
    return this->append_ref( fname, fname_len, mref );
  } 
  template< class T >
  RvMsgWriter & append_array_type( const char *fname,  size_t fname_len,
                                   const T *val,  size_t num,  MDType t ) {
    MDReference mref;
    mref.fptr     = (uint8_t *) (void *) val;
    mref.fsize    = sizeof( val[ 0 ] ) * num;
    mref.ftype    = MD_ARRAY;
    mref.fentrysz = sizeof( val[ 0 ] );
    mref.fentrytp = t;
    mref.fendian  = md_endian; /* machine endian */
    return this->append_ref( fname, fname_len, mref );
  } 
  template< class T >
  RvMsgWriter & append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  RvMsgWriter & append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  RvMsgWriter & append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }
  template< class T >
  RvMsgWriter & append_int( const char *fname,  T ival ) {
    return this->append_type( fname, ::strlen( fname ) + 1, ival, MD_INT );
  }
  template< class T >
  RvMsgWriter & append_uint( const char *fname,  T uval ) {
    return this->append_type( fname, ::strlen( fname ) + 1, uval, MD_UINT );
  }
  template< class T >
  RvMsgWriter & append_real( const char *fname,  T rval ) {
    return this->append_type( fname, ::strlen( fname ) + 1, rval, MD_REAL );
  }
  template< class T >
  RvMsgWriter & append_ipdata( const char *fname,  size_t fname_len,  T val ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = MD_IPDATA;
    mref.fendian = MD_BIG; /* alredy in network order */
    return this->append_ref( fname, fname_len, mref );
  }
  RvMsgWriter & append_string( const char *fname,  const char *str ) {
    return this->append_string( fname, ::strlen( fname ) + 1, str,
                                ::strlen( str ) + 1 );
  }
  RvMsgWriter & append_string( const char *fname,  size_t fname_len,
                               const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }
  RvMsgWriter & append_opaque( const char *fname,  size_t fname_len,
                               const void *ptr,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) ptr;
    mref.fsize   = len;
    mref.ftype   = MD_OPAQUE;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }
  /* subject format is string "XYZ.REC.INST.EX" */
  RvMsgWriter & append_subject( const char *fname,  size_t fname_len,
                              const char *subj,  size_t subj_len = 0 ) noexcept;
  RvMsgWriter & append_decimal( const char *fname,  size_t fname_len,
                                MDDecimal &dec ) noexcept;
  RvMsgWriter & append_time( const char *fname,  size_t fname_len,
                             MDTime &time ) noexcept;
  RvMsgWriter & append_date( const char *fname,  size_t fname_len,
                             MDDate &date ) noexcept;
  RvMsgWriter & append_stamp( const char *fname,  size_t fname_len,
                              MDStamp &stamp ) noexcept;
  RvMsgWriter & append_xml( const char *fname,  size_t fname_len,
                            const char *doc,  size_t doc_len ) noexcept;
  RvMsgWriter & append_enum( const char *fname,  size_t fname_len,
                             MDEnum &enu ) noexcept;
  RvMsgWriter & append_string_array( const char *fname,  size_t fname_len,
                                     char **ar,  size_t array_size,
                                     size_t fsize ) noexcept;
  RvMsgWriter & append_iter( MDFieldIter *iter ) noexcept;
  RvMsgWriter & append_writer( const RvMsgWriter &wr ) noexcept;
  RvMsgWriter & append_rvmsg( const RvMsg &msg ) noexcept;
  RvMsgWriter & append_buffer( const void *buffer,  size_t len ) noexcept;
  int convert_msg( MDMsg &jmsg,  bool skip_hdr ) noexcept;
};

}
} // namespace rai

#endif
#endif
