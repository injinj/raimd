#ifndef __rai_raimd__app_a_h__
#define __rai_raimd__app_a_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

/* col
 * 1     2         3    4           5        6       7         8
 * ACRO  DDE_ACRO  FID  RIPPLES_TO  MF_type  MF_len  RWF_type  RWF_len */

enum MF_type {
  MF_NONE         = 0,
  MF_ALPHANUMERIC = 1,
  MF_TIME         = 2,
  MF_DATE         = 3,
  MF_ENUMERATED   = 4,
  MF_INTEGER      = 5,
  MF_PRICE        = 6,
  MF_TIME_SECONDS = 7,
  MF_BINARY       = 8  /* binary + uint = mf encoded integer */
};

enum RWF_type {
  RWF_NONE         = 0,
  RWF_INT          = 3, /* int64, int32 (rare) */
  RWF_UINT         = 4, /* uint64 */
  RWF_REAL         = 8, /* real64 */
  RWF_DATE         = 9,
  RWF_TIME         = 10,
  RWF_ENUM         = 14,
  RWF_ARRAY        = 15,
  RWF_BUFFER       = 16,
  RWF_ASCII_STRING = 17,
  RWF_RMTES_STRING = 19, /* Reuters Multilingual Text Enc */
  RWF_FIELD_LIST   = 132,
  RWF_ELEMENT_LIST = 133,
  RWF_FILTER_LIST  = 135,
  RWF_VECTOR       = 136,
  RWF_MAP          = 137,
  RWF_SERIES       = 138
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
  ATK_REAL64       = 10,
  ATK_RMTES_STRING = 11,
  ATK_UINT64       = 12,
  ATK_ASCII_STRING = 13,
  ATK_BINARY       = 14,
  ATK_BUFFER       = 15,
  ATK_NULL         = 16,
  ATK_ENUM         = 17,
  ATK_TIME_SECONDS = 18,
  ATK_INT64        = 19,
  ATK_NONE         = 20,
  ATK_ARRAY        = 21,
  ATK_SERIES       = 22,
  ATK_ELEMENT_LIST = 23,
  ATK_VECTOR       = 24,
  ATK_MAP          = 25
};

struct AppAKeyword {
  const char * str;
  size_t       len;
  AppATok      tok;
};

struct MDDictBuild;
struct AppA : public DictParser {
  int     fid,
          length,
          enum_length,
          rwf_len;
  AppATok field_type,
          rwf_type;
  char    acro[ 256 ],
          dde_acro[ 256 ],
          ripples_to[ 256 ];

  void * operator new( size_t, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  AppA( const char *p ) : DictParser( p, ATK_INT, ATK_IDENT, ATK_ERROR ) {
    this->clear_line();
  }
  ~AppA() {
    this->clear_line();
    this->close();
  }
  void clear_line( void );
  void set_ident( char *id_buf );
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
  AppATok get_token( void );
  bool match( AppAKeyword &kw ) {
    return this->DictParser::match( kw.str, kw.len );
  }
  void get_type_size( MDType &type,  uint32_t &size );

  static AppA * open_path( const char *path,  const char *filename );

  static int parse_path( MDDictBuild &dict_build,  const char *path,
                         const char *fn );
};

}
}
#endif

