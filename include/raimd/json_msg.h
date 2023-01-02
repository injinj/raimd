#ifndef __rai_raimd__json_msg_h__
#define __rai_raimd__json_msg_h__

#include <raimd/md_msg.h>
#include <raimd/json.h>

namespace rai {
namespace md {

struct JsonParser;
struct JsonObject;

static const uint32_t JSON_TYPE_ID = 0x4a014cc2;

struct JsonMsg : public MDMsg {
  /* used by unpack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonValue * js; /* any json atom:  object, string, array, number */

  JsonMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ), js( 0 ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  virtual int get_array_ref( MDReference &mref, size_t i,
                             MDReference &aref ) noexcept final;
  static int value_to_ref( MDReference &mref,  JsonValue &x ) noexcept;
  static bool is_jsonmsg( void *bb,  size_t off,  size_t end,
                          uint32_t h ) noexcept;
  /* unpack only objects delimited by '{' '}' */
  static JsonMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                          MDDict *d,  MDMsgMem *m ) noexcept;
  /* try to parse any json:  array, string, number */
  static JsonMsg *unpack_any( void *bb,  size_t off,  size_t end,  uint32_t h,
                              MDDict *d,  MDMsgMem *m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

struct JsonMsgCtx {
  JsonMsg      * msg;
  JsonParser   * parser;
  JsonBufInput * input;
  MDMsgMem     * mem;
  JsonMsgCtx() : msg( 0 ), parser( 0 ), input( 0 ), mem( 0 ) {}
  ~JsonMsgCtx() noexcept;
  int parse( void *bb,  size_t off,  size_t end,  MDDict *d,
             MDMsgMem *m,  bool is_yaml ) noexcept;
  void release( void ) noexcept;
};

struct JsonFieldIter : public MDFieldIter {
  JsonMsg    & me;
  JsonObject & obj;

  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonFieldIter( JsonMsg &m,  JsonObject &o ) : MDFieldIter( m ),
    me( m ), obj( o ) {}

  virtual int get_name( MDName &name ) noexcept final;
  virtual int copy_name( char *name,  size_t &name_len,  MDFid &fid ) noexcept;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
};

struct JsonMsgWriter {
  enum {
    FIRST_FIELD = 1
  };
  uint8_t * buf;
  size_t    off,
            buflen;
  int       flags;

  JsonMsgWriter( void *bb,  size_t len ) : buf( (uint8_t *) bb ), off( 0 ),
                                           buflen( len ), flags( 0 ) {}
  void reset( void ) {
    this->off = 0;
  }
  bool has_space( size_t len ) const {
    return this->off + len <= this->buflen;
  }
  int append_field_name( const char *fname,  size_t fname_len ) noexcept;

  int append_field( const char *fname,  size_t fname_len,
                    MDReference &mref ) noexcept;
  int append_ref( MDReference &mref ) noexcept;

  int append_msg( const char *fname,  size_t fname_len,
                  JsonMsgWriter &submsg ) noexcept;
  int convert_msg( MDMsg &msg ) noexcept;

  bool s( const char *str,  size_t len ) {
    bool b = this->has_space( len );
    if ( b ) {
      ::memcpy( &this->buf[ this->off ], str, len );
      this->off += len;
    }
    return b;
  }
  bool start( void ) {
    bool b = this->has_space( 3 );
    if ( b )
      this->buf[ this->off++ ] = '{';
    return b;
  }
  bool finish( void ) {
    bool b = ( ( this->flags & FIRST_FIELD ) == 0 ) ?
      this->start() : this->has_space( 2 );
    if ( b ) {
      this->buf[ this->off++ ] = '}';
      this->buf[ this->off ]   = '\0';
    }
    return b;
  }

  template< class T >
  int append_type( const char *fname,  size_t fname_len,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_field( fname, fname_len, mref );
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
    return this->append_field( fname, fname_len, mref );
  }

  int append_decimal( const char *fname,  size_t fname_len,
                      MDDecimal &dec ) {
    return this->append_type( fname, fname_len, dec, MD_DECIMAL );
  }
  int append_time( const char *fname,  size_t fname_len,
                   MDTime &time ) {
    return this->append_type( fname, fname_len, time, MD_TIME );
  }
  int append_date( const char *fname,  size_t fname_len,
                   MDDate &date ) {
    return this->append_type( fname, fname_len, date, MD_DATE );
  }
  size_t update_hdr( void ) {
    this->finish();
    return this->off;
  }
};

}
}

#endif
