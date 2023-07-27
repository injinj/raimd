#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/enum_def.h>

using namespace rai;
using namespace md;

void
EnumDef::clear_enum( void ) noexcept
{
  while ( ! this->acro.is_empty() )
    delete this->acro.pop();
  while ( ! this->map.is_empty() )
    delete this->map.pop();
  this->max_len   = 0;
  this->max_value = 0;
  this->value_cnt = 0;
}

void
EnumDef::define_enum( MDDictBuild &dict_build ) noexcept
{
  EnumValue * v;
  size_t      i,
              map_sz,
              value_cnt;
  uint8_t   * p;
  uint8_t   * map   = NULL;
  uint16_t  * value = NULL;
#if 0
  for ( v = this->acro.hd; v != NULL; v = v->next )
    dict_build.add_entry( v->value, this->map_num, MD_ENUM, 0,
                          v->str, NULL, NULL, this->fname, v->lineno );
#endif
  for ( v = this->acro.hd; v != NULL; v = v->next )
    dict_build.update_entry_enum( v->value, this->map_num + 1, this->max_len );
  v = this->map.hd;
  if ( (size_t) this->max_value + 1 < this->value_cnt * 2 ) {
    value_cnt = (size_t) this->max_value + 1;
    map_sz    = value_cnt * this->max_len;
    map       = (uint8_t *) ::malloc( map_sz );
    ::memset( map, ' ', map_sz );
    while ( v != NULL ) {
      i = (size_t) v->value * this->max_len;
      p = &map[ i ];
      ::memcpy( p, v->str, v->len );
      v = v->next;
    }
  }
  else {
    value_cnt = this->value_cnt;
    map_sz    = value_cnt * this->max_len;
    map       = (uint8_t *) ::malloc( map_sz );
    value     = (uint16_t *) ::malloc( value_cnt * sizeof( uint16_t ) );
    for ( i = 0; v != NULL; i++ ) {
      p = &map[ i * this->max_len ];
      ::memcpy( p, v->str, v->len );
      value[ i ] = v->value;
      v = v->next;
    }
  }
  MDEnumAdd a;
  a.map_num   = this->map_num + 1;
  a.max_value = this->max_value;
  a.value_cnt = value_cnt;
  a.value     = value;
  a.max_len   = this->max_len;
  a.map       = map;
  dict_build.add_enum_map( a );
  /*
  dict_build.add_enum_map( this->map_num, this->max_value, (uint32_t) value_cnt,
                           value, (uint16_t) this->max_len, map );
  */
  ::free( map );
  if ( value != NULL )
    ::free( value );
  this->map_num++;
  this->clear_enum();
}

void
MDDictBuild::add_rwf_enum_map( int16_t *fids,  uint32_t num_fids,
                               uint16_t *values,  uint16_t num_values,
                               char *display,  uint32_t disp_len,
                               uint32_t map_num ) noexcept
{
  uint32_t i;
/*
  for ( i = 0; i < num_fids; i++ ) {
    this->add_entry( fids[ i ], map_num + 10, MD_ENUM, 0, NULL, NULL, NULL,
                     "rwf", map_num );
  }
*/
  for ( i = 0; i < num_fids; i++ )
    this->update_entry_enum( fids[ i ], map_num + 1, disp_len );

  uint16_t max_value = values[ num_values - 1 ],
           max_len   = ::strnlen( display, disp_len );
  uint8_t * map      = (uint8_t *) display;
  if ( max_len != disp_len ) {
    map = (uint8_t *) ::malloc( max_len * num_values );
    for ( i = 0; i < num_values; i++ )
      ::memcpy( &map[ i * max_len ], &display[ i * disp_len ], max_len );
  }
  if ( max_value == num_values - 1 )
    values = NULL;

  MDEnumAdd a;
  a.map_num   = map_num + 1;
  a.max_value = max_value;
  a.value_cnt = num_values;
  a.value     = values;
  a.max_len   = max_len;
  a.map       = map;
  this->add_enum_map( a );
/*
  this->add_enum_map( map_num + 10, max_value, num_values, values,
                      max_len, map );
*/
  if ( map != (uint8_t *) display )
    ::free( map );
}

void
EnumDef::push_acro( char *buf,  size_t len,  int fid,  int ln ) noexcept
{
  void *p = malloc( EnumValue::alloc_size( len ) );
  this->acro.push_tl( new ( p ) EnumValue( buf, len, fid, ln ) );
}

static uint8_t
hexval( char c )
{
  if ( c >= '0' && c <= '9' ) return (uint8_t) ( c - '0' );
  if ( c >= 'a' && c <= 'f' ) return (uint8_t) ( c - 'a' + 10 );
  if ( c >= 'A' && c <= 'F' ) return (uint8_t) ( c - 'A' + 10 );
  return 0;
}

void
EnumDef::push_enum( uint32_t value,  char *buf,  size_t len,  int ln ) noexcept
{
  uint8_t tmp[ 256 ];
  size_t  j = 0;
  if ( len > 1 && buf[ 0 ] == '#' && buf[ len - 1 ] == '#' ) {
    for ( size_t i = 1; i < len - 1; i += 2 ) {
      uint8_t hi = hexval( buf[ i ] ), lo = hexval( buf[ i + 1 ] );
      tmp[ j++ ] = ( (uint8_t) hi << 4 ) | (uint8_t) lo;
      if ( j == sizeof( tmp ) )
        break;
    }
    len = j;
    buf = (char *) (void *) tmp;
  }
  void *p = malloc( EnumValue::alloc_size( len ) );
  this->map.push_tl( new ( p ) EnumValue( buf, len, value, ln ) );
  if ( len > this->max_len )
    this->max_len = len;
  if ( value > this->max_value )
    this->max_value = value;
  this->value_cnt++;
}

EnumDefTok
EnumDef::consume_hex( void ) noexcept
{
  size_t i = 1;
  int    c;
  while ( this->get_char( i, c ) ) {
    if ( ( c >= '0' && c <= '9' ) ||
         ( c >= 'a' && c <= 'f' ) ||
         ( c >= 'A' && c <= 'F' ) )
     i++;
    else {
      if ( c == '#' )
        return this->consume( ETK_IDENT, i + 1 );
      break;
    }
  }
  return this->consume( ETK_ERROR, 1 );
}

EnumDefTok
EnumDef::get_token( MDDictBuild &dict_build ) noexcept
{
  int c;
  for (;;) {
    while ( (c = this->eat_white()) == '!' ) {
      size_t tag_sz = this->match_tag( "!tag ", 5 );
      if ( tag_sz > 0 )
        dict_build.add_tag( &this->buf[ this->off ], tag_sz );
      this->eat_comment();
    }
    this->col++;
    switch ( c ) {
      case '"': return this->consume_string();

      case '#': return this->consume_hex();
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return this->consume_int();
      default:
          break;
      case EOF_CHAR:
        return ETK_EOF;
    }
    if ( isalpha( c ) )
      return this->consume_ident();
    return this->consume( ETK_ERROR, 1 );
  }
}

EnumDef *
EnumDef::open_path( const char *path,  const char *filename,
                    int debug_flags ) noexcept
{
  char path2[ 1024 ];
  if ( DictParser::find_file( path, filename, ::strlen( filename ),
                              path2 ) ) {
    void * p = ::malloc( sizeof( EnumDef ) );
    return new ( p ) EnumDef( path2, debug_flags );
  }
  return NULL;
}

int
EnumDef::parse_path( MDDictBuild &dict_build,  const char *path,
                     const char *fn ) noexcept
{
  EnumDef  * p = NULL;
  char       buf[ 256 ];
  int        n = 0, state = 0, ret = 0;
  size_t     len = 0;
  EnumDefTok tok;

  p = EnumDef::open_path( path, fn, dict_build.debug_flags );
  if ( p == NULL ) {
    fprintf( stderr, "\"%s\": file not found\n", fn );
    return Err::FILE_NOT_FOUND;
  }
  while ( p != NULL ) {
    if ( p->col == 2 )
      p->eat_comment();
    tok = p->get_token( dict_build );
    /* flush last enum list */
    if ( p->col == 1 && tok == ETK_IDENT && p->map.tl != NULL )
      p->define_enum( dict_build );
    /* col
     * 1     2
     * ACRO  FID
     *
     * VALUE DISPLAY MEANING */
    switch ( tok ) {
      case ETK_IDENT:
        if ( ( state & 1 ) != 0 )
          goto is_error;
        len = ( p->tok_sz > 255 ? 255 : p->tok_sz );
        ::memcpy( buf, p->tok_buf, len );
        buf[ len ] = '\0';
        state |= 1;
        if ( state == 3 ) {
          if ( p->acro.tl == NULL ) /* should be at least one acro defn */
            goto is_error;
          /* 0   "    " */
          /* 1   "CALL" */
          /* 2   "PUT " */
          p->push_enum( n, buf, len, p->value_lineno ); /* new enum value */
          state = 0;
        }
        else {
          p->value_lineno = p->lineno;
        }
        break;

      case ETK_INT:
        if ( ( state & 2 ) != 0 )
          goto is_error;
        p->tok_buf[ p->tok_sz ] = '\0';
        n = atoi( p->tok_buf );
        state |= 2;
        if ( state == 3 ) { /* can be multiple acros per enum defn */
          p->push_acro( buf, len, n, p->value_lineno );
          state = 0;
        }
        else {
          p->value_lineno = p->lineno;
        }
        break;

      case ETK_ERROR:
      is_error:
        fprintf( stderr, "error at \"%s\" line %u: \"%.*s\"\n",
                 p->fname, p->lineno, (int) p->tok_sz, p->tok_buf );
        if ( ret == 0 )
          ret = Err::DICT_PARSE_ERROR;
        /* FALLTHRU */
      case ETK_EOF:
        if ( ret == 0 ) {
          if ( p->map.tl != NULL ) /* last enum defined */
            p->define_enum( dict_build );
        }
        delete p;
        p = NULL;
        break;
    }
  }
  return ret;
}

