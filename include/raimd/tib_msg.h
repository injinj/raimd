#ifndef __rai_raimd__tib_msg_h__
#define __rai_raimd__tib_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

static const uint32_t RAIMSG_TYPE_ID = 0x07344064,
                      TIBMSG_TYPE_ID = RAIMSG_TYPE_ID;

struct TibMsg : public MDMsg {
  bool is_submsg;         /* if has tibmsg 9 byte header */
  /* used by unpack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibMsg( void *bb,  size_t off,  size_t len,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, len, d, m ), is_submsg( false ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg,
                           MDFieldIter *iter ) noexcept final;
  virtual int get_array_ref( MDReference &mref, size_t i,
                             MDReference &aref ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  /* convert tib decimal to md decimal */
  static bool set_decimal( MDDecimal &dec,  double val,
                           uint8_t tib_hint ) noexcept;
  static bool is_tibmsg( void *bb,  size_t off,  size_t len,
                         uint32_t h ) noexcept;
  static TibMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                         MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

struct TibFieldIter : public MDFieldIter {
  uint32_t  size,         /* size of data */
            hint_size,    /* hint size */
            data_off;     /* offset of data */
  uint8_t   name_len,     /* length of field name */
            type,         /* tib field type */
            hint_type,    /* tib field hint type */
            decimal_hint; /* tib decimal field hint */
  MDDecimal dec;          /* temp storage for decimal */
  MDDate    date;         /* temp storage for date */
  MDTime    time;         /* temp storage for time */
  bool      is_submsg;    /* if has tibmsg header */

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibFieldIter( TibMsg &m ) : MDFieldIter( m ), size( 0 ), hint_size( 0 ),
    name_len( 0 ), type( 0 ), hint_type( 0 ), is_submsg( m.is_submsg ) {}

  virtual int get_name( MDName &name ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_hint_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack( void ) noexcept;
};

enum {
  TIB_NODATA   = 0,
  TIB_MESSAGE  = 1,
  TIB_STRING   = 2,
  TIB_OPAQUE   = 3,
  TIB_BOOLEAN  = 4,
  TIB_INT      = 5,
  TIB_UINT     = 6,
  TIB_REAL     = 7,
  TIB_ARRAY    = 8,
  TIB_PARTIAL  = 9,
  TIB_IPDATA   = 10
#define TIB_MAX_TYPE 11
};

enum {
  TIB_HINT_NONE            = 0,   /* no hint */
  TIB_HINT_DENOM_2         = 1,   /* 1/2 */
  TIB_HINT_DENOM_4         = 2,
  TIB_HINT_DENOM_8         = 3,
  TIB_HINT_DENOM_16        = 4,
  TIB_HINT_DENOM_32        = 5,
  TIB_HINT_DENOM_64        = 6,
  TIB_HINT_DENOM_128       = 7,
  TIB_HINT_DENOM_256       = 8,   /* 1/256 */
  TIB_HINT_PRECISION_1     = 17,  /* 10^-1 */
  TIB_HINT_PRECISION_2     = 18,
  TIB_HINT_PRECISION_3     = 19,
  TIB_HINT_PRECISION_4     = 20,
  TIB_HINT_PRECISION_5     = 21,
  TIB_HINT_PRECISION_6     = 22,
  TIB_HINT_PRECISION_7     = 23,
  TIB_HINT_PRECISION_8     = 24,
  TIB_HINT_PRECISION_9     = 25,   /* 10^-9 */
  TIB_HINT_BLANK_VALUE     = 127,  /* no value */
  TIB_HINT_DATE_TYPE       = 256,  /* SASS TSS_STIME */
  TIB_HINT_TIME_TYPE       = 257,  /* SASS TSS_SDATE */
  TIB_HINT_MF_DATE_TYPE    = 258,  /* marketfeed date */
  TIB_HINT_MF_TIME_TYPE    = 259,  /* marketfeed time */
  TIB_HINT_MF_TIME_SECONDS = 260,  /* marketfeed time_seconds */
  TIB_HINT_MF_ENUM         = 261   /* marketfeed enum */
};

struct TibMsgWriter {
  MDMsgMem & mem;
  uint8_t  * buf;    /* output buffer */
  size_t     off,    /* index to end of data (+9 for hdr) */
             buflen; /* max length of a buf */
  int        err;

  TibMsgWriter( MDMsgMem &m,  void *bb,  size_t len )
    : mem( m ), buf( (uint8_t *) bb ), off( 0 ), buflen( len ), err( 0 ) {}
  TibMsgWriter & error( int status ) {
    if ( this->err == 0 )
      this->err = status;
    return *this;
  }
  void reset( void ) {
    this->off = 0;
    this->err = 0;
  }
  TibMsgWriter & append_ref( const char *fname,  size_t fname_len,
                             MDReference &mref ) noexcept;
  TibMsgWriter & append_ref( const char *fname,  size_t fname_len,
                             MDReference &mref,  MDReference &href ) noexcept;
  bool has_space( size_t len ) {
    bool b = ( this->off + 9 + len <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool resize( size_t len ) noexcept;
  size_t update_hdr( void ) {
    this->buf[ 0 ] = 0xce;
    this->buf[ 1 ] = 0x13;
    this->buf[ 2 ] = 0xaa;
    this->buf[ 3 ] = 0x1f;
    this->buf[ 4 ] = 1;
    this->buf[ 5 ] = ( this->off >> 24 ) & 0xffU;
    this->buf[ 6 ] = ( this->off >> 16 ) & 0xffU;
    this->buf[ 7 ] = ( this->off >> 8 ) & 0xffU;
    this->buf[ 8 ] = this->off & 0xffU;
    return this->off + 9; /* returns length of data in buffer */
  }

  template< class T >
  TibMsgWriter & append_type( const char *fname,  size_t fname_len,  T val,
                              MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  template< class T >
  TibMsgWriter & append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  TibMsgWriter & append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  TibMsgWriter & append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }

  TibMsgWriter & append_string( const char *fname,  size_t fname_len,
                                const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  TibMsgWriter & append_decimal( const char *fname,  size_t fname_len,
                                 MDDecimal &dec ) noexcept;
  TibMsgWriter & append_time( const char *fname,  size_t fname_len,
                              MDTime &time ) noexcept;
  TibMsgWriter & append_date( const char *fname,  size_t fname_len,
                              MDDate &date ) noexcept;
  TibMsgWriter & append_enum( const char *fname,  size_t fname_len,
                              MDEnum &enu ) noexcept;
  TibMsgWriter & append_iter( MDFieldIter *iter ) noexcept;
  int convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept;
};

}
} // namespace rai

#endif
