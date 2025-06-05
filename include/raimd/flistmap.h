#ifndef __rai_raimd__flistmap_h__
#define __rai_raimd__flistmap_h__

#include <raimd/md_types.h>

#ifdef __cplusplus
extern "C" {
#endif
  
#ifdef __cplusplus
}   
namespace rai {
namespace md {

enum FlistMapTok {
  FLIST_ERROR = -2,
  FLIST_EOF   = -1,
  FLIST_IDENT = 0,
  FLIST_INT   = 1,
};

struct MDDictBuild;
struct MDDictAdd;
struct FlistMap : public DictParser {
  int  fid;
  char formclass[ 256 ];

  void * operator new( size_t, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  FlistMap( const char *p,  int debug_flags )
      : DictParser( p, FLIST_INT, FLIST_IDENT, FLIST_ERROR, debug_flags,
                    "Flist Map Dictionary" ) {
    this->clear_line();
  }
  ~FlistMap() {
    this->clear_line();
    this->close();
  }
  void clear_line( void ) noexcept;
  FlistMapTok consume( FlistMapTok k,  size_t sz ) {
    return (FlistMapTok) this->DictParser::consume_tok( k, sz );
  }
  FlistMapTok consume_int( void ) {
    return (FlistMapTok) this->DictParser::consume_int_tok();
  }
  FlistMapTok consume_ident( void ) {
    return (FlistMapTok) this->DictParser::consume_ident_tok();
  }
  FlistMapTok get_token( void ) noexcept;

  static FlistMap * open_path( const char *path,  const char *filename,
                               int debug_flags ) noexcept;
  static int parse_path( MDDictBuild &dict_build,  const char *path,
                         const char *fn ) noexcept;
};

}
}
#endif
#endif
