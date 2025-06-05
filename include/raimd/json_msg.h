#ifndef __rai_raimd__json_msg_h__
#define __rai_raimd__json_msg_h__

#include <raimd/md_msg.h>
#include <raimd/json.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t JSON_TYPE_ID = 0x4a014cc2U;
#ifndef __cplusplus
#define JSON_TYPE_ID 0x4a014cc2U
#endif
MDMsg_t *json_msg_unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                          MDDict_t *d,  MDMsgMem_t *m );
MDMsgWriter_t * json_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,
                                        void *buf_ptr, size_t buf_sz );
#ifdef __cplusplus
}
namespace rai {
namespace md {

struct JsonParser;
struct JsonObject;

struct JsonMsg : public MDMsg {
  /* used by unpack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonValue * js; /* any json atom:  object, string, array, number */

  JsonMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, end, d, m ), js( 0 ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg,
                           MDFieldIter *iter ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  virtual int get_array_ref( MDReference &mref, size_t i,
                             MDReference &aref ) noexcept final;
  static int value_to_ref( MDReference &mref,  JsonValue &x ) noexcept;
  static bool is_jsonmsg( void *bb,  size_t off,  size_t end,
                          uint32_t h ) noexcept;
  /* unpack only objects delimited by '{' '}' */
  static JsonMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                          MDDict *d,  MDMsgMem &m ) noexcept;
  /* try to parse any json:  array, string, number */
  static JsonMsg *unpack_any( void *bb,  size_t off,  size_t end,  uint32_t h,
                              MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

struct JsonMsgCtx {
  JsonMsg         * msg;
  JsonParser      * parser;
  JsonBufInput    * input;
  JsonStreamInput * stream;
  MDMsgMem        * mem;
  JsonMsgCtx() : msg( 0 ), parser( 0 ), input( 0 ), stream( 0 ), mem( 0 ) {}
  ~JsonMsgCtx() noexcept;
  int parse( void *bb,  size_t off,  size_t end,  MDDict *d,
             MDMsgMem &m,  bool is_yaml ) noexcept;
  int parse_fd( int fd,  MDDict *d,  MDMsgMem &m,  bool is_yaml ) noexcept;
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

struct JsonMsgWriter : public MDMsgWriterBase {
  enum {
    FIRST_FIELD = 1
  };
  int             flags;
  JsonMsgWriter * parent;

  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonMsgWriter( MDMsgMem &m,  void *bb,  size_t len ) {
    this->msg_mem = &m;
    this->buf     = (uint8_t *) bb;
    this->off     = 0;
    this->buflen  = len;
    this->wr_type = JSON_TYPE_ID;
    this->err     = 0;
    this->flags   = 0;
    this->parent  = NULL;
  }
  JsonMsgWriter & error( int status ) {
    if ( this->err == 0 )
      this->err = status;
    if ( this->parent != NULL )
      this->parent->error( status );
    return *this;
  }
  void reset( void ) {
    this->off = 0;
    this->err = 0;
  }
  bool has_space( size_t len ) {
    bool b = ( this->off + len <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool resize( size_t len ) noexcept;

  int append_field_name( const char *fname,  size_t fname_len ) noexcept;

  JsonMsgWriter & append_field( const char *fname,  size_t fname_len,
                                MDReference &mref ) noexcept;
  JsonMsgWriter & append_ref( MDReference &mref ) noexcept;

  JsonMsgWriter & append_msg( const char *fname,  size_t fname_len,
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
  JsonMsgWriter & append_type( const char *fname,  size_t fname_len,  T val,
                               MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_field( fname, fname_len, mref );
  }

  template< class T >
  JsonMsgWriter & append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  JsonMsgWriter & append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  JsonMsgWriter & append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }

  JsonMsgWriter & append_string( const char *fname,  size_t fname_len,
                                 const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_field( fname, fname_len, mref );
  }

  JsonMsgWriter & append_decimal( const char *fname,  size_t fname_len,
                                  MDDecimal &dec ) {
    return this->append_type( fname, fname_len, dec, MD_DECIMAL );
  }
  JsonMsgWriter & append_time( const char *fname,  size_t fname_len,
                               MDTime &time ) {
    return this->append_type( fname, fname_len, time, MD_TIME );
  }
  JsonMsgWriter & append_date( const char *fname,  size_t fname_len,
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
#endif
