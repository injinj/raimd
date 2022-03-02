#ifndef __rai_raimd__json_h__
#define __rai_raimd__json_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

enum JsonType {
  JSON_NONE,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_NUMBER,
  JSON_STRING,
  JSON_BOOLEAN,
  JSON_NULL
};

struct JsonValue;
struct JsonStreamInput;
struct JsonBufInput;
struct MDMsgMem;

struct JsonParser {
  MDMsgMem  & mem;
  JsonValue * value;
  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonParser( MDMsgMem &m ) : mem( m ) {}
  /* result contains the offset of json[] consumed as well as the memory
     allocated, if any */
  int parse( JsonStreamInput &input ) noexcept;

  int parse( JsonBufInput &input ) noexcept;

  int parse_yaml( JsonStreamInput &input ) noexcept;

  int parse_yaml( JsonBufInput &input ) noexcept;
};

struct JsonBoolean;
struct JsonNumber;
struct JsonString;
struct JsonObject;
struct JsonArray;

struct JsonValue {
  JsonType type;
  int print( MDOutput *out ) noexcept;
  JsonBoolean *to_bool( void ) const;
  JsonNumber *to_num( void ) const;
  JsonString *to_str( void ) const;
  JsonObject *to_obj( void ) const;
  JsonArray *to_arr( void ) const;
  int to_double( double &v ) const noexcept; /* return double */
  int to_int( int64_t &v ) const noexcept;  /* return int */
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonValue( JsonType t = JSON_NULL ) : type( t ) {}
};

struct JsonBoolean : public JsonValue {
  bool val;
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonBoolean(  bool v = false ) : JsonValue( JSON_BOOLEAN ), val( v ) {}
  int print( MDOutput *out ) noexcept;
};

struct JsonNumber : public JsonValue {
  MDDecimal val;
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonNumber( void ) : JsonValue( JSON_NUMBER ) { this->val.zero(); }
  int print( MDOutput *out ) noexcept;
};

struct JsonString : public JsonValue {
  char * val;
  size_t length;
  void set( const JsonString &s ) {
    this->type   = JSON_STRING;
    this->val    = s.val;
    this->length = s.length;
  }
  void zero( void ) {
    this->type   = JSON_STRING;
    this->val    = NULL;
    this->length = 0;
  }
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonString() : JsonValue( JSON_STRING ), val( 0 ), length( 0 ) {}
  int print( MDOutput *out ) noexcept;
};

struct JsonObject : public JsonValue {
  struct Pair {
    JsonString  name;
    JsonValue * val;
  } * val;
  size_t length;
  JsonValue *find( const char *name ) const noexcept;
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonObject() : JsonValue( JSON_OBJECT ), val( 0 ), length( 0 ) {}
  int print( MDOutput *out ) noexcept;
};

struct JsonArray : public JsonValue {
  JsonValue ** val;
  size_t       length;
  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonArray() : JsonValue( JSON_ARRAY ), val( 0 ), length( 0 ) {}
  int print( MDOutput *out ) noexcept;
};

inline JsonBoolean * JsonValue::to_bool( void ) const { return (JsonBoolean *) this; }
inline JsonNumber  * JsonValue::to_num( void )  const { return (JsonNumber *) this; }
inline JsonString  * JsonValue::to_str( void )  const { return (JsonString *) this; }
inline JsonObject  * JsonValue::to_obj( void )  const { return (JsonObject *) this; }
inline JsonArray   * JsonValue::to_arr( void )  const { return (JsonArray *) this; }

static const int JSON_EOF = 256;

struct JsonStreamInput {
  char   buf[ 4 * 1024 ];
  char * json;
  size_t offset,
         length,
         line_start,
         line_count,
         buf_len;
  bool   is_eof;

  virtual size_t read( uint8_t *buf,  size_t len ) noexcept;
  bool fill_buf( void ) noexcept;
  int  cur( void ) {
    do {
      if ( this->offset < this->length )
        return (int) (uint8_t) this->json[ this->offset ];
    } while ( ! this->is_eof && this->fill_buf() );
    return JSON_EOF;
  }
  int  next( void ) {
    int n = this->cur();
    if ( n != JSON_EOF ) {
      this->offset++;
    }
    return n;
  }
  int  forward( void ) {
    if ( this->next() == JSON_EOF )
      return JSON_EOF;
    return this->cur();
  }
  bool match( char c1,  char c2,  char c3,  char c4,  char c5 ) noexcept;
  bool lookahead_case( char c1,  char c2,  char c3 ) noexcept;
  int  eat_white( void ) noexcept;
  void skip_BOM( void ) noexcept;

  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonStreamInput() : json( this->buf ) {
    this->init();
  }
  virtual ~JsonStreamInput() {
    if ( this->json != this->buf )
      ::free( this->json );
  }

  void init( void ) {
    if ( this->json != this->buf )
      ::free( this->json );
    this->json       = this->buf;
    this->buf_len    = sizeof( this->buf );
    this->offset     = 0;
    this->length     = 0;
    this->line_start = 0;
    this->line_count = 0;
    this->is_eof     = false;
  }
  int col( void ) {
    return (int) ( this->offset - this->line_start + 1 );
  }
  const char *line_ptr( void ) const {
    return &this->json[ this->line_start ];
  }
  const char *end_ptr( void ) const {
    return &this->json[ this->length ];
  }
};

struct JsonBufInput {
  const char * json;
  size_t       offset,
               length,
               line_start,
               line_count;

  bool fill_buf( void ) { return false; }
  int  cur( void ) {
    if ( this->offset < this->length )
      return (int) (uint8_t) this->json[ this->offset ];
    return JSON_EOF;
  }
  int  next( void ) {
    if ( this->offset < this->length )
      return (int) (uint8_t) this->json[ this->offset++ ];
    return JSON_EOF;
  }
  int  forward( void ) {
    this->offset++;
    return this->cur();
  }
  bool match( char c1,  char c2,  char c3,  char c4,  char c5 ) noexcept;
  bool lookahead_case( char c1,  char c2,  char c3 ) noexcept;
  int  eat_white( void ) noexcept;
  void skip_BOM( void ) noexcept;

  void * operator new( size_t, void *ptr ) { return ptr; }
  JsonBufInput( const char *js = NULL,  size_t off = 0,
                size_t len = 0 ) {
    this->init( js, off, len );
  }

  void init( const char *js,  size_t off,  size_t len ) {
    this->json       = js;
    this->offset     = off;
    this->length     = len;
    this->line_start = 0;
    this->line_count = 0;
  }
  int col( void ) {
    return (int) ( this->offset - this->line_start + 1 );
  }
  const char *line_ptr( void ) const {
    return &this->json[ this->line_start ];
  }
  const char *end_ptr( void ) const {
    return &this->json[ this->length ];
  }
};

}
}

#endif
