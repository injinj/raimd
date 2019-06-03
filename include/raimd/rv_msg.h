#ifndef __rai_raimd__rv_msg_h__
#define __rai_raimd__rv_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

struct RvMsg : public MDMsg {
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg ) final;
  virtual int get_field_iter( MDFieldIter *&iter ) final;

  /* may return tibmsg, sass qform or rv */
  static bool is_rvmsg( void *bb,  size_t off,  size_t end,  uint32_t h );
  static MDMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict *d,  MDMsgMem *m );
  static void init_auto_unpack( void );
  static MDMsg *opaque_extract( uint8_t *bb,  size_t off,  size_t end,
                                MDDict *d,  MDMsgMem *m );
};

struct RvFieldIter : public MDFieldIter {
  uint32_t size;
  uint8_t  type,
           name_len;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RvFieldIter( MDMsg &m )
    : MDFieldIter( m ), size( 0 ), type( 0 ), name_len( 0 ) {}

  virtual int get_name( MDName &name ) final;
  virtual int get_reference( MDReference &mref ) final;
  virtual int find( const char *name, size_t name_len, MDReference &mref )final;
  virtual int first( void ) final;
  virtual int next( void ) final;
  int unpack( void );
};

struct RvMsgWriter {
  uint8_t * buf;
  size_t    off,
            buflen;

  RvMsgWriter( void *bb,  size_t len ) : buf( (uint8_t *) bb ), off( 8 ),
                                         buflen( len ) {}
  int append_ref( const char *fname,  size_t fname_len,  MDReference &mref );

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

  template< class T >
  int append_type( const char *fname,  size_t fname_len,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
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

  int append_string( const char *fname,  size_t fname_len,
                     const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  int append_decimal( const char *fname,  size_t fname_len, MDDecimal &dec );
  int append_time( const char *fname,  size_t fname_len,  MDTime &time );
  int append_date( const char *fname,  size_t fname_len,  MDDate &date );
};

}
} // namespace rai

#endif
