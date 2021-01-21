#ifndef __rai_raimd__rv_msg_h__
#define __rai_raimd__rv_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

static const uint32_t RVMSG_TYPE_ID = 0xebf946be;

struct RvMsg : public MDMsg {
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;

  /* may return tibmsg, sass qform or rv */
  static bool is_rvmsg( void *bb,  size_t off,  size_t end,
                        uint32_t h ) noexcept;
  static RvMsg *unpack_rv( void *bb,  size_t off,  size_t end,  uint32_t h,
                           MDDict *d,  MDMsgMem *m ) noexcept;
  static MDMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict *d,  MDMsgMem *m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
  static MDMsg *opaque_extract( uint8_t *bb,  size_t off,  size_t end,
                                MDDict *d,  MDMsgMem *m ) noexcept;
};

struct RvFieldIter : public MDFieldIter {
  uint32_t size;
  uint8_t  type,
           name_len;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvFieldIter( MDMsg &m )
    : MDFieldIter( m ), size( 0 ), type( 0 ), name_len( 0 ) {}

  virtual int get_name( MDName &name ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack( void ) noexcept;
};

struct RvMsgWriter {
  uint8_t * buf;
  size_t    off,
            buflen;

  RvMsgWriter( void *bb,  size_t len ) : buf( (uint8_t *) bb ), off( 8 ),
                                         buflen( len ) {}
  void reset( void ) {
    this->off = 8;
  }
  int append_ref( const char *fname,  size_t fname_len,
                  MDReference &mref ) noexcept;
  bool has_space( size_t len ) const {
    return this->off + len <= this->buflen;
  }

  size_t update_hdr( void ) {
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
  int append_msg( const char *fname,  size_t fname_len,
                  RvMsgWriter &submsg ) noexcept;
  size_t update_hdr( RvMsgWriter &submsg ) {
    this->off += submsg.update_hdr();
    return this->update_hdr();
  }

  template< class T >
  int append_type( const char *fname,  size_t fname_len,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian; /* machine endian */
    return this->append_ref( fname, fname_len, mref );
  } 
    
  template< class T >
  int append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  int append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  int append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }
  template< class T >
  int append_ipdata( const char *fname,  size_t fname_len,  T val ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = MD_IPDATA;
    mref.fendian = MD_BIG; /* alredy in network order */
    return this->append_ref( fname, fname_len, mref );
  }
  int append_string( const char *fname,  size_t fname_len,
                     const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }
  int append_opaque( const char *fname,  size_t fname_len,
                     const void *ptr,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) ptr;
    mref.fsize   = len;
    mref.ftype   = MD_OPAQUE;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }
  /* subject format is string "XYZ.REC.INST.EX" */
  int append_subject( const char *fname,  size_t fname_len,
                      const char *subj,  size_t subj_len = 0 ) noexcept;
  int append_decimal( const char *fname,  size_t fname_len,
                      MDDecimal &dec ) noexcept;
  int append_time( const char *fname,  size_t fname_len,
                   MDTime &time ) noexcept;
  int append_date( const char *fname,  size_t fname_len,
                   MDDate &date ) noexcept;
};

}
} // namespace rai

#endif
