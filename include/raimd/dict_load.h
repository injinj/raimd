#ifndef __rai_raimd__dict_load_h__
#define __rai_raimd__dict_load_h__

#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/flistmap.h>
#include <raimd/enum_def.h>

#ifdef __cplusplus
extern "C" {
#endif

MDDict_t * md_load_dict_files( const char *path,  bool verbose );
struct MDMsg_s;
MDDict_t * md_load_sass_dict( struct MDMsg_s *m );

#ifdef __cplusplus
}
namespace rai {
namespace md {

MDDict * load_dict_files( const char *path,  bool verbose = true ) noexcept;

}
}

#endif
#endif
