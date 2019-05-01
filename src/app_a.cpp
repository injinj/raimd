#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

static AppAKeyword
  integer_tok      = { "integer",       7, ATK_INTEGER },
  alphanumeric_tok = { "alphanumeric", 12, ATK_ALPHANUMERIC },
  enumerated_tok   = { "enumerated",   10, ATK_ENUMERATED },
  enum_tok         = { "enum",          4, ATK_ENUM },
  time_tok         = { "time",          4, ATK_TIME },
  time_seconds_tok = { "time_seconds", 12, ATK_TIME_SECONDS },
  date_tok         = { "date",          4, ATK_DATE },
  price_tok        = { "price",         5, ATK_PRICE },
  real64_tok       = { "real64",        6, ATK_REAL64 },
  rmtes_string_tok = { "rmtes_string", 12, ATK_RMTES_STRING },
  uint64_tok       = { "uint64",        6, ATK_UINT64 },
  ascii_string_tok = { "ascii_string", 12, ATK_ASCII_STRING },
  binary_tok       = { "binary",        6, ATK_BINARY },
  buffer_tok       = { "buffer",        6, ATK_BUFFER },
  null_tok         = { "null",          4, ATK_NULL },
  int64_tok        = { "int64",         5, ATK_INT64 },
  none_tok         = { "none",          4, ATK_NONE },
  array_tok        = { "array",         5, ATK_ARRAY },
  series_tok       = { "series",        6, ATK_SERIES },
  element_list_tok = { "element_list", 12, ATK_ELEMENT_LIST },
  vector_tok       = { "vector",        6, ATK_VECTOR },
  map_tok          = { "map",           3, ATK_MAP };

static uint8_t
tok_to_type( AppATok t )
{
  switch ( t ) {
    case ATK_INTEGER:      return MF_INTEGER;
    case ATK_ALPHANUMERIC: return MF_ALPHANUMERIC;
    case ATK_ENUMERATED:   return MF_ENUMERATED;
    case ATK_TIME:         return MF_TIME;
    case ATK_DATE:         return MF_DATE;
    case ATK_PRICE:        return MF_PRICE;
    case ATK_REAL64:       return RWF_REAL;
    case ATK_RMTES_STRING: return RWF_RMTES_STRING;
    case ATK_UINT64:       return RWF_UINT;
    case ATK_ASCII_STRING: return RWF_ASCII_STRING;
    case ATK_BINARY:       return MF_BINARY;
    case ATK_BUFFER:       return RWF_BUFFER;
    case ATK_ENUM:         return RWF_ENUM;
    case ATK_TIME_SECONDS: return MF_TIME_SECONDS;
    case ATK_INT64:        return RWF_INT;
    default:
    case ATK_NONE:         return RWF_NONE;
    case ATK_ARRAY:        return RWF_ARRAY;
    case ATK_SERIES:       return RWF_SERIES;
    case ATK_ELEMENT_LIST: return RWF_ELEMENT_LIST;
    case ATK_VECTOR:       return RWF_VECTOR;
    case ATK_MAP:          return RWF_MAP;
  }
}

void
AppA::clear_line( void )
{
  this->fid             = 0;
  this->length          = 0;
  this->enum_length     = 0;
  this->rwf_len         = 0;
  this->field_type      = ATK_ERROR;
  this->rwf_type        = ATK_ERROR;
  this->acro[ 0 ]       = '\0';
  this->dde_acro[ 0 ]   = '\0';
  this->ripples_to[ 0 ] = '\0';
}

void
AppA::set_ident( char *id_buf )
{
  size_t len = ( this->tok_sz > 255 ? 255 : this->tok_sz );
  ::memcpy( id_buf, this->tok_buf, len );
  id_buf[ len ] = '\0';
}

AppATok
AppA::get_token( void )
{
  int c;
  for (;;) {
    while ( (c = this->eat_white()) == '!' )
      this->eat_comment();
    if ( this->br_level == 0 )
      this->col++;
    switch ( c ) {
      case '(': return this->consume( ATK_OPEN_PAREN, 1 );
      case ')': return this->consume( ATK_CLOSE_PAREN, 1 );
      case '"': return this->consume_string();

      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return this->consume_int();
      default:
          break;
      case EOF_CHAR:
        return ATK_EOF;
    }
    if ( this->col > 4 ) { /* acro, dde, fid, ripple, >[field type] ... */
      switch ( c ) {
        case 'a': case 'A':
          if ( this->match( alphanumeric_tok ) )
            return this->consume( alphanumeric_tok );
          if ( this->match( ascii_string_tok ) )
            return this->consume( ascii_string_tok );
          if ( this->match( array_tok ) )
            return this->consume( array_tok );
          break;
        case 'b': case 'B':
          if ( this->match( buffer_tok ) )
            return this->consume( buffer_tok );
          if ( this->match( binary_tok ) )
            return this->consume( binary_tok );
          break;
        case 'd': case 'D':
          if ( this->match( date_tok ) )
            return this->consume( date_tok );
          break;
        case 'e': case 'E':
          if ( this->match( enumerated_tok ) )
            return this->consume( enumerated_tok );
          if ( this->match( element_list_tok ) )
            return this->consume( element_list_tok );
          if ( this->match( enum_tok ) )
            return this->consume( enum_tok );
          break;
        case 'i': case 'I':
          if ( this->match( int64_tok ) )
            return this->consume( int64_tok );
          if ( this->match( integer_tok ) )
            return this->consume( integer_tok );
          break;
        case 'm': case 'M':
          if ( this->match( map_tok ) )
            return this->consume( map_tok );
          break;
        case 'n': case 'N':
          if ( this->match( null_tok ) )
            return this->consume( null_tok );
          if ( this->match( none_tok ) )
            return this->consume( none_tok );
          break;
        case 'p': case 'P':
          if ( this->match( price_tok ) )
            return this->consume( price_tok );
          break;
        case 'r': case 'R':
          if ( this->match( rmtes_string_tok ) )
            return this->consume( rmtes_string_tok );
          if ( this->match( real64_tok ) )
            return this->consume( real64_tok );
          break;
        case 's': case 'S':
          if ( this->match( series_tok ) )
            return this->consume( series_tok );
          break;
        case 't': case 'T':
          if ( this->match( time_seconds_tok ) )
            return this->consume( time_seconds_tok );
          if ( this->match( time_tok ) )
            return this->consume( time_tok );
          break;
        case 'u': case 'U':
          if ( this->match( uint64_tok ) )
            return this->consume( uint64_tok );
          break;
        case 'v': case 'V':
          if ( this->match( vector_tok ) )
            return this->consume( vector_tok );
          break;
        default:
          break;
      }
    }
    else if ( this->col == 3 ) { /* ripples may be null */
      if ( this->match( null_tok ) )
        return this->consume( null_tok );
    }
    if ( isalpha( c ) )
      return this->consume_ident();
    return this->consume( ATK_ERROR, 1 );
  }
}

AppA *
AppA::open_path( const char *path,  const char *filename )
{
  char path2[ 1024 ];
  if ( DictParser::find_file( path, filename, ::strlen( filename ),
                              path2 ) ) {
    void * p = ::malloc( sizeof( AppA ) );
    return new ( p ) AppA( path2 );
  }
  return NULL;
}
#if 0
static const char *
app_a_type_str( AppATok type )
{
  switch ( type ) {
    case ATK_INTEGER:      return "INTEGER";
    case ATK_ALPHANUMERIC: return "ALPHANUMERIC";
    case ATK_ENUMERATED:   return "ENUMERATED";
    case ATK_TIME:         return "TIME";
    case ATK_DATE:         return "DATE";
    case ATK_PRICE:        return "PRICE";
    case ATK_REAL64:       return "REAL64";
    case ATK_RMTES_STRING: return "RMTES_STRING";
    case ATK_UINT64:       return "UINT64";
    case ATK_ASCII_STRING: return "ASCII_STRING";
    case ATK_BINARY:       return "BINARY";
    case ATK_BUFFER:       return "BUFFER";
    case ATK_ENUM:         return "ENUM";
    case ATK_TIME_SECONDS: return "TIME_SECONDS";
    case ATK_INT64:        return "INT64";
    case ATK_NONE:         return "NONE";
    case ATK_ARRAY:        return "ARRAY";
    case ATK_SERIES:       return "SERIES";
    case ATK_ELEMENT_LIST: return "ELEMENT_LIST";
    case ATK_VECTOR:       return "VECTOR";
    case ATK_MAP:          return "MAP";
    default:               return "__bad__";
  }
}
#endif
void
AppA::get_type_size( MDType &type,  uint32_t &size )
{
  switch ( this->field_type ) {
    case ATK_ALPHANUMERIC:
      if ( ::strncmp( this->acro, "ROW", 3 ) == 0 ) {
        if ( isdigit( this->acro[ 3 ] ) && isdigit( this->acro[ 4 ] ) ) {
          type = MD_PARTIAL;
          if ( this->length == 99 )
            size = 300;
          else
            size = this->length * 2 + 2;
          break;
        }
      }
      type = MD_STRING;
      size = this->length + 1;
      size = ( size + 1 ) & ~1;
      break;

    case ATK_TIME:
      type = MD_TIME;
      size = 6; /* hr:mi */
      return;

    case ATK_DATE:
      type = MD_DATE;
      size = 12; /* dd MMM yyyy */
      break;

    case ATK_ENUMERATED:
#if 0
      type = MD_STRING;
      if ( this->length > this->enum_length )
        size = this->length;
      else
        size = this->enum_length;
      size = ( size + 1 ) & ~1;
#endif
      type = MD_ENUM;
      size = 2;
      break;

    case ATK_INTEGER:
      if ( this->rwf_type != ATK_REAL64 ) {
        if ( this->length <= 5 ) {
          if ( this->rwf_type == ATK_UINT64 )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 2;
          break;
        }
        if ( this->length <= 10 ) {
          if ( this->rwf_type == ATK_UINT64 )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 4;
          break;
        }
        if ( this->length <= 15 ) {
          if ( this->rwf_type == ATK_UINT64 )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 8;
          break;
        }
      }
      /* FALLTHRU */
    case ATK_PRICE:
      type = MD_DECIMAL;
      size = 9;
      break;

    case ATK_TIME_SECONDS:
      type = MD_TIME;
      size = 10; /* hr:mi:se */
      break;

    case ATK_BINARY:
      if ( this->rwf_type == ATK_UINT64 ) {
        type = MD_UINT;
        size = 8;
        break;
      }
      /* FALLTHRU */
    default:
      if ( this->rwf_type == ATK_REAL64 ) {
        type = MD_DECIMAL;
        size = 9;
        break;
      }
      type = MD_OPAQUE;
      size = this->length;
      break;
  }
}

int
AppA::parse_path( MDDictBuild &dict_build,  const char *path,  const char *fn )
{
  AppA * p = NULL;
  int ret = 0, curlineno = 0, n;

  p = AppA::open_path( path, fn );
  if ( p == NULL ) {
    fprintf( stderr, "\"%s\": file not found\n", fn );
    return Err::FILE_NOT_FOUND;
  }
  while ( p != NULL ) {
    AppATok tok = p->get_token();
    if ( p->lineno != curlineno ) {
      if ( curlineno != 0 ) {
        MDDictEntry *entry;
        MDType   tp;
        uint32_t sz;
        p->get_type_size( tp, sz );
        const char * dde = NULL, /*p->dde_acro,*/
                   * rip = NULL; /*p->ripple_to;*/
        dict_build.add_entry( p->fid, sz, tp, p->acro, dde, rip, p->fname,
                              curlineno, &entry );
                              
        if ( entry != NULL ) {
          entry->mf_len   = p->length;
          entry->rwf_len  = p->rwf_len;
          entry->enum_len = p->enum_length;
          entry->mf_type  = tok_to_type( p->field_type );
          entry->rwf_type = tok_to_type( p->rwf_type );
        }
#if 0
        printf( "%s %s %d %s %s %d %s %d => %s %u\n",
                p->acro, p->dde_acro, p->fid,
                p->ripples_to, app_a_type_str( p->field_type ), 
                p->length, app_a_type_str( p->rwf_type ), p->rwf_len,
                md_type_str( tp ), sz );
#endif
      }
      curlineno = p->lineno;
      p->clear_line();
      p->br_level = 0;
    }
    else {
      if ( p->col == 6 || p->col == 7 ) {
        if ( tok == ATK_OPEN_PAREN ) {
          p->col = 6;
          p->br_level++;
          continue;
        }
        else if ( tok == ATK_CLOSE_PAREN ) {
          p->br_level--;
          continue;
        }
      }
    }
    /* col
     * 1     2         3    4           5           6       7         8
     * ACRO  DDE_ACRO  FID  RIPPLES_TO  FIELD_TYPE  LENGTH  RWF_TYPE  RWF_LEN */
    switch ( tok ) {
      case ATK_IDENT:
      case ATK_NULL:
      set_ident:;
        switch ( p->col ) {
          case 1: p->set_ident( p->acro ); break;
          case 2: p->set_ident( p->dde_acro ); break;
          case 4: p->set_ident( p->ripples_to ); break;
          /*case 7: unknown rwf type */
          default:
            fprintf( stderr, "ident \"%.*s\" at line %u\n",
                     (int) p->tok_sz, p->tok_buf, p->lineno );
            if ( ret == 0 )
              ret = Err::DICT_PARSE_ERROR;
            goto is_error;
        }
        break;

      case ATK_INT:
        p->tok_buf[ p->tok_sz ] = '\0';
        n = atoi( p->tok_buf );
        switch ( p->col ) {
          case 1: case 2: case 4: goto set_ident;
          case 3: p->fid = n; break;
          case 6:
            if ( p->br_level == 0 )
              p->length = n;
            else
              p->enum_length = n;
            break;
          case 8:
            p->rwf_len = n;
            break;
          default:
            goto is_error;
        }
        break;

      case ATK_INTEGER:
      case ATK_ALPHANUMERIC:
      case ATK_ENUMERATED:
      case ATK_ENUM:
      case ATK_TIME:
      case ATK_TIME_SECONDS:
      case ATK_DATE:
      case ATK_PRICE:
      case ATK_BINARY:
      case ATK_RMTES_STRING:
      case ATK_ASCII_STRING:
      case ATK_REAL64:
      case ATK_UINT64:
      case ATK_BUFFER:
      case ATK_INT64:
      case ATK_NONE:
      case ATK_ARRAY:
      case ATK_SERIES:
      case ATK_ELEMENT_LIST:
      case ATK_VECTOR:
      case ATK_MAP:
        switch ( p->col ) {
          case 1: case 2: case 4: goto set_ident;
          case 5: p->field_type = tok; break;
          case 7: p->rwf_type = tok; break;
          default: goto is_error; /* fid, length, rwf_len */
        }
        break;

      case ATK_OPEN_PAREN:
      case ATK_CLOSE_PAREN:
      case ATK_ERROR:
      is_error:
        fprintf( stderr, "error at \"%s\" line %u: \"%.*s\"\n",
                 p->fname, p->lineno, (int) p->tok_sz, p->tok_buf );
        if ( p->col >= 1 && p->col <= 8 ) {
          static const char *col_name[] = { "ACRO", "DDE ACRO", "FID",
            "RIPPLES", "FIELD TYPE", "LENGTH", "RWF TYPE", "RWF LEN" };
          fprintf( stderr, "  col %d, expecting %s\n", p->col,
                   col_name[ p->col - 1 ] );
        }
        if ( ret == 0 )
          ret = Err::DICT_PARSE_ERROR;
      case ATK_EOF:
        if ( ret == 0 && p->br_level != 0 ) {
          fprintf( stderr, "mismatched paren: ')' in file \"%s\"\n",
                   p->fname );
        }
        delete p;
        p = NULL;
        break;
    }
  }
  return ret;
}

