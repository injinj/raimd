#include <string.h>
#include <ctype.h>
#include <math.h>

#include <raimd/json.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

int
JsonValue::to_double( double &v ) const {
  if ( this->type != JSON_NUMBER ) {
    if ( this->type == JSON_STRING ) {
      const JsonString * js = (const JsonString *) this;
      v = parse_f64( js->val, NULL );
      return 0;
    }
    return Err::BAD_CAST;
  }
  ((const JsonNumber *) this)->val.get_real( v );
  return 0;
}

int
JsonValue::to_int( int64_t &v ) const {
  double x;
  if ( this->type != JSON_NUMBER ) {
    if ( this->type == JSON_STRING ) {
      const JsonString * js = (const JsonString *) this;
      v = parse_i64( js->val, NULL );
      return 0;
    }
    return Err::BAD_CAST;
  }
  ((const JsonNumber *) this)->val.get_real( x );
  v = (int64_t) x;
  return 0;
}

JsonValue *
JsonObject::find( const char *name ) const {
  for ( size_t i = 0; i < this->length; i++ )
    if ( ::strcmp( this->val[ i ].name, name ) == 0 )
      return this->val[ i ].val;
  return NULL;
}

namespace rai {
namespace md {

struct JsonValueP : public JsonValue {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonValueP() { this->type = JSON_NULL; }
  int print( MDOutput *out );
};

struct JsonBooleanP : public JsonBoolean {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonBooleanP(  bool v = false ) { this->type = JSON_BOOLEAN; this->val = v; }
  int print( MDOutput *out );
};

struct JsonNumberP : public JsonNumber {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonNumberP( void ) {
    this->type = JSON_NUMBER;
    this->val.zero();
  }
  int print( MDOutput *out );
};

struct JsonStringP : public JsonString {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonStringP() { this->type = JSON_STRING; this->val = NULL; }
  int print( MDOutput *out );
};

struct JsonObjectP : public JsonObject {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonObjectP() { this->type = JSON_OBJECT; this->val = 0; this->length = 0; }
  int print( MDOutput *out );
};

struct JsonArrayP : public JsonArray {
  void * operator new( size_t sz, void *ptr ) { return ptr; }
  JsonArrayP() { this->type = JSON_ARRAY; this->val = NULL; this->length = 0; }
  int print( MDOutput *out );
};


struct JsonContext {
  JsonParser & parser;

  JsonContext( JsonParser *p ) : parser( *p ) {}

  void *alloc( size_t sz );

  void *extend( void *p,  size_t cursz,  size_t addsz );
};

void *
JsonContext::alloc( size_t sz )
{
  void * p;
  this->parser.mem.alloc( sz, &p );
  return p;
}

void *
JsonContext::extend( void *p,  size_t cursz,  size_t addsz )
{
  this->parser.mem.extend( cursz, cursz + addsz, &p );
  return p;
}

template <class JsonInput>
struct JsonOne {
  JsonInput   & input;
  JsonContext & ctx;

  void * operator new( size_t sz, void *ptr ) { return ptr; }

  JsonOne( JsonInput &in,  JsonContext &c ) : input( in ), ctx( c ) {}

  JsonBooleanP *create_boolean( bool v ) {
    return new ( this->ctx.alloc( sizeof( JsonBooleanP ) ) ) JsonBooleanP( v );
  }
  JsonNumberP *create_number( void ) {
    return new ( this->ctx.alloc( sizeof( JsonNumberP ) ) ) JsonNumberP();
  }
  JsonStringP *create_string( void ) {
    return new ( this->ctx.alloc( sizeof( JsonStringP ) ) ) JsonStringP();
  }
  JsonObjectP *create_object( void ) {
    return new ( this->ctx.alloc( sizeof( JsonObjectP ) ) ) JsonObjectP();
  }
  JsonArrayP *create_array( void ) {
    return new ( this->ctx.alloc( sizeof( JsonArrayP ) ) ) JsonArrayP();
  }
  JsonValueP *create_null( void ) {
    return new ( this->ctx.alloc( sizeof( JsonValueP ) ) ) JsonValueP();
  }
  int parse( JsonValue *&val );
  int parse_number( JsonNumberP &num );
  int parse_array( JsonArrayP &ar );
  int parse_object( JsonObjectP &obj );
  int parse_string( JsonStringP &string );
  int parse_ident( JsonStringP &string );
};

}
} // namespace rai

size_t
JsonStreamInput::read( uint8_t *buf,  size_t len )
{
  return 0;
}

bool
JsonStreamInput::fill_buf( void )
{
  size_t len = 0;
  if ( ! this->is_eof ) {
    char   * new_buf;
    size_t * new_off,
           * old_off;
    if ( this->json == this->buf1 ) {
      new_buf = this->buf2;
      new_off = &this->buf2_off;
      old_off = &this->buf1_off;
    }
    else {
      new_buf = this->buf1;
      new_off = &this->buf1_off;
      old_off = &this->buf2_off;
    }
    *new_off = *old_off + this->length;
    if ( this->offset < this->length ) {
      ::memcpy( new_buf, &this->json[ this->offset ],
                this->length - this->offset );
      this->length -= this->offset;
      *new_off -= this->length;
      this->offset  = 0;
    }
    else {
      this->offset = this->length = 0;
    }
    this->json = new_buf;
    len = this->read( (uint8_t *) &this->json[ this->length ],
                      sizeof( this->buf1 ) - this->length );
    if ( len == 0 )
      this->is_eof = true;
    this->length += len;
  }
  return len > 0;
}

bool
JsonStreamInput::match( char c1,  char c2,  char c3,  char c4,  char c5 )
{
  for (;;) {
    if ( this->offset + 5 >= this->length )
      if ( this->is_eof || ! this->fill_buf() )
        return false;

    if ( this->offset < this->length &&
         c1 != this->json[ this->offset ] )
      return false;
    if ( this->offset + 1 < this->length &&
         c2 != this->json[ this->offset + 1 ] )
      return false;
    if ( this->offset + 2 < this->length &&
         c3 != this->json[ this->offset + 2 ] )
      return false;
    if ( this->offset + 3 < this->length &&
         c4 != this->json[ this->offset + 3 ] )
      return false;
    if ( this->offset + 3 < this->length && c5 == 0 ) {
      this->offset += 4;
      return true;
    }
    if ( this->offset + 4 < this->length &&
         c5 != this->json[ this->offset + 4 ] )
      return false;
    if ( this->offset + 4 < this->length ) {
      this->offset += 5;
      return true;
    }
  }
}

int
JsonStreamInput::eat_white( void )
{
  int c = this->cur();
  if ( isspace( c ) ) {
    do {
      if ( c == '\n' ) {
        this->line_count++;
        this->line_start = this->offset + 1;
      }
      c = this->forward();
    } while ( isspace( c ) );
  }
  return c;
}

void
JsonStreamInput::skip_BOM( void )
{
  for (;;) {
    if ( this->offset + 3 > this->length )
      if ( this->is_eof || ! this->fill_buf() )
        return;
    /* Skip UTF-8 BOM */
    if ( this->offset < this->length &&
         (uint8_t) this->json[ this->offset ] != 0xefU )
      return;
    if ( this->offset + 1 < this->length &&
         (uint8_t) this->json[ this->offset + 1 ] != 0xbbU )
      return;
    if ( this->offset + 2 < this->length &&
         (uint8_t) this->json[ this->offset + 2 ] != 0xbfU )
      return;
    if ( this->offset + 2 < this->length ) {
      this->offset += 3;
      return;
    }
  }
}

int
JsonBufInput::eat_white( void )
{
  int c = this->cur();
  if ( isspace( c ) ) {
    do {
      if ( c == '\n' ) {
        this->line_count++;
        this->line_start = this->offset + 1;
      }
      c = this->forward();
    } while ( isspace( c ) );
  }
  return c;
}

bool
JsonBufInput::match( char c1,  char c2,  char c3,  char c4,  char c5 )
{
  if ( this->offset + 4 > this->length ||
       c1 != this->json[ this->offset ] ||
       c2 != this->json[ this->offset + 1 ] ||
       c3 != this->json[ this->offset + 2 ] ||
       c4 != this->json[ this->offset + 3 ] )
    return false;
  if ( c5 == 0 ) {
    this->offset += 4;
    return true;
  }
  if ( this->offset + 5 > this->length ||
       c5 != this->json[ this->offset + 4 ] )
    return false;
  this->offset += 5;
  return true;
}

void
JsonBufInput::skip_BOM( void )
{
  /* Skip UTF-8 BOM */
  if ( this->offset + 3 <= this->length &&
       (uint8_t) this->json[ this->offset ] == 0xefU &&
       (uint8_t) this->json[ this->offset + 1 ] == 0xbbU &&
       (uint8_t) this->json[ this->offset + 2 ] == 0xbfU )
    this->offset += 3;
}

int
JsonParser::parse( JsonStreamInput &input )
{
  JsonContext ctx( this );
  JsonOne<JsonStreamInput> one( input, ctx );
  input.skip_BOM();
  return one.parse( this->value );
}

int
JsonParser::parse( JsonBufInput &input )
{
  JsonContext ctx( this );
  JsonOne<JsonBufInput> one( input, ctx );
  input.skip_BOM();
  return one.parse( this->value );
}

static inline bool
is_ident_char( int c ) {
  return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' );
}

static inline bool
is_ident_char2( int c ) {
  return is_ident_char( c ) ||
         ( c >= '0' && c <= '9' ) ||
         ( c == '-' );
}

template<class JsonInput>
int
JsonOne<JsonInput>::parse( JsonValue *&val )
{
  int c = this->input.eat_white();
  int res = 0;
  switch ( c ) {
    case '{': {
      JsonObjectP * obj = this->create_object();
      val = obj;
      res = this->parse_object( *obj );
      break;
    }
    case '[': {
      JsonArrayP * arr = this->create_array();
      val = arr;
      res = this->parse_array( *arr );
      break;
    }
    case '"': {
      JsonStringP * str = this->create_string();
      val = str;
      res = this->parse_string( *str );
      break;
    }
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case '-': case '+': {
      JsonNumberP * num = this->create_number();
      val = num;
      res = this->parse_number( *num );
      break;
    }
    case JSON_EOF:
      val = NULL;
      break;
    case 't':
      if ( this->input.match( 't', 'r', 'u', 'e', 0 ) ) {
        val = this->create_boolean( true );
        break;
      }
    case 'f':
      if ( this->input.match( 'f', 'a', 'l', 's', 'e' ) ) {
        val = this->create_boolean( false );
        break;
      }
    case 'n':
      if ( this->input.match( 'n', 'u', 'l', 'l', 0 ) ) {
        val = this->create_null();
        break;
      }
    default:
      if ( is_ident_char( c ) ) {
        JsonStringP * str = this->create_string();
        val = str;
        res = this->parse_ident( *str );
        break;
      }
      res = Err::INVALID_TOKEN;
      break;
  }
  return res;
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_number( JsonNumberP &num )
{
  char buf[ 64 ];
  size_t n = 0;
  int c = this->input.cur();
  if ( c == '-' || c == '+' ) {
    buf[ n++ ] = (char) c;
    c = this->input.forward();
  }
  int state = 0;
  do {
    switch ( state ) {
      case 0:
        if ( c >= '0' && c <= '9' ) {
        state_continue:
          buf[ n++ ] = (char) c;
          c = this->input.forward();
          break;
        }
        if ( c == '.' ) {
          state = 1; /* number + e */
          buf[ n++ ] = (char) c;
          c = this->input.forward();
          break;
        }
        if ( c == 'e' || c == 'E' ) {
        state_exponent:;
          state = 2; /* number */
          buf[ n++ ] = (char) c;
          c = this->input.forward();
          break;
        }
        goto break_loop;

      case 1: /* state after '.' */
        if ( c >= '0' && c <= '9' )
          goto state_continue;
        if ( c == 'e' || c == 'E' )
          goto state_exponent;
        goto break_loop;

      case 2: /* state exponent */
        if ( c == '+' || c == '-' || ( c >= '0' && c <= '9' ) ) {
          state = 3; /* number */
          buf[ n++ ] = (char) c;
          c = this->input.forward();
          break;
        }
        goto break_loop;

      case 3: /* number following exponent */
        if ( c >= '0' && c <= '9' )
          goto state_continue;
        goto break_loop;
    }
  } while ( n < 64 );
break_loop:;
  num.val.parse( buf, n );
  return 0;
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_array( JsonArrayP &ar )
{
  size_t      sz = 0, tos = 0, i = 0, j;
  JsonValue * value,
           ** val[ 40 ],
           ** end[ 40 ];

  val[ 0 ] = NULL;
  end[ 0 ] = NULL;

  this->input.next(); /* eat '[' */
  int c = this->input.eat_white();
  while ( c != ']' ) {
    int res = this->parse( value );
    if ( res != 0 )
      return res;
    if ( value == NULL )
      return Err::EXPECTING_AR_VALUE;

    if ( tos == 0 || &val[ tos ][ i ] == end[ tos ] ) {
      size_t newsz = ( sz + 2 ) * 3 / 2;
      tos++;
      if ( tos == 40 )
        return Err::TOO_BIG;
      val[ tos ] = (JsonValue **)
                   this->ctx.alloc( sizeof( JsonValue * ) * newsz );
      end[ tos ] = &val[ tos ][ newsz ];
      sz = newsz;
      i  = 0;
    }
    val[ tos ][ i ] = value;
    i++;

    c = this->input.eat_white();
    if ( c != ',' )
      break;
    this->input.next(); /* eat ',' */
    c = this->input.eat_white();
  }
  if ( c != ']' )
    return Err::UNEXPECTED_ARRAY_END;
  this->input.next(); /* eat ']' */

  if ( tos > 0 ) {
    if ( tos == 1 ) {
      ar.val    = val[ 1 ];
      ar.length = i;
    }
    else {
      sz = i;
      for ( j = 1; j < tos; j++ )
        sz += end[ j ] - val[ j ];
      ar.val    = (JsonValue **)
                  this->ctx.alloc( sizeof( JsonValue * ) * sz );
      ar.length = sz;
      sz = 0;
      for ( j = 1; j < tos; j++ ) {
        ::memcpy( &ar.val[ sz ], val[ j ],
                  ( end[ j ] - val[ j ] ) * sizeof( JsonValue * ) );
        sz += end[ j ] - val[ j ];
      }
      ::memcpy( &ar.val[ sz ], val[ tos ], i * sizeof( JsonValue * ) );
    }
  }
  return 0;
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_object( JsonObjectP &obj )
{
  size_t             sz = 0, tos = 0, i = 0, j;
  JsonObject::Pair * val[ 40 ],
                   * end[ 40 ];

  val[ 0 ] = NULL;
  end[ 0 ] = NULL;

  this->input.next(); /* eat '{' */
  int c = this->input.eat_white();
  if ( c != '}' ) {
    while ( c == '"' || is_ident_char( c ) ) {
      JsonStringP   tmp;
      JsonValue   * value;
      int           res;
      if ( c == '"' )
        res = this->parse_string( tmp );
      else
        res = this->parse_ident( tmp );
      if ( res != 0 )
        return res;

      if ( this->input.eat_white() != ':' )
        return Err::EXPECTING_COLON;
      this->input.next(); /* eat ':' */

      res = this->parse( value );
      if ( res != 0 )
        return res;
      if ( value == NULL )
        return Err::EXPECTING_OBJ_VALUE;

      if ( tos == 0 || &val[ tos ][ i ] == end[ tos ] ) {
        size_t newsz = ( sz + 2 ) * 3 / 2;
        tos++;
        if ( tos == 40 )
          return Err::TOO_BIG;
        val[ tos ] = (JsonObject::Pair *)
                     this->ctx.alloc( sizeof( JsonObject::Pair ) * newsz );
        end[ tos ] = &val[ tos ][ newsz ];
        sz = newsz;
        i  = 0;
      }
      val[ tos ][ i ].name = tmp.val; tmp.val = NULL;
      val[ tos ][ i ].val  = value;
      i++;

      c = this->input.eat_white();
      if ( c != ',' )
        break;
      this->input.next(); /* eat ',' */
      c = this->input.eat_white();
    }
  }
  if ( c != '}' )
    return Err::UNEXPECTED_OBJECT_END;
  this->input.next(); /* eat '}' */

  if ( tos > 0 ) {
    if ( tos == 1 ) {
      obj.val    = val[ 1 ];
      obj.length = i;
    }
    else {
      sz = i;
      for ( j = 1; j < tos; j++ )
        sz += end[ j ] - val[ j ];
      obj.val    = (JsonObject::Pair *)
                   this->ctx.alloc( sizeof( JsonObject::Pair ) * sz );
      obj.length = sz;
      sz = 0;
      for ( j = 1; j < tos; j++ ) {
        ::memcpy( &obj.val[ sz ], val[ j ],
                  ( end[ j ] - val[ j ] ) * sizeof( JsonObject::Pair ) );
        sz += end[ j ] - val[ j ];
      }
      ::memcpy( &obj.val[ sz ], val[ tos ], i * sizeof( JsonObject::Pair ) );
    }
  }
  return 0;
}

static inline uint32_t
hex_value( int c )
{
  if ( c >= '0' && c <= '9' )
    return (uint32_t) ( c - '0' );
  if ( c >= 'a' && c <= 'f' )
    return (uint32_t) ( c - 'a' + 10 );
  if ( c >= 'A' && c <= 'F' )
    return (uint32_t) ( c - 'A' + 10 );
  return 0xffU;
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_string( JsonStringP &string )
{
  size_t sz = 8;
  char * str = string.val = (char *) this->ctx.alloc( 8 ),
       * end = &str[ 8 ];

  this->input.next(); /* eat '"' */
  for (;;) {
    int c = this->input.next();
    if ( c == JSON_EOF )
      return Err::UNEXPECTED_STRING_END;
    if ( str == end ) {
      string.val = (char *) this->ctx.extend( string.val, sz, 16 );
      str = &string.val[ sz ];
      sz += 16;
      end = &str[ 16 ];
    }
    if ( c == '"' ) {
      *str = '\0';
      return 0;
    }
    if ( c != '\\' ) {
      *str++ = (char) c;
      continue;
    }

    int b = this->input.next(); /* escaped char */
    switch ( b ) {
      case 'b': *str++ = '\b'; break;
      case 'f': *str++ = '\f'; break;
      case 'n': *str++ = '\n'; break;
      case 'r': *str++ = '\r'; break;
      case 't': *str++ = '\t'; break;
      default:  *str++ = (char) b; break;
      case JSON_EOF: 
        return Err::UNEXPECTED_STRING_END;

      case 'u': { /* format \uXXXX where X = hex nibble */
        uint32_t uc_b1, uc_b2, uc_b3, uc_b4;

        if ( (uc_b1 = hex_value( this->input.next() )) == 0xff ||
             (uc_b2 = hex_value( this->input.next() )) == 0xff ||
             (uc_b3 = hex_value( this->input.next() )) == 0xff ||
             (uc_b4 = hex_value( this->input.next() )) == 0xff )
          return Err::INVALID_CHAR_HEX;

        uint32_t uchar = ( uc_b1 << 12 ) | ( uc_b2 << 8 ) |
                         ( uc_b3 << 4 ) | uc_b4;
        if ( uchar <= 0x7f ) {
          *str++ = (char) uchar;
        }
        else if ( uchar <= 0x7ffU ) {
          if ( &str[ 1 ] == end ) {
            string.val = (char *) this->ctx.extend( string.val, sz, 16 );
            str = &string.val[ sz ];
            sz += 16;
            end = &str[ 16 ];
          }
          *str++ = (char) ( 0xc0U | (   uchar            >> 6 ) );
          *str++ = (char) ( 0x80U |   ( uchar & 0x03fU ) );
        }
        else {
          if ( &str[ 2 ] >= end ) {
            string.val = (char *) this->ctx.extend( string.val, sz, 16 );
            str = &string.val[ sz ];
            sz += 16;
            end = &str[ 16 ];
          }
          *str++ = (char) ( 0xe0U | (   uchar              >> 12 ) );
          *str++ = (char) ( 0x80U | ( ( uchar & 0x00fc0U ) >> 6 ) );
          *str++ = (char) ( 0x80U |   ( uchar & 0x0003fU ) );
        }
        break;
      }
    }
  }
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_ident( JsonStringP &string )
{
  size_t sz = 8;
  char * str = string.val = (char *) this->ctx.alloc( 8 ),
       * end = &str[ 8 ];
  *str++ = this->input.cur();
  for (;;) {
    if ( str == end ) {
      string.val = (char *) this->ctx.extend( string.val, sz, 8 );
      str = &string.val[ sz ];
      sz += 8;
      end = &str[ 8 ];
    }
    int c = this->input.forward(); /* eat a-zA-Z_ */
    if ( ! is_ident_char2( c ) ) {
      *str = '\0';
      return 0;
    }
    *str++ = c;
  }
}

int
JsonValue::print( MDOutput *out )
{
  switch ( this->type ) {
    default:           return ((JsonValueP *) this)->print( out );
    case JSON_OBJECT:  return ((JsonObjectP *) this)->print( out );
    case JSON_ARRAY:   return ((JsonArrayP *) this)->print( out );
    case JSON_NUMBER:  return ((JsonNumberP *) this)->print( out );
    case JSON_STRING:  return ((JsonStringP *) this)->print( out );
    case JSON_BOOLEAN: return ((JsonBooleanP *) this)->print( out );
  }
}

int
JsonValueP::print( MDOutput *out )
{
  return out->puts( "null" );
}

int
JsonBooleanP::print( MDOutput *out )
{
  return out->puts( this->val ? "true" : "false" );
}

int
JsonNumberP::print( MDOutput *out )
{
  char buf[ 64 ];
  size_t n = this->val.get_string( buf, sizeof( buf ) );
  buf[ n ] = '\0';
  return out->puts( buf );
}

int
JsonArrayP::print( MDOutput *out )
{
  int n = out->puts( "[" );
  if ( this->length > 0 )
    n += this->val[ 0 ]->print( out );
  for ( size_t i = 1; i < this->length; i++ ) {
    n += out->puts( "," );
    n += this->val[ i ]->print( out );
  }
  return n + out->puts( "]" );
}

int
JsonObjectP::print( MDOutput *out )
{
  int n = out->puts( "{" );
  if ( this->length > 0 ) {
    n += out->printf( "\"%s\":", this->val[ 0 ].name );
    n += this->val[ 0 ].val->print( out );
  }
  for ( size_t i = 1; i < this->length; i++ ) {
    n += out->puts( "," );
    n += out->printf( "\"%s\":", this->val[ i ].name );
    n += this->val[ i ].val->print( out );
  }
  return n + out->puts( "}" );
}

int
JsonStringP::print( MDOutput *out )
{
  return out->printf( "\"%s\"", this->val );
}

