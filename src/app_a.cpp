#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

#define S( s ) s, sizeof( s ) - 1
static AppAKeyword
  integer_tok      = { S( "integer"      ),  ATK_INTEGER },
  alphanumeric_tok = { S( "alphanumeric" ),  ATK_ALPHANUMERIC },
  enumerated_tok   = { S( "enumerated"   ),  ATK_ENUMERATED },
  enum_tok         = { S( "enum"         ),  ATK_ENUM },
  time_tok         = { S( "time"         ),  ATK_TIME },
  time_seconds_tok = { S( "time_seconds" ),  ATK_TIME_SECONDS },
  date_tok         = { S( "date"         ),  ATK_DATE },
  price_tok        = { S( "price"        ),  ATK_PRICE },
  real32_tok       = { S( "real32"       ),  ATK_REAL32 },
  real64_tok       = { S( "real64"       ),  ATK_REAL64 },
  rmtes_string_tok = { S( "rmtes_string" ),  ATK_RMTES_STRING },
  uint32_tok       = { S( "uint32"       ),  ATK_UINT32 },
  uint64_tok       = { S( "uint64"       ),  ATK_UINT64 },
  ascii_string_tok = { S( "ascii_string" ),  ATK_ASCII_STRING },
  binary_tok       = { S( "binary"       ),  ATK_BINARY },
  buffer_tok       = { S( "buffer"       ),  ATK_BUFFER },
  null_tok         = { S( "null"         ),  ATK_NULL },
  int32_tok        = { S( "int32"        ),  ATK_INT32 },
  int64_tok        = { S( "int64"        ),  ATK_INT64 },
  none_tok         = { S( "none"         ),  ATK_NONE },
  array_tok        = { S( "array"        ),  ATK_ARRAY },
  series_tok       = { S( "series"       ),  ATK_SERIES },
  element_list_tok = { S( "element_list" ),  ATK_ELEMENT_LIST },
  vector_tok       = { S( "vector"       ),  ATK_VECTOR },
  map_tok          = { S( "map"          ),  ATK_MAP };
#undef S

const char *
rai::md::rwf_type_str( RWF_type t ) noexcept
{
  switch ( t ) {
    default:
    case RWF_NONE:
    case RWF_RSVD_1:
    case RWF_RSVD_2:       break;
    case RWF_INT:          return "INT";
    case RWF_UINT:         return "UINT";
    case RWF_FLOAT:        return "FLOAT";
    case RWF_DOUBLE:       return "DOUBLE";
    case RWF_REAL:         return "REAL";
    case RWF_DATE:         return "DATE";
    case RWF_TIME:         return "TIME";
    case RWF_DATETIME:     return "DATETIME";
    case RWF_QOS:          return "QOS";
    case RWF_ENUM:         return "ENUM";
    case RWF_ARRAY:        return "ARRAY";
    case RWF_BUFFER:       return "BUFFER";
    case RWF_ASCII_STRING: return "ASCII_STRING";
    case RWF_UTF8_STRING:  return "UTF8_STRING";
    case RWF_RMTES_STRING: return "RMTES_STRING";

    /* sets */
    case RWF_INT_1:        return "INT_1";
    case RWF_UINT_1:       return "UINT_1";
    case RWF_INT_2:        return "INT_2";
    case RWF_UINT_2:       return "UINT_2";
    case RWF_INT_4:        return "INT_4";
    case RWF_UINT_4:       return "UINT_4";
    case RWF_INT_8:        return "INT_8";
    case RWF_UINT_8:       return "UINT_8";
    case RWF_FLOAT_4:      return "FLOAT_4";
    case RWF_DOUBLE_8:     return "DOUBLE_8";
    case RWF_REAL_4RB:     return "REAL_4RB";
    case RWF_REAL_8RB:     return "REAL_8RB";
    case RWF_DATE_4:       return "DATE_4";
    case RWF_TIME_3:       return "TIME_3";
    case RWF_TIME_5:       return "TIME_5";
    case RWF_DATETIME_7:   return "DATETIME_7";
    case RWF_DATETIME_9:   return "DATETIME_9";
    case RWF_DATETIME_11:  return "DATETIME_11";
    case RWF_DATETIME_12:  return "DATETIME_12";
    case RWF_TIME_7:       return "TIME_7";
    case RWF_TIME_8:       return "TIME_8";

    /* containers */
    case RWF_NO_DATA:      return "NO_DATA";
    case RWF_MSG_KEY:      return "MSG_KEY";
    case RWF_OPAQUE:       return "OPAQUE";
    case RWF_XML:          return "XML";
    case RWF_FIELD_LIST:   return "FIELD_LIST";
    case RWF_ELEMENT_LIST: return "ELEMENT_LIST";
    case RWF_ANSI_PAGE:    return "ANSI_PAGE";
    case RWF_FILTER_LIST:  return "FILTER_LIST";
    case RWF_VECTOR:       return "VECTOR";
    case RWF_MAP:          return "MAP";
    case RWF_SERIES:       return "SERIES";
    case RWF_RSVD_139:
    case RWF_RSVD_140:     break;
    case RWF_MSG:          return "MSG";
    case RWF_JSON:         return "JSON";
  }
  return "NONE";
}

const char *
rai::md::mf_type_str( MF_type t ) noexcept
{
  switch ( t ) {
    default:
    case MF_NONE:              break;
    case MF_TIME_SECONDS:      return "TIME_SECONDS";
    case MF_INTEGER:           return "INTEGER";
    case MF_NUMERIC:           return "NUMERIC";
    case MF_DATE:              return "DATE";
    case MF_PRICE:             return "PRICE";
    case MF_ALPHANUMERIC:      return "ALPHANUMERIC";
    case MF_ENUMERATED:        return "ENUMERATED";
    case MF_TIME:              return "TIME";
    case MF_BINARY:            return "BINARY";
    case MF_LONG_ALPHANUMERIC: return "LONG_ALPHANUMERIC";
    case MF_OPAQUE:            return "OPAQUE";
  }
  return "NONE";
}

static RWF_type
tok_to_rwf_type( AppATok t ) noexcept
{
  switch ( t ) {
    case ATK_TIME:         return RWF_TIME;
    case ATK_DATE:         return RWF_DATE;
    case ATK_REAL32:       return RWF_REAL;
    case ATK_REAL64:       return RWF_REAL;
    case ATK_RMTES_STRING: return RWF_RMTES_STRING;
    case ATK_UINT32:       return RWF_UINT;
    case ATK_UINT64:       return RWF_UINT;
    case ATK_ASCII_STRING: return RWF_ASCII_STRING;
    case ATK_BUFFER:       return RWF_BUFFER;
    case ATK_ENUM:         return RWF_ENUM;
    case ATK_INT32:        return RWF_INT;
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

static MF_type
tok_to_mf_type( AppATok t ) noexcept
{
  switch ( t ) {
    case ATK_INTEGER:      return MF_INTEGER;
    case ATK_ALPHANUMERIC: return MF_ALPHANUMERIC;
    case ATK_ENUMERATED:   return MF_ENUMERATED;
    case ATK_TIME:         return MF_TIME;
    case ATK_DATE:         return MF_DATE;
    case ATK_PRICE:        return MF_PRICE;
    case ATK_BINARY:       return MF_BINARY;
    case ATK_TIME_SECONDS: return MF_TIME_SECONDS;
    default:
    case ATK_NONE:         return MF_NONE;
  }
}

void
AppA::clear_line( void ) noexcept
{
  this->fid             = 0;
  this->length          = 0;
  this->enum_length     = 0;
  this->rwf_len         = 0;
  this->field_token     = ATK_ERROR;
  this->rwf_token       = ATK_ERROR;
  this->acro[ 0 ]       = '\0';
  this->dde_acro[ 0 ]   = '\0';
  this->ripples_to[ 0 ] = '\0';
  this->rwf_type        = RWF_NONE;
  this->mf_type         = MF_NONE;
}

void
AppA::set_ident( char *id_buf ) noexcept
{
  size_t len = ( this->tok_sz > 255 ? 255 : this->tok_sz );
  ::memcpy( id_buf, this->tok_buf, len );
  id_buf[ len ] = '\0';
}

AppATok
AppA::get_token( MDDictBuild &dict_build ) noexcept
{
  int c;
  for (;;) {
    while ( (c = this->eat_white()) == '!' ) {
      size_t tag_sz = this->match_tag( "!tag ", 5 );
      if ( tag_sz > 0 )
        dict_build.add_tag( &this->buf[ this->off ], tag_sz );
      this->eat_comment();
    }
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
          if ( this->match( int32_tok ) )
            return this->consume( int32_tok );
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
          if ( this->match( real32_tok ) )
            return this->consume( real32_tok );
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
          if ( this->match( uint32_tok ) )
            return this->consume( uint32_tok );
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
AppA::open_path( const char *path,  const char *filename,
                 int debug_flags ) noexcept
{
  char path2[ 1024 ];
  if ( DictParser::find_file( path, filename, ::strlen( filename ),
                              path2 ) ) {
    void * p = ::malloc( sizeof( AppA ) );
    return new ( p ) AppA( path2, debug_flags );
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
static void
rwf_type_size( MDType &type,  uint32_t &size,  const char *,
               uint8_t mf_type,  uint32_t length,  uint8_t rwf_type,
               uint32_t rwf_len ) noexcept
{
  if ( length == 0 )
    length = rwf_len;
  switch ( mf_type ) {
    case MF_LONG_ALPHANUMERIC:
    case MF_ALPHANUMERIC:
      type = MD_STRING;
      size = length;
      break;

    case MF_TIME:
      type = MD_TIME;
      size = 6; /* hr:mi */
      return;

    case MF_DATE:
      type = MD_DATE;
      size = 12; /* dd MMM yyyy */
      break;

    case MF_ENUMERATED:
      type = MD_ENUM;
      size = 2;
      break;

    case MF_INTEGER:
      if ( rwf_type != RWF_REAL ) {
        if ( length <= 5 ) {
          if ( rwf_type == RWF_UINT )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 2;
          break;
        }
        if ( length <= 10 ) {
          if ( rwf_type == RWF_UINT )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 4;
          break;
        }
        if ( length <= 15 ) {
          if ( rwf_type == RWF_UINT )
            type = MD_UINT;
          else
            type = MD_INT;
          size = 8;
          break;
        }
      }
      /* FALLTHRU */
    case MF_PRICE:
      type = MD_DECIMAL;
      size = 9;
      break;

    case MF_TIME_SECONDS:
      type = MD_TIME;
      size = 10; /* hr:mi:se */
      break;

      /* FALLTHRU */
    case MF_BINARY:
    case MF_OPAQUE:
    default:
      switch ( rwf_type ) {
        default:
        case RWF_BUFFER:
        case RWF_ARRAY:
          type = MD_OPAQUE;
          size = length;
          break;
        case RWF_REAL:
          type = MD_DECIMAL;
          size = 9;
          break;
        case RWF_INT:
          type = MD_INT;
          size = 8;
          break;
        case RWF_UINT:
          type = MD_UINT;
          size = 8;
          break;
        case RWF_ASCII_STRING:
        case RWF_UTF8_STRING:
        case RWF_RMTES_STRING:
          type = MD_STRING;
          size = length;
          break;
        case RWF_TIME:
          type = MD_TIME;
          size = 6;
          return;
        case RWF_DATE:
          type = MD_DATE;
          size = 12;
          break;
        case RWF_MAP:
        case RWF_VECTOR:
        case RWF_SERIES:
        case RWF_ELEMENT_LIST:
          type = MD_MESSAGE;
          size = length;
          break;
      }
      break;
  }
}

static MF_type md_to_mf[] = {
/*MD_NODAT       */ MF_NONE,
/*MD_MESSAGE     */ MF_OPAQUE,
/*MD_STRING      */ MF_ALPHANUMERIC,
/*MD_OPAQUE      */ MF_OPAQUE,
/*MD_BOOLEAN     */ MF_INTEGER,
/*MD_INT         */ MF_INTEGER,
/*MD_UINT        */ MF_INTEGER,
/*MD_REAL        */ MF_PRICE,
/*MD_ARRAY       */ MF_OPAQUE,
/*MD_PARTIAL     */ MF_ALPHANUMERIC,
/*MD_IPDATA      */ MF_OPAQUE,
/*MD_SUBJECT     */ MF_OPAQUE,
/*MD_ENUM        */ MF_ENUMERATED,
/*MD_TIME        */ MF_TIME,
/*MD_DATE        */ MF_DATE,
/*MD_DATETIME    */ MF_OPAQUE,
/*MD_STAMP       */ MF_INTEGER,
/*MD_DECIMAL     */ MF_PRICE,
/*MD_LIST        */ MF_OPAQUE,
/*MD_HASH        */ MF_OPAQUE,
/*MD_SET         */ MF_OPAQUE,
/*MD_ZSET        */ MF_OPAQUE,
/*MD_GEO         */ MF_OPAQUE,
/*MD_HYPERLOGLOG */ MF_OPAQUE,
/*MD_STREAM      */ MF_OPAQUE
};
void
MDLookup::mf_type( uint8_t &mf_type,  uint32_t &mf_len,
                   uint32_t &enum_len ) noexcept
{
  mf_type  = md_to_mf[ this->ftype ];
  enum_len = this->enumlen;
  if ( this->ftype == MD_MESSAGE || this->mflen == 0 ) {
    mf_type = MF_NONE;
    mf_len  = 0;
  }
  else {
    if ( this->mflen == ( 1 << MF_LEN_BITS ) - 1 )
      mf_len = this->fsize;
    else
      mf_len = this->mflen;
    if ( this->ftype == MD_DECIMAL ) {
      if ( ( this->flags & DECIMAL_TO_MF_INTEGER ) != 0 )
        mf_type = MF_INTEGER;
    }
    else if ( this->ftype == MD_UINT ) {
      if ( ( this->flags & UINT_TO_MF_BINARY ) != 0 )
        mf_type = MF_BINARY;
    }
    else if ( this->ftype == MD_OPAQUE ) {
      if ( ( this->flags & OPAQUE_TO_MF_BINARY ) != 0 )
        mf_type = MF_BINARY;
    }
    else if ( this->ftype == MD_TIME ) {
      if ( ( this->flags & TIME_TO_MF_TIME_SECONDS ) != 0 )
        mf_type = MF_TIME_SECONDS;
    }
  }
}

static RWF_type md_to_rwf[] = {
/*MD_NODAT       */ RWF_NONE,
/*MD_MESSAGE     */ RWF_MSG,
/*MD_STRING      */ RWF_RMTES_STRING,
/*MD_OPAQUE      */ RWF_OPAQUE,
/*MD_BOOLEAN     */ RWF_UINT,
/*MD_INT         */ RWF_INT,
/*MD_UINT        */ RWF_UINT,
/*MD_REAL        */ RWF_REAL,
/*MD_ARRAY       */ RWF_ARRAY,
/*MD_PARTIAL     */ RWF_RMTES_STRING,
/*MD_IPDATA      */ RWF_OPAQUE,
/*MD_SUBJECT     */ RWF_OPAQUE,
/*MD_ENUM        */ RWF_ENUM,
/*MD_TIME        */ RWF_TIME,
/*MD_DATE        */ RWF_DATE,
/*MD_DATETIME    */ RWF_DATETIME,
/*MD_STAMP       */ RWF_UINT,
/*MD_DECIMAL     */ RWF_REAL,
/*MD_LIST        */ RWF_OPAQUE,
/*MD_HASH        */ RWF_OPAQUE,
/*MD_SET         */ RWF_OPAQUE,
/*MD_ZSET        */ RWF_OPAQUE,
/*MD_GEO         */ RWF_OPAQUE,
/*MD_HYPERLOGLOG */ RWF_OPAQUE,
/*MD_STREAM      */ RWF_OPAQUE
};
void
MDLookup::rwf_type( uint8_t &rwf_type,  uint32_t &rwf_len ) noexcept
{
  rwf_type = md_to_rwf[ this->ftype ];
  if ( this->rwflen == ( 1 << RWF_LEN_BITS ) - 1 )
    rwf_len = this->fsize;
  else
    rwf_len = this->rwflen;
  if ( this->ftype == MD_STRING ) {
    if ( ( this->flags & STRING_TO_RWF_ASCII ) != 0 )
      rwf_type = RWF_ASCII_STRING;
    else if ( ( this->flags & STRING_TO_RWF_UTF8 ) != 0 )
      rwf_type = RWF_UTF8_STRING;
    else if ( ( this->flags & STRING_TO_RWF_BUFFER ) != 0 )
      rwf_type = RWF_BUFFER;
  }
  else if ( this->ftype == MD_OPAQUE ) {
    if ( ( this->flags & OPAQUE_TO_RWF_BUFFER ) != 0 )
      rwf_type = RWF_BUFFER;
    else if ( ( this->flags & OPAQUE_TO_RWF_ARRAY ) != 0 )
      rwf_type = RWF_ARRAY;
  }
  else if ( this->ftype == MD_INT ) {
    if ( ( this->flags & INT_TO_RWF_TIME ) != 0 )
      rwf_type = RWF_TIME;
  }
  else if ( this->ftype == MD_MESSAGE ) {
    rwf_type = ( this->flags & 0xf ) + 128;
  }
}

int
MDDictBuild::add_rwf_entry( const char *acro,  const char *acro_dde,
                            MDFid fid,  MDFid ripplesto,
                            uint8_t mf_type,  uint32_t length,
                            uint32_t enum_len, uint8_t rwf_type,
                            uint32_t rwf_len,  const char *filename,
                            uint32_t lineno ) noexcept
{
  MDDictAdd a;
  a.fid       = fid;
  a.fname     = acro;
  a.name      = acro_dde;
  a.ripplefid = ripplesto;
  a.filename  = filename ? filename : "rwf";
  a.lineno    = lineno;
  a.mf_len    = length;
  a.rwf_len   = rwf_len;
  a.enum_len  = enum_len;
  a.mf_type   = mf_type;
  a.rwf_type  = rwf_type;
  AppA::rwf_to_md_type_size( a );
  return this->add_entry( a );
}

void
AppA::rwf_to_md_type_size( MDDictAdd &a ) noexcept
{
  uint32_t flags = 0;
  MDType   tp;
  uint32_t sz;

  rwf_type_size( tp, sz, a.fname, a.mf_type, a.mf_len, a.rwf_type, a.rwf_len );
  a.ftype = tp;
  a.fsize = sz;

  if ( a.mf_len >= ( 1 << MF_LEN_BITS ) )
    a.mf_len = ( 1 << MF_LEN_BITS ) - 1;
  if ( a.rwf_len >= ( 1 << RWF_LEN_BITS ) )
    a.rwf_len = ( 1 << RWF_LEN_BITS ) - 1;

  if ( tp == MD_STRING ) {
    if ( a.rwf_type == RWF_RMTES_STRING )
      flags = STRING_TO_RWF_RMTES;
    else if ( a.rwf_type == RWF_ASCII_STRING )
      flags = STRING_TO_RWF_ASCII;
    else if ( a.rwf_type == RWF_UTF8_STRING )
      flags = STRING_TO_RWF_UTF8;
    else if ( a.rwf_type == RWF_BUFFER )
      flags = STRING_TO_RWF_BUFFER;
  }
  else if ( tp == MD_DECIMAL ) {
    if ( a.mf_type == MF_INTEGER )
      flags = DECIMAL_TO_MF_INTEGER;
    else if ( a.mf_type == MF_PRICE )
      flags = DECIMAL_TO_MF_PRICE;
  }
  else if ( tp == MD_UINT ) {
    if ( a.mf_type == MF_INTEGER )
      flags = UINT_TO_MF_INTEGER;
    else if ( a.mf_type == MF_BINARY )
      flags = UINT_TO_MF_BINARY;
  }
  else if ( tp == MD_OPAQUE ) {
    if ( a.mf_type == MF_BINARY )
      flags = OPAQUE_TO_MF_BINARY;
    if ( a.rwf_type == RWF_BUFFER )
      flags |= OPAQUE_TO_RWF_BUFFER;
    else if ( a.rwf_type == RWF_ARRAY )
      flags |= OPAQUE_TO_RWF_ARRAY;
  }
  else if ( tp == MD_INT ) {
    if ( a.rwf_type == RWF_TIME )
      flags |= INT_TO_RWF_TIME;
  }
  else if ( tp == MD_TIME ) {
    if ( a.mf_type == MF_TIME_SECONDS )
      flags |= TIME_TO_MF_TIME_SECONDS;
  }
  else if ( tp == MD_MESSAGE ) {
    if ( a.rwf_type > 128 )
      flags = ( a.rwf_type - 128 );
  }
  a.flags |= flags;
}

int
AppA::parse_path( MDDictBuild &dict_build,  const char *path,
                  const char *fn ) noexcept
{
  AppA * p = NULL;
  int ret = 0, curlineno = 0, n;

  p = AppA::open_path( path, fn, dict_build.debug_flags );
  if ( p == NULL ) {
    fprintf( stderr, "\"%s\": file not found\n", fn );
    return Err::FILE_NOT_FOUND;
  }
  while ( p != NULL ) {
    AppATok tok = p->get_token( dict_build );
    if ( p->lineno != curlineno ) {
      if ( curlineno != 0 ) {
        MDDictAdd a;
        a.fid       = p->fid;
        a.fname     = p->acro;
        a.name      = p->dde_acro;
        a.ripple    = p->ripples_to;
        a.filename  = p->fname;
        a.lineno    = curlineno;
        a.mf_len    = p->length;
        a.rwf_len   = p->rwf_len;
        a.enum_len  = p->enum_length;
        a.mf_type   = p->mf_type;
        a.rwf_type  = p->rwf_type;
        if ( a.ripple != NULL && ::strcmp( a.ripple, "NULL" ) == 0 )
          a.ripple = NULL;
        rwf_to_md_type_size( a );
        dict_build.add_entry( a );
        /*
        dict_build.add_rwf_entry( p->acro, p->dde_acro, p->fid, p->ripples_to,
                                  p->mf_type, p->length, p->enum_length,
                                  p->rwf_type, p->rwf_len,
                                  p->fname, curlineno );*/
#if 0
        printf( "%s %s %d %s %s %d %s %d => %s %u\n",
                p->acro, p->dde_acro, p->fid,
                p->ripples_to, app_a_type_str( p->field_token ), 
                p->length, app_a_type_str( p->rwf_token ), p->rwf_len,
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
          case 1: case 2: case 4:
            goto set_ident;
          case 3:
            p->fid = n;
            break;
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
      case ATK_REAL32:
      case ATK_UINT64:
      case ATK_UINT32:
      case ATK_BUFFER:
      case ATK_INT64:
      case ATK_INT32:
      case ATK_NONE:
      case ATK_ARRAY:
      case ATK_SERIES:
      case ATK_ELEMENT_LIST:
      case ATK_VECTOR:
      case ATK_MAP:
        switch ( p->col ) {
          case 1: case 2: case 4:
            goto set_ident;
          case 5:
            p->field_token = tok;
            p->mf_type = tok_to_mf_type( tok );
            break;
          case 7:
            p->rwf_token = tok;
            p->rwf_type = tok_to_rwf_type( tok );
            break;
          default:
            goto is_error; /* fid, length, rwf_len */
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
        /* FALLTHRU */
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

