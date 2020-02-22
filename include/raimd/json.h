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
};

struct JsonBoolean : public JsonValue {
  bool val;
};

struct JsonNumber : public JsonValue {
  MDDecimal val;
};

struct JsonString : public JsonValue {
  char * val;
};

struct JsonObject : public JsonValue {
  struct Pair {
    char      * name;
    JsonValue * val;
  } * val;
  size_t length;
  JsonValue *find( const char *name ) const noexcept;
};

struct JsonArray : public JsonValue {
  JsonValue ** val;
  size_t       length;
};

inline JsonBoolean * JsonValue::to_bool( void ) const { return (JsonBoolean *) this; }
inline JsonNumber  * JsonValue::to_num( void )  const { return (JsonNumber *) this; }
inline JsonString  * JsonValue::to_str( void )  const { return (JsonString *) this; }
inline JsonObject  * JsonValue::to_obj( void )  const { return (JsonObject *) this; }
inline JsonArray   * JsonValue::to_arr( void )  const { return (JsonArray *) this; }

static const int JSON_EOF = 256;

struct JsonStreamInput {
  char * json,
         buf1[ 4 * 1024 ],
         buf2[ 4 * 1024 ];
  size_t offset,
         length,
         line_start,
         line_count,
         buf1_off,
         buf2_off;
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
    if ( n != JSON_EOF )
      this->offset++;
    return n;
  }
  int  forward( void ) {
    if ( this->next() == JSON_EOF )
      return JSON_EOF;
    return this->cur();
  }
  bool match( char c1,  char c2,  char c3,  char c4,  char c5 ) noexcept;
  int  eat_white( void ) noexcept;
  void skip_BOM( void ) noexcept;

  JsonStreamInput() {
    this->init();
  }

  void init( void ) {
    this->offset     = 0;
    this->length     = 0;
    this->line_start = 0;
    this->line_count = 0;
    this->buf1_off   = 0;
    this->buf2_off   = 0;
    this->is_eof     = false;
    this->json       = this->buf2;
  }
};

struct JsonBufInput {
  const char * json;
  size_t       offset,
               length,
               line_start,
               line_count;

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
  int  eat_white( void ) noexcept;
  void skip_BOM( void ) noexcept;

  JsonBufInput( const char *js = NULL,  unsigned int off = 0,
                unsigned int len = 0 ) {
    this->init( js, off, len );
  }

  void init( const char *js,  unsigned int off,  unsigned int len ) {
    this->json       = js;
    this->offset     = off;
    this->length     = len;
    this->line_start = 0;
    this->line_count = 0;
  }
};

}
}

#endif
