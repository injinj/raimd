#ifndef __rai_raimd__app_a_h__
#define __rai_raimd__app_a_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

/* col
 * 1     2         3    4           5        6       7         8
 * ACRO  DDE_ACRO  FID  RIPPLES_TO  MF_type  MF_len  RWF_type  RWF_len */

enum MF_type {
  MF_NONE              = -1,
  MF_TIME_SECONDS      = 0,
  MF_INTEGER           = 1,
  MF_NUMERIC           = 2,
  MF_DATE              = 3,
  MF_PRICE             = 4,
  MF_ALPHANUMERIC      = 5,
  MF_ENUMERATED        = 6,
  MF_TIME              = 7,
  MF_BINARY            = 8,
  MF_LONG_ALPHANUMERIC = 9,
  MF_OPAQUE            = 10
};
const char *mf_type_str( MF_type ) noexcept;

enum RWF_type {
  /* primitives */
  RWF_NONE         = 0,
  RWF_RSVD_1       = 1, /* INT32 */
  RWF_RSVD_2       = 2, /* UINT32 */
  RWF_INT          = 3, /* int64, int32 (rare) */
  RWF_UINT         = 4, /* uint64 */
  RWF_FLOAT        = 5, /* float */
  RWF_DOUBLE       = 6, /* double */
  RWF_REAL         = 8, /* real64 */
  RWF_DATE         = 9, /* y/m/d */
  RWF_TIME         = 10, /* h:m:s */
  RWF_DATETIME     = 11, /* date + time */
  RWF_QOS          = 12, /* qos bits */
  RWF_STATE        = 13, /* stream state */
  RWF_ENUM         = 14, /* uint16 enum */
  RWF_ARRAY        = 15, /* primitive type array */
  RWF_BUFFER       = 16, /* len + opaque */
  RWF_ASCII_STRING = 17,
  RWF_UTF8_STRING  = 18,
  RWF_RMTES_STRING = 19, /* need rsslRMTESToUTF8 */

  /* sets */
  RWF_INT_1        = 64,
  RWF_UINT_1       = 65,
  RWF_INT_2        = 66,
  RWF_UINT_2       = 67,
  RWF_INT_4        = 68,
  RWF_UINT_4       = 69,
  RWF_INT_8        = 70,
  RWF_UINT_8       = 71,
  RWF_FLOAT_4      = 72,
  RWF_DOUBLE_8     = 73,
  RWF_REAL_4RB     = 74,
  RWF_REAL_8RB     = 75,
  RWF_DATE_4       = 76,
  RWF_TIME_3       = 77,
  RWF_TIME_5       = 78,
  RWF_DATETIME_7   = 79,
  RWF_DATETIME_9   = 80,
  RWF_DATETIME_11  = 81,
  RWF_DATETIME_12  = 82,
  RWF_TIME_7       = 83,
  RWF_TIME_8       = 84,

  /* containers */
  RWF_NO_DATA      = 128,
  RWF_MSG_KEY      = 129,
  RWF_OPAQUE       = 130,
  RWF_XML          = 131,
  RWF_FIELD_LIST   = 132,
  RWF_ELEMENT_LIST = 133,
  RWF_ANSI_PAGE    = 134,
  RWF_FILTER_LIST  = 135,
  RWF_VECTOR       = 136,
  RWF_MAP          = 137,
  RWF_SERIES       = 138,
  RWF_RSVD_139     = 139,
  RWF_RSVD_140     = 140,
  RWF_MSG          = 141,
  RWF_JSON         = 142
};
const char *rwf_type_str( RWF_type t ) noexcept;

static inline int is_rwf_primitive( RWF_type t ) {
  return t >= RWF_INT && t <= RWF_RMTES_STRING;
}
static inline int is_rwf_set_primitive( RWF_type t ) {
  return t >= RWF_INT_1 && t <= RWF_TIME_8;
}
static inline int is_rwf_container( RWF_type t ) {
  return t >= RWF_NO_DATA  && t <= RWF_JSON &&
         t != RWF_RSVD_139 && t != RWF_RSVD_140;
}

static inline size_t rwf_fixed_size( RWF_type t ) {
  switch ( t ) {
    case RWF_INT_1:
    case RWF_UINT_1:
    case RWF_INT_2:
    case RWF_UINT_2:
    case RWF_INT_4:
    case RWF_UINT_4:
    case RWF_INT_8:
    case RWF_UINT_8:
      return 1 << ( ( t - RWF_INT_1 ) / 2 );

    case RWF_FLOAT_4:
    case RWF_DOUBLE_8:
    case RWF_DATE_4:
      return ( t & 1 ) * 4 + 4;

    case RWF_TIME_3:
    case RWF_TIME_5:
    case RWF_DATETIME_7:
    case RWF_DATETIME_9:
    case RWF_DATETIME_11:
    case RWF_DATETIME_12:
    case RWF_TIME_7:
    case RWF_TIME_8: {
      static const uint8_t x[] = { 3, 5, 7, 9, 11, 12, 7, 8 };
      return x[ t - RWF_TIME_3 ];
    }

    case RWF_REAL_4RB:
    case RWF_REAL_8RB:
    default: return 0;
  }
}
/*
 * STRING  -> ALPHANUMERIC        -> { RMTES_STRING, ASCII_STRING, UTF8_STRING }
 * DECIMAL -> { PRICE, INTEGER }  -> REAL64
 * UINT    -> { INTEGER, BINARY } -> UINT64
 * OPAQUE  -> BINARY              -> BUFFER
 */
enum MD_RWF_MF_Flags {
  STRING_TO_RWF_RMTES     = 0, /* default */
  STRING_TO_RWF_ASCII     = 1,
  STRING_TO_RWF_UTF8      = 2,
  STRING_TO_RWF_BUFFER    = 4,
  DECIMAL_TO_MF_PRICE     = 0, /* default */
  DECIMAL_TO_MF_INTEGER   = 1,
  UINT_TO_MF_INTEGER      = 0, /* default */
  UINT_TO_MF_BINARY       = 1,
  OPAQUE_TO_RWF_BUFFER    = 1,
  OPAQUE_TO_RWF_ARRAY     = 2,
  OPAQUE_TO_MF_BINARY     = 4,
  INT_TO_RWF_TIME         = 1,
  TIME_TO_MF_TIME_SECONDS = 1
};

enum AppATok {
  ATK_ERROR        = -2,
  ATK_EOF          = -1,
  ATK_IDENT        = 0,
  ATK_INT          = 1,
  ATK_INTEGER      = 2,
  ATK_ALPHANUMERIC = 3,
  ATK_ENUMERATED   = 4,
  ATK_TIME         = 5,
  ATK_DATE         = 6,
  ATK_PRICE        = 7,
  ATK_OPEN_PAREN   = 8,
  ATK_CLOSE_PAREN  = 9,
  ATK_REAL32       = 10,
  ATK_REAL64       = 11,
  ATK_RMTES_STRING = 12,
  ATK_UINT32       = 13,
  ATK_UINT64       = 14,
  ATK_ASCII_STRING = 15,
  ATK_BINARY       = 16,
  ATK_BUFFER       = 17,
  ATK_NULL         = 18,
  ATK_ENUM         = 19,
  ATK_TIME_SECONDS = 20,
  ATK_INT32        = 21,
  ATK_INT64        = 22,
  ATK_NONE         = 23,
  ATK_ARRAY        = 24,
  ATK_SERIES       = 25,
  ATK_ELEMENT_LIST = 26,
  ATK_VECTOR       = 27,
  ATK_MAP          = 28
};

struct AppAKeyword {
  const char * str;
  size_t       len;
  AppATok      tok;
};

struct MDDictBuild;
struct MDDictAdd;
struct AppA : public DictParser {
  int      fid,
           length,
           enum_length,
           rwf_len;
  AppATok  field_token,
           rwf_token;
  char     acro[ 256 ],
           dde_acro[ 256 ],
           ripples_to[ 256 ];
  RWF_type rwf_type;
  MF_type  mf_type;

  void * operator new( size_t, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  AppA( const char *p,  int debug_flags )
      : DictParser( p, ATK_INT, ATK_IDENT, ATK_ERROR, debug_flags,
                    "RDM Field Dictionary" ) {
    this->clear_line();
  }
  ~AppA() {
    this->clear_line();
    this->close();
  }
  void clear_line( void ) noexcept;
  void set_ident( char *id_buf ) noexcept;
  AppATok consume( AppATok k,  size_t sz ) {
    return (AppATok) this->DictParser::consume_tok( k, sz );
  }
  AppATok consume( AppAKeyword &kw ) {
    return this->consume( kw.tok, kw.len );
  }
  AppATok consume_int( void ) {
    return (AppATok) this->DictParser::consume_int_tok();
  }
  AppATok consume_ident( void ) {
    return (AppATok) this->DictParser::consume_ident_tok();
  }
  AppATok consume_string( void ) {
    return (AppATok) this->DictParser::consume_string_tok();
  }
  AppATok get_token( MDDictBuild &dict_build ) noexcept;
  bool match( AppAKeyword &kw ) {
    return this->DictParser::match( kw.str, kw.len );
  }
  static void rwf_to_md_type_size( MDDictAdd &a ) noexcept;

  static AppA * open_path( const char *path,  const char *filename,
                           int debug_flags ) noexcept;
  static int parse_path( MDDictBuild &dict_build,  const char *path,
                         const char *fn ) noexcept;
};

}
}
#endif

