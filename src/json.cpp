#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
/*#include <unistd.h>*/

#include <raimd/json.h>
#include <raimd/md_msg.h>

/*using namespace rai;
using namespace md;*/

namespace rai {
namespace md {

int
JsonValue::to_double( double &v ) const noexcept {
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
JsonValue::to_int( int64_t &v ) const noexcept {
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
JsonObject::find( const char *name ) const noexcept {
  size_t len = ::strlen( name );
  for ( size_t i = 0; i < this->length; i++ )
    if ( this->val[ i ].name.length == len &&
         ::memcmp( name, this->val[ i ].name.val, len ) == 0 )
      return this->val[ i ].val;
  return NULL;
}

struct JsonContext {
  JsonParser & parser;
  JsonValue  * null_val;

  JsonContext( JsonParser &p ) : parser( p ), null_val( 0 ) {}
  JsonContext( JsonContext &c ) : parser( c.parser ), null_val( c.null_val ) {}

  void *alloc( size_t sz ) {
    void * p = NULL;
    this->parser.mem.alloc( sz, &p );
    return p;
  }
  void *extend( void *p,  size_t cursz,  size_t addsz ) {
    this->parser.mem.extend( cursz, cursz + addsz, &p );
    return p;
  }

  JsonBoolean *create_boolean( bool v ) {
    return new ( this->alloc( sizeof( JsonBoolean ) ) ) JsonBoolean( v );
  }
  JsonNumber *create_number( void ) {
    return new ( this->alloc( sizeof( JsonNumber ) ) ) JsonNumber();
  }
  JsonString *create_string( void ) {
    return new ( this->alloc( sizeof( JsonString ) ) ) JsonString();
  }
  JsonObject *create_object( void ) {
    return new ( this->alloc( sizeof( JsonObject ) ) ) JsonObject();
  }
  JsonArray *create_array( void ) {
    return new ( this->alloc( sizeof( JsonArray ) ) ) JsonArray();
  }
  JsonValue *create_null( void ) {
    if ( this->null_val == NULL )
      this->null_val = new ( this->alloc( sizeof( JsonValue ) ) ) JsonValue();
    return this->null_val;
  }
  void extend_array( JsonArray &ar,  JsonValue *v ) noexcept;
  void extend_object( JsonObject &c,  JsonString &field,
                      JsonValue *val ) noexcept;
  void extend_object( JsonObject &c,  JsonObject &d ) noexcept;

  void make_string( JsonString &str,  const char *v,  size_t len ) noexcept;

  JsonString * create_string( const char *v,  size_t len ) noexcept;
  JsonString * create_string( JsonString &s ) noexcept;
  void         concat_string( JsonString &s,  const char *v,  size_t len ) noexcept;
  void         concat_string( JsonString &s,  JsonString &t ) noexcept;
  JsonArray  * create_array( JsonValue *v ) noexcept;
  JsonObject * create_object( JsonString &field,  JsonValue *val ) noexcept;
  int split_field( const char *line,  size_t len,  JsonString &field,
                   JsonValue *&x ) noexcept;
};

template <class JsonInput>
struct JsonOne : public JsonContext {
  JsonInput & input;

  JsonOne( JsonInput &in,  JsonParser &p ) : JsonContext( p ), input( in ) {}
  JsonOne( JsonInput &in,  JsonContext &c ) : JsonContext( c ), input( in ) {}

  int parse( JsonValue *&val ) noexcept;
  int parse_number( JsonNumber &num ) noexcept;
  int parse_array( JsonArray &ar ) noexcept;
  int parse_object( JsonObject &obj ) noexcept;
  int parse_string( JsonString &string ) noexcept;
  int parse_ident( JsonString &string ) noexcept;
  int parse_yaml( JsonValue *&val ) noexcept;
};

size_t
JsonStreamInput::read( uint8_t *buf,  size_t len ) noexcept
{
  return ::fread( buf, 1, len, stdin );
}

bool
JsonStreamInput::fill_buf( void ) noexcept
{
  size_t len = 0;
  if ( ! this->is_eof ) {
    if ( this->line_start == 0 && this->length == this->buf_len ) {
      if ( this->json == this->buf ) {
        this->json = (char *) ::malloc( this->buf_len * 2 );
        ::memcpy( this->json, this->buf, this->length );
      }
      else {
        this->json = (char *) ::realloc( this->json, this->buf_len * 2 );
      }
      this->buf_len *= 2;
    }
    else {
      ::memmove( this->json, &this->json[ this->line_start ],
                 this->length - this->line_start );
      this->length -= this->line_start;
      this->offset -= this->line_start;
      this->line_start = 0;
    }
    len = this->read( (uint8_t *) &this->json[ this->length ],
                      this->buf_len - this->length );
    if ( len == 0 )
      this->is_eof = true;
    this->length += len;
  }
  return len > 0;
}

bool
JsonStreamInput::match( char c1,  char c2,  char c3,  char c4,
                        char c5 ) noexcept
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

bool
JsonStreamInput::lookahead_case( char c1,  char c2,  char c3 ) noexcept
{
  for (;;) {
    if ( this->offset + 3 <= this->length &&
         c1 == tolower( this->json[ this->offset ] ) &&
         c2 == tolower( this->json[ this->offset + 1 ] ) &&
         c3 == tolower( this->json[ this->offset + 2 ] ) ) {

      if ( ( this->offset + 3 == this->length && this->is_eof ) ||
           isspace( this->json[ this->offset + 3 ] ) ||
           ispunct( this->json[ this->offset + 3 ] ) )
        return true;
    }
    if ( this->offset + 4 < this->length || this->is_eof || ! this->fill_buf() )
      return false;
  }
}

int
JsonStreamInput::eat_white( void ) noexcept
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
JsonStreamInput::skip_BOM( void ) noexcept
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
JsonBufInput::eat_white( void ) noexcept
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
JsonBufInput::match( char c1,  char c2,  char c3,  char c4,  char c5 ) noexcept
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

bool
JsonBufInput::lookahead_case( char c1,  char c2,  char c3 ) noexcept
{
  if ( this->offset + 3 <= this->length &&
       c1 == tolower( this->json[ this->offset ] ) &&
       c2 == tolower( this->json[ this->offset + 1 ] ) &&
       c3 == tolower( this->json[ this->offset + 2 ] ) ) {

    if ( this->offset + 3 == this->length ||
         isspace( this->json[ this->offset + 3 ] ) ||
         ispunct( this->json[ this->offset + 3 ] ) )
      return true;
  }
  return false;
}

void
JsonBufInput::skip_BOM( void ) noexcept
{
  /* Skip UTF-8 BOM */
  if ( this->offset + 3 <= this->length &&
       (uint8_t) this->json[ this->offset ] == 0xefU &&
       (uint8_t) this->json[ this->offset + 1 ] == 0xbbU &&
       (uint8_t) this->json[ this->offset + 2 ] == 0xbfU )
    this->offset += 3;
}

int
JsonParser::parse( JsonStreamInput &input ) noexcept
{
  JsonOne<JsonStreamInput> one( input, *this );
  input.skip_BOM();
  return one.parse( this->value );
}

int
JsonParser::parse( JsonBufInput &input ) noexcept
{
  JsonOne<JsonBufInput> one( input, *this );
  input.skip_BOM();
  return one.parse( this->value );
}

int
JsonParser::parse_yaml( JsonStreamInput &input ) noexcept
{
  JsonOne<JsonStreamInput> one( input, *this );
  input.skip_BOM();
  return one.parse_yaml( this->value );
}

int
JsonParser::parse_yaml( JsonBufInput &input ) noexcept
{
  JsonOne<JsonBufInput> one( input, *this );
  input.skip_BOM();
  return one.parse_yaml( this->value );
}

static inline bool
is_ident_char( int c ) {
  return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' );
}

static inline bool
is_ident_char2( int c ) {
  return is_ident_char( c ) ||
         ( c >= '0' && c <= '9' ) ||
         ( c == '-' || c == '.' );
}

void
JsonContext::extend_array( JsonArray &ar,  JsonValue *v ) noexcept
{
  JsonValue ** p   = ar.val;
  size_t       len = ar.length;
  ar.val = (JsonValue **) this->alloc( sizeof( p[ 0 ] ) * ( len + 1 ) );
  ::memcpy( ar.val, p, sizeof( p[ 0 ] ) * len );
  ar.val[ len ] = v;
  ar.length = len + 1;
}

JsonArray *
JsonContext::create_array( JsonValue *v ) noexcept
{
  JsonArray * ar = this->create_array();
  if ( v != NULL )
    this->extend_array( *ar, v );
  return ar;
}

void
JsonContext::make_string( JsonString &str,  const char *v,  size_t len ) noexcept
{
  size_t sz = ( len + 7 ) & ~7;
  str.type   = JSON_STRING;
  str.length = len;
  str.val    = (char *) this->alloc( sz );
  ::memcpy( str.val, v, len );
}

JsonString *
JsonContext::create_string( const char *v,  size_t len ) noexcept
{
  JsonString * str = this->create_string();
  this->make_string( *str, v, len );
  return str;
}

JsonString *
JsonContext::create_string( JsonString &s ) noexcept
{
  JsonString * str = this->create_string();
  str->set( s );
  return str;
}

void
JsonContext::concat_string( JsonString &s,  const char *v,  size_t len ) noexcept
{
  size_t sz = ( s.length + len + 7 ) & ~7;
  s.type    = JSON_STRING;
  s.val     = (char *) this->extend( s.val, ( s.length + 7 ) & ~7, sz );
  ::memcpy( &s.val[ s.length ], v, len );
  s.length += len;
}

void
JsonContext::concat_string( JsonString &s,  JsonString &t ) noexcept
{
  this->concat_string( s, t.val, t.length );
}

void
JsonContext::extend_object( JsonObject &c,  JsonString &field,
                            JsonValue *val ) noexcept
{
  JsonObject::Pair * p = c.val;
  size_t             len = c.length;
  c.val = (JsonObject::Pair *) this->alloc( sizeof( p[ 0 ] ) * ( len + 1 ) );
  ::memcpy( c.val, p, sizeof( p[ 0 ] ) * len );
  c.val[ len ].val = val;
  c.val[ len ].name.set( field );
  c.length = len + 1;
}

void
JsonContext::extend_object( JsonObject &c,  JsonObject &d ) noexcept
{
  JsonObject::Pair * p = c.val;
  size_t             len = c.length + d.length;
  c.val = (JsonObject::Pair *) this->alloc( sizeof( p[ 0 ] ) * len );
  ::memcpy( c.val, p, sizeof( p[ 0 ] ) * c.length );
  ::memcpy( &c.val[ c.length ], d.val, sizeof( p[ 0 ] ) * d.length );
  c.length = len;
}

JsonObject *
JsonContext::create_object( JsonString &field,  JsonValue *val ) noexcept
{
  JsonObject * obj = this->create_object();
  this->extend_object( *obj, field, val );
  return obj;
}

enum YamlLine {
  FIELD_OBJECT, /* field: something */
  STRING_UNIT,  /* "string" */
  ARRAY_UNIT,   /* [array] */
  OBJECT_UNIT,  /* {object} */
  EMPTY_LINE    /* <emtpy> */
};

int
JsonContext::split_field( const char *line,  size_t len,  JsonString &field,
                          JsonValue *&x ) noexcept
{
  const char * eol  = &line[ len ],
             * data = NULL;
  int res;

  if ( line[ 0 ] == '\"' || line[ 0 ] == '\'' ||
       line[ 0 ] == '[' || line[ 0 ] == '{' ) {
    JsonBufInput sinput( line, 0, len );
    JsonOne<JsonBufInput> p( sinput, *this );
    if ( line[ 0 ] == '\"' || line[ 0 ] == '\'' ) {
      res = p.parse_string( field );
      if ( res == 0 ) {
        if ( sinput.cur() == ':' ) {
          if ( sinput.forward() == ' ' )
            data = &sinput.json[ sinput.offset + 1 ];
          else {
            x = this->create_null();
            return FIELD_OBJECT; /* "field": */
          }
        }
        else {
          return STRING_UNIT; /* "string" */
        }
      }
    }
    else if ( line[ 0 ] == '[' ) {
      JsonArray * ar = this->create_array();
      res = p.parse_array( *ar );
      if ( res == 0 ) {
        x = ar;
        return ARRAY_UNIT;
      }
    }
    else { /* line[ 0 ] == '{' */
      JsonObject * o = this->create_object();
      res = p.parse_object( *o );
      if ( res == 0 ) {
        x = o;
        return OBJECT_UNIT;
      }
    }
    if ( data == NULL )
      goto check_space;
  }
  else {
    const char * colon = (const char *) ::memchr( line, ':', len );
    if ( colon == NULL )
      goto check_space;
    this->make_string( field, line, colon - line );
    data = colon + 1;
    if ( data < eol && data[ 0 ] == ' ' )
      data++;
  }
  if ( data == eol )
    x = this->create_null();
  else {
    if ( data[ 0 ] == '\"' || data[ 0 ] == '\'' ||
         data[ 0 ] == '[' || data[ 0 ] == '{' ) {
      JsonBufInput sinput( data, 0, eol - data );
      JsonOne<JsonBufInput> p( sinput, *this );

      if ( data[ 0 ] == '\"' || data[ 0 ] == '\'' ) {
        x = this->create_string();
        res = p.parse_string( (JsonString &) *x );
      }
      else if ( line[ 0 ] == '[' ) {
        JsonArray * ar = this->create_array();
        res = p.parse_array( *ar );
        if ( res == 0 )
          x = ar;
        else
          x = NULL;
      }
      else { /* line[ 0 ] == '{' */
        JsonObject * o = this->create_object();
        res = p.parse_object( *o );
        if ( res == 0 )
          x = o;
        else
          x = NULL;
      }
      if ( res != 0 ) {
        if ( x == NULL )
          x = this->create_string();
        this->make_string( (JsonString &) *x, data, eol - data );
      }
    }
    else {
      x = this->create_string( data, eol - data );
    }
  }
  return FIELD_OBJECT; /* field: something|nothing */

check_space:;
  size_t i;
  for ( i = 0; i < len; i++ )
    if ( ! isspace( line[ i ] ) )
      break;
  if ( i == len ) /* empty line */
    return EMPTY_LINE;
  this->make_string( field, line, len );
  return STRING_UNIT;
}

struct YamlStack {
  JsonContext &ctx;
  struct Stack {
    size_t      indent;
    JsonValue * val;
  } stk[ 40 ];
  size_t tos;

  YamlStack( JsonContext &c ) : ctx( c ), tos( 0 ) {}
  int push_array( size_t i,  JsonValue *v ) noexcept;
  int push_field( size_t i,  JsonString &field,  JsonValue *v ) noexcept;
  int push_object( size_t i,  JsonValue *v ) noexcept;
  int collapse( void ) noexcept;
  int append_array( size_t i,  JsonValue *v ) noexcept;
  int append_field( size_t i,  JsonString &field, JsonValue *v ) noexcept;
  int append_string( size_t i,  JsonString &str ) noexcept;
  int concat_string( JsonValue *&v,  JsonString &str ) noexcept;
  int append_object( size_t i,  JsonValue *v ) noexcept;
};

int
YamlStack::push_array( size_t i,  JsonValue *v ) noexcept
{
  if ( this->tos == 40 )
    return Err::NO_SPACE;
  Stack & s = this->stk[ this->tos++ ];
  s.indent = i;
  s.val    = this->ctx.create_array( v );
  return 0;
}

int
YamlStack::push_field( size_t i,  JsonString &field,  JsonValue *v ) noexcept
{
  if ( this->tos == 40 )
    return Err::NO_SPACE;
  Stack & s = this->stk[ this->tos++ ];
  s.indent = i;
  s.val    = this->ctx.create_object( field, v );
  return 0;
}

int
YamlStack::push_object( size_t i,  JsonValue *v ) noexcept
{
  if ( this->tos == 40 )
    return Err::NO_SPACE;
  Stack & s = this->stk[ this->tos++ ];
  s.indent = i;
  s.val    = v;
  return 0;
}

int
YamlStack::collapse( void ) noexcept
{
  Stack & t = this->stk[ this->tos - 1 ],
        & s = this->stk[ this->tos - 2 ];
  this->tos -= 1;
  if ( s.val->type == JSON_OBJECT ) {
    JsonObject *o = (JsonObject *) s.val;
    o->val[ o->length - 1 ].val = t.val;
  }
  else if ( s.val->type == JSON_ARRAY ) {
    JsonArray *a = (JsonArray *) s.val;
    this->ctx.extend_array( *a, t.val );
  }
  else {
    return Err::BAD_FORMAT;
  }
  return 0;
}

int
YamlStack::append_object( size_t i,  JsonValue *v ) noexcept
{
  size_t indent;
  int res;
  if ( this->tos == 0 || i > (indent = this->stk[ this->tos - 1 ].indent) )
    return this->push_object( i, v );

  while ( i < indent ) {
    if ( (res = this->collapse()) != 0 )
      return res;
    indent = this->stk[ this->tos - 1 ].indent;
  }

  JsonValue *top = this->stk[ this->tos - 1 ].val;
  if ( top->type == JSON_ARRAY ) {
    this->ctx.extend_array( *(JsonArray *) top, v );
    return 0;
  }
  if ( top->type == JSON_OBJECT && v->type == JSON_OBJECT ) {
    this->ctx.extend_object( *(JsonObject *) top, *(JsonObject *) v );
    return 0;
  }
  return Err::BAD_FORMAT;
}

int
YamlStack::append_array( size_t i,  JsonValue *v ) noexcept
{
  size_t indent;
  int res;
  if ( this->tos == 0 || i > (indent = this->stk[ this->tos - 1 ].indent) )
    return this->push_array( i, v );

  while ( i < indent ) {
    if ( (res = this->collapse()) != 0 )
      return res;
    indent = this->stk[ this->tos - 1 ].indent;
  }

  JsonValue *top = this->stk[ this->tos - 1 ].val;
  if ( top->type != JSON_ARRAY )
    return Err::BAD_FORMAT;
  if ( v != NULL )
    this->ctx.extend_array( *(JsonArray *) top, v );
  return 0;
}

int
YamlStack::append_field( size_t i,  JsonString &field,  JsonValue *v ) noexcept
{
  size_t indent;
  int res;
  if ( this->tos == 0 || i > (indent = this->stk[ this->tos - 1 ].indent) )
    return this->push_field( i, field, v );

  while ( i < indent ) {
    if ( (res = this->collapse()) != 0 )
      return res;
    indent = this->stk[ this->tos - 1 ].indent;
  }

  JsonValue *top = this->stk[ this->tos - 1 ].val;
  if ( top->type != JSON_OBJECT )
    return Err::BAD_FORMAT;
  this->ctx.extend_object( *(JsonObject *) top, field, v );
  return 0;
}

int
YamlStack::concat_string( JsonValue *&v,  JsonString &str ) noexcept
{
  if ( v->type == JSON_STRING ) {
    this->ctx.concat_string( (JsonString &) *v, str );
    return 0;
  }
  if ( v->type == JSON_NULL ) {
    v = this->ctx.create_string( str );
    return 0;
  }
  return Err::BAD_FORMAT;
}

int
YamlStack::append_string( size_t,  JsonString &str ) noexcept
{
  JsonValue * v;
  if ( this->tos == 0 )
    return Err::BAD_FORMAT;
  JsonValue * top = this->stk[ this->tos - 1 ].val;
  if ( top->type == JSON_ARRAY ) {
    JsonArray * ar = (JsonArray *) top;
    if ( ar->length == 0 ) {
      v = this->ctx.create_string( str );
      this->ctx.extend_array( *ar, v );
      return 0;
    }
    return this->concat_string( ar->val[ ar->length - 1 ], str );
  }
  if ( top->type == JSON_OBJECT ) {
    JsonObject * o = (JsonObject *) top;
    if ( o->length > 0 )
      return this->concat_string( o->val[ o->length - 1 ].val, str );
  }
  return Err::BAD_FORMAT;
}

template<class JsonInput>
int
JsonOne<JsonInput>::parse_yaml( JsonValue *&val ) noexcept
{
  YamlStack    stk( *this );
  int          res  = 0;
  const char * line = this->input.line_ptr(),
             * end  = this->input.end_ptr(),
             * eol;
  val = NULL;
  for (;;) {
    while ( line == end ||
            (eol = (const char *) ::memchr( line, '\n', end - line )) == NULL ) {
      if ( ! this->input.fill_buf() )
        goto break_loop;
      line = this->input.line_ptr();
      end  = this->input.end_ptr();
    }
    size_t len         = eol - line,
           line_offset = this->input.line_start;
    JsonValue  * x,
               * y;
    JsonString   field;

    for ( size_t i = 0; ; ) {
      if ( i == len || line[ i ] == '#' )
        break;
      if ( isspace( line[ i ] ) ) {
        i++;
        continue;
      }
      x = y = NULL;
      field.zero();
      /* list element */
      if ( line[ i ] == '-' && line[ i + 1 ] == ' ' ) {
        size_t j;
        j  = i;
        i += 2;
        /* multiple arrays: - - - */
        while ( line[ i ] == '-' && line[ i + 1 ] == ' ' )
          i += 2;
        /* multiple spaces after '-' */
        if ( i == len )
          x = this->create_null();
        else {
          /* res == 0, empty, res = 1, string, res = 2 object */
          res = this->split_field( &line[ i ], len - i, field, y );
          if ( res == EMPTY_LINE ) {
            x = this->create_null();
          }
          else if ( res == STRING_UNIT ) {
            x = this->create_string();
            ((JsonString *) x)->set( field );
          }
          else if ( res == FIELD_OBJECT ) {
            x = NULL; /* object is first */
          }
          else if ( res == ARRAY_UNIT || res == OBJECT_UNIT ) {
            x = y;
            y = NULL;
          }
        }
        for ( ; j < i; j += 2 ) {
          JsonValue * v = ( j + 2 == i ? x : NULL );
          if ( (res = stk.append_array( j, v )) != 0 )
            return res;
        }
        if ( y != NULL ) {
          if ( (res = stk.append_field( i, field, y )) != 0 )
            return res;
        }
        break;
      }
      /* field element */
      else {
        res = this->split_field( &line[ i ], len - i, field, x );
        if ( res == FIELD_OBJECT ) {
          if ( (res = stk.append_field( i, field, x )) != 0 )
            return res;
        }
        else if ( res == STRING_UNIT ) {
          res = stk.append_string( i, field );
        }
        else if ( res != EMPTY_LINE ) {
          res = stk.append_object( i, x );
        }
        break;
      }
    }
    this->input.offset = line_offset + len + 1;
    this->input.line_start = this->input.offset;
    line = &eol[ 1 ];
  }
break_loop:;
  while ( stk.tos > 1 )
    if ( (res = stk.collapse()) != 0 )
      return res;
  if ( stk.tos == 1 )
    val = stk.stk[ 0 ].val;
  return 0;
}

template<class JsonInput>
int
JsonOne<JsonInput>::parse( JsonValue *&val ) noexcept
{
  int c = this->input.eat_white();
  int res = 0;
  switch ( c ) {
    case '{': {
      JsonObject * obj = this->create_object();
      val = obj;
      res = this->parse_object( *obj );
      break;
    }
    case '[': {
      JsonArray * arr = this->create_array();
      val = arr;
      res = this->parse_array( *arr );
      break;
    }
    case '"': {
      JsonString * str = this->create_string();
      val = str;
      res = this->parse_string( *str );
      break;
    }
    case '0': case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case '-': case '+': {
      JsonNumber * num = this->create_number();
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
      /* FALLTHRU */
    case 'f':
      if ( this->input.match( 'f', 'a', 'l', 's', 'e' ) ) {
        val = this->create_boolean( false );
        break;
      }
      /* FALLTHRU */
    case 'n':
      if ( this->input.match( 'n', 'u', 'l', 'l', 0 ) ) {
        val = this->create_null();
        break;
      }
      /* FALLTHRU */
    case 'i':
      if ( this->input.lookahead_case( 'i', 'n', 'f' ) ||
           this->input.lookahead_case( 'n', 'a', 'n' ) ) {
        JsonNumber * num = this->create_number();
        num->val.parse( &this->input.json[ this->input.offset ], 3 );
        val = num;
        this->input.offset += 3;
        res = 0;
        break;
      }
      /* FALLTHRU */
    default:
      if ( is_ident_char( c ) ) {
        JsonString * str = this->create_string();
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
JsonOne<JsonInput>::parse_number( JsonNumber &num ) noexcept
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
  if ( n <= 1 && ( c == 'i' || c == 'I' || c == 'n' || c == 'N' ) ) {
    if ( this->input.lookahead_case( 'i', 'n', 'f' ) ||
         this->input.lookahead_case( 'n', 'a', 'n' ) ) {
      buf[ n++ ] = c; c = this->input.forward();
      buf[ n++ ] = c; c = this->input.forward();
      buf[ n++ ] = c; c = this->input.forward();
    }
  }
  num.val.parse( buf, n );
  return 0;
}

template <class JsonInput>
int
JsonOne<JsonInput>::parse_array( JsonArray &ar ) noexcept
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
    if ( value == NULL ) {
      if ( this->input.cur() == JSON_EOF )
        return Err::UNEXPECTED_ARRAY_END;
      return Err::EXPECTING_AR_VALUE;
    }
    if ( tos == 0 || &val[ tos ][ i ] == end[ tos ] ) {
      size_t newsz = ( sz + 2 ) * 3 / 2;
      tos++;
      if ( tos == 40 )
        return Err::TOO_BIG;
      val[ tos ] = (JsonValue **)
                   this->alloc( sizeof( JsonValue * ) * newsz );
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
                  this->alloc( sizeof( JsonValue * ) * sz );
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
JsonOne<JsonInput>::parse_object( JsonObject &obj ) noexcept
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
      JsonString  tmp;
      JsonValue * value;
      int         res;
      if ( c == '"' )
        res = this->parse_string( tmp );
      else
        res = this->parse_ident( tmp );
      if ( res != 0 )
        return res;

      c = this->input.eat_white();
      if ( c != ':' ) {
        if ( c == JSON_EOF )
          return Err::UNEXPECTED_OBJECT_END;
        return Err::EXPECTING_COLON;
      }
      this->input.next(); /* eat ':' */

      res = this->parse( value );
      if ( res != 0 )
        return res;
      if ( value == NULL ) {
        if ( this->input.cur() == JSON_EOF )
          return Err::UNEXPECTED_OBJECT_END;
        return Err::EXPECTING_OBJ_VALUE;
      }
      if ( tos == 0 || &val[ tos ][ i ] == end[ tos ] ) {
        size_t newsz = ( sz + 2 ) * 3 / 2;
        tos++;
        if ( tos == 40 )
          return Err::TOO_BIG;
        val[ tos ] = (JsonObject::Pair *)
                     this->alloc( sizeof( JsonObject::Pair ) * newsz );
        end[ tos ] = &val[ tos ][ newsz ];
        sz = newsz;
        i  = 0;
      }
      val[ tos ][ i ].name.set( tmp );
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
                   this->alloc( sizeof( JsonObject::Pair ) * sz );
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
JsonOne<JsonInput>::parse_string( JsonString &string ) noexcept
{
  size_t sz  = ( string.length + 7 ) & ~7,
         len = string.length;
  char * str = string.val;
  int    quote;

  if ( sz == len ) {
    if ( sz == 0 )
      str = (char *) this->alloc( 8 );
    else
      str = (char *) this->extend( str, sz, 8 );
    sz += 8;
  }
  if ( len > 0 )
    str[ len++ ] = ' '; /* concat existing */

  quote = this->input.next(); /* eat '"' */
  for (;;) {
    int c = this->input.next();
    if ( c == JSON_EOF )
      return Err::UNEXPECTED_STRING_END;
    if ( sz == len ) {
      str = (char *) this->extend( str, sz, 8 );
      sz += 8;
    }
    if ( c == quote ) {
      str[ len ]    = '\0';
      string.val    = str;
      string.length = len;
      return 0;
    }
    if ( c != '\\' ) {
      str[ len++ ] = (char) c;
      continue;
    }

    int b = this->input.next(); /* escaped char */
    switch ( b ) {
      case 'b': str[ len++ ] = '\b'; break;
      case 'f': str[ len++ ] = '\f'; break;
      case 'n': str[ len++ ] = '\n'; break;
      case 'r': str[ len++ ] = '\r'; break;
      case 't': str[ len++ ] = '\t'; break;
      default:  str[ len++ ] = (char) b; break;
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
          str[ len++ ] = (char) uchar;
        }
        else if ( uchar <= 0x7ffU ) {
          if ( &str[ len + 1 ] >= &str[ sz ] ) {
            str = (char *) this->extend( str, sz, 16 );
            sz += 16;
          }
          str[ len++ ] = (char) ( 0xc0U | (   uchar            >> 6 ) );
          str[ len++ ] = (char) ( 0x80U |   ( uchar & 0x03fU ) );
        }
        else {
          if ( &str[ len + 2 ] >= &str[ sz ] ) {
            str = (char *) this->extend( str, sz, 16 );
            sz += 16;
          }
          str[ len++ ] = (char) ( 0xe0U | (   uchar              >> 12 ) );
          str[ len++ ] = (char) ( 0x80U | ( ( uchar & 0x00fc0U ) >> 6 ) );
          str[ len++ ] = (char) ( 0x80U |   ( uchar & 0x0003fU ) );
        }
        break;
      }
    }
  }
}

template <>
int
JsonOne<JsonBufInput>::parse_ident( JsonString &string ) noexcept
{
  size_t       len       = 1,
               input_len = this->input.length,
               input_off = this->input.offset;
  const char * str       = &this->input.json[ input_off ];

  for (;;) {
    if ( ++input_off == input_len || ! is_ident_char2( str[ len ] ) )
      goto end_of_ident;
    len++;
  }
end_of_ident:;
  string.val    = (char *) str;
  string.length = len;
  this->input.offset = input_off;
  return 0;
}

template <>
int
JsonOne<JsonStreamInput>::parse_ident( JsonString &string ) noexcept
{
  size_t sz  = 8,
         len = 0;
  char * str = (char *) this->alloc( 8 );
  int    i, c;

  c = this->input.cur();
  for ( i = 0; i < 8; i++ ) {
    str[ len++ ] = c;
    c = this->input.forward(); /* eat a-zA-Z_ */
    if ( ! is_ident_char2( c ) )
      goto end_of_ident;
  }
  for (;;) {
    str = (char *) this->extend( str, sz, 8 );
    sz += 8;
    for ( i = 0; i < 8; i++ ) {
      str[ len++ ] = c;
      c = this->input.forward(); /* eat a-zA-Z_ */
      if ( ! is_ident_char2( c ) )
        goto end_of_ident;
    }
  }
end_of_ident:;
  str[ len ]    = '\0';
  string.val    = str;
  string.length = len;
  return 0;
}

int
JsonValue::print( MDOutput *out ) noexcept
{
  switch ( this->type ) {
    default:
    case JSON_NULL:    return out->puts( "null" );
    case JSON_OBJECT:  return ((JsonObject *) this)->print( out );
    case JSON_ARRAY:   return ((JsonArray *) this)->print( out );
    case JSON_NUMBER:  return ((JsonNumber *) this)->print( out );
    case JSON_STRING:  return ((JsonString *) this)->print( out );
    case JSON_BOOLEAN: return ((JsonBoolean *) this)->print( out );
  }
}

int
JsonBoolean::print( MDOutput *out ) noexcept
{
  return out->puts( this->val ? "true" : "false" );
}

int
JsonNumber::print( MDOutput *out ) noexcept
{
  char buf[ 64 ];
  size_t n = this->val.get_string( buf, sizeof( buf ) );
  buf[ n ] = '\0';
  return out->puts( buf );
}

int
JsonArray::print( MDOutput *out ) noexcept
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
JsonObject::print( MDOutput *out ) noexcept
{
  int n = out->puts( "{" );
  if ( this->length > 0 ) {
    n += this->val[ 0 ].name.print( out );
    n += out->puts( ":" );
    n += this->val[ 0 ].val->print( out );
  }
  for ( size_t i = 1; i < this->length; i++ ) {
    n += out->puts( "," );
    n += this->val[ i ].name.print( out );
    n += out->puts( ":" );
    n += this->val[ i ].val->print( out );
  }
  return n + out->puts( "}" );
}

int
JsonString::print( MDOutput *out ) noexcept
{
  return out->printf( "\"%.*s\"", (int) this->length, this->val );
}

}
} // namespace rai

