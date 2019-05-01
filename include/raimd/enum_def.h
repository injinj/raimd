#ifndef __rai_raimd__enum_def_h__
#define __rai_raimd__enum_def_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

enum EnumDefTok {
  ETK_ERROR = -2,
  ETK_EOF   = -1,
  ETK_IDENT = 0,
  ETK_INT   = 1
};

struct EnumValue {
  EnumValue * next;
  uint32_t value;
  int      lineno;
  size_t   len;
  char     str[ 4 ];

  void * operator new( size_t sz, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  EnumValue( char *s,  size_t sz,  uint32_t v,  int ln )
    : next( 0 ), value( v ), lineno( ln ) {
    ::memcpy( this->str, s, sz );
    this->len = sz;
    this->str[ sz ] = '\0';
  }
  static size_t alloc_size( size_t sz ) {
    return sizeof( EnumValue ) - 4 + sz + 1;
  }
};

struct MDDictBuild;
struct EnumDef : public DictParser {
  uint32_t map_num,
           max_value;
  int      value_lineno;
  size_t   max_len,
           value_cnt;
  MDQueue< EnumValue > acro;
  MDQueue< EnumValue > map;

  void * operator new( size_t sz, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  EnumDef( const char *p ) : DictParser( p, ETK_INT, ETK_IDENT, ETK_ERROR ),
    map_num( 10 ), max_value( 0 ), value_lineno( 0 ), max_len( 0 ),
    value_cnt( 0 ) {}
  ~EnumDef() {
    this->clear_enum();
    this->close();
  }
  void define_enum( MDDictBuild &dict_build );
  void push_acro( char *buf,  size_t len,  int fid,  int ln );
  void push_enum( uint32_t value,  char *buf,  size_t len,  int ln );
  void clear_enum( void );

  EnumDefTok consume( EnumDefTok k,  size_t sz ) {
    return (EnumDefTok) this->DictParser::consume_tok( k, sz );
  }
  EnumDefTok consume_int( void ) {
    return (EnumDefTok) this->DictParser::consume_int_tok();
  }
  EnumDefTok consume_ident( void ) {
    return (EnumDefTok) this->DictParser::consume_ident_tok();
  }
  EnumDefTok consume_string( void ) {
    return (EnumDefTok) this->DictParser::consume_string_tok();
  }
  EnumDefTok consume_hex( void );
  EnumDefTok get_token( void );

  static EnumDef * open_path( const char *path,  const char *filename );

  static int parse_path( MDDictBuild &dict_build,  const char *path,
                         const char *fn );
};

}
}
#endif

