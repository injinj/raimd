#ifndef __rai_raimd__tib_sass_msg_h__
#define __rai_raimd__tib_sass_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

static const uint32_t TIB_SASS_TYPE_ID      = 0x179ca0f5,
                      TIB_SASS_FORM_TYPE_ID = 0xa08b0040;

struct TibSassMsg : public MDMsg {
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibSassMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) final;
  virtual uint32_t get_type_id( void ) final;
  virtual int get_field_iter( MDFieldIter *&iter ) final;

  /* may return tibmsg, sass qform or rv */
  static bool is_tibsassmsg( void *bb,  size_t off,  size_t end,  uint32_t h );
  static TibSassMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                             MDDict *d,  MDMsgMem *m );
  static void init_auto_unpack( void );
};

static inline size_t tib_sass_pack_size( size_t fsize ) {
  return ( 1U + ( ( fsize + 1U ) >> 1 ) ) << 1; /* fid + fsize  2b aligned */
}
static inline size_t tib_sass_partial_pack_size( size_t fsize ) {
  return tib_sass_pack_size( fsize + 4 ); /* fid + off + len, 2b aligned */
}

struct TibSassFieldIter : public MDFieldIter {
  const char * fname;
  uint32_t     fsize;
  MDFid        fid;
  MDType       ftype;
  uint8_t      fnamelen;
  MDDecimal    dec;
  MDTime       time;
  MDDate       date;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibSassFieldIter( MDMsg &m ) : MDFieldIter( m ), fname( 0 ), fsize( 0 ),
                                 fid( 0 ), ftype( MD_NODATA ), fnamelen( 0 ) {}

  virtual int get_name( MDName &name ) final;
  virtual int get_reference( MDReference &mref ) final;
  virtual int get_hint_reference( MDReference &mref ) final;
  virtual int find( const char *name, size_t name_len, MDReference &mref )final;
  virtual int first( void ) final;
  virtual int next( void ) final;
  int unpack( void );

  size_t pack_size( void ) const {
    return tib_sass_pack_size( this->fsize );
  }
  static size_t partial_pack_size( uint16_t len ) {
    return tib_sass_partial_pack_size( len );
  }
};

struct TibSassMsgWriter {
  MDDict  * dict;
  uint8_t * buf;
  size_t    off,
            buflen;

  TibSassMsgWriter( MDDict *d,  void *bb,  size_t len );

  int append_ref( MDFid fid,  MDReference &mref );
  int append_ref( const char *fname,  size_t fname_len,  MDReference &mref );
  int append_ref( MDFid fid, MDType ftype, uint32_t fsize, MDReference &mref );

  bool has_space( size_t len ) const {
    return this->off + 8 + len <= this->buflen;
  }

  size_t update_hdr( void ) {
    this->buf[ 0 ] = 0x11;
    this->buf[ 1 ] = 0x11;
    this->buf[ 2 ] = 0x11;
    this->buf[ 3 ] = 0x12;
    this->buf[ 4 ] = ( this->off >> 24 ) & 0xffU;
    this->buf[ 5 ] = ( this->off >> 16 ) & 0xffU;
    this->buf[ 6 ] = ( this->off >> 8 ) & 0xffU;
    this->buf[ 7 ] = this->off & 0xffU;
    return this->off + 8;
  }

  template< class T >
  int append_type( MDFid fid,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
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
  int append_int( MDFid fid,  T ival ) {
    return this->append_type( fid, ival, MD_INT );
  }
  template< class T >
  int append_uint( MDFid fid,  T uval ) {
    return this->append_type( fid, uval, MD_UINT );
  }
  template< class T >
  int append_real( MDFid fid,  T rval ) {
    return this->append_type( fid, rval, MD_REAL );
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

  int append_string( MDFid fid,  const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
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

  int append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                      MDDecimal &dec );
  int append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                   MDTime &time );
  int append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                   MDDate &date );

  int append_decimal( MDFid fid,  MDDecimal &dec );
  int append_time( MDFid,  MDTime &time );
  int append_date( MDFid,  MDDate &date );

  int append_decimal( const char *fname,  size_t fname_len,  MDDecimal &dec );
  int append_time( const char *fname,  size_t fname_len,  MDTime &time );
  int append_date( const char *fname,  size_t fname_len,  MDDate &date );
};

}
} // namespace rai

#endif
