#ifndef __rai_raimd__dict_load_h__
#define __rai_raimd__dict_load_h__

#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/flistmap.h>
#include <raimd/enum_def.h>

namespace rai {
namespace md {

static MDDict *
load_dict_files( const char *path,  bool verbose = true ) noexcept
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  int x, y, z;
  if ( verbose )
    dict_build.debug_flags = MD_DICT_PRINT_FILES;
  if ( (x = CFile::parse_path( dict_build, path, "tss_fields.cf" )) == 0 ) {
    CFile::parse_path( dict_build, path, "tss_records.cf" );
    dict_build.index_dict( "cfile", dict ); /* dict contains index */
  }
  dict_build.clear_build(); /* frees temp memory used to index dict */
  if ( (y = AppA::parse_path( dict_build, path, "RDMFieldDictionary" )) == 0){
    EnumDef::parse_path( dict_build, path, "enumtype.def" );
    dict_build.index_dict( "app_a", dict ); /* dict is a list */
  }
  dict_build.clear_build();
  if ( (z = FlistMap::parse_path( dict_build, path, "flistmapping" )) == 0 ) {
    dict_build.index_dict( "flist", dict );
  }
  dict_build.clear_build();
  if ( dict != NULL ) { /* print which dictionaries loaded */
    if ( verbose )
      fprintf( stderr, "%s dict loaded (size: %u)\n", dict->dict_type,
               dict->dict_size );
    if ( dict->next != NULL ) {
      if ( verbose )
        fprintf( stderr, "%s dict loaded (size: %u)\n", dict->next->dict_type,
                 dict->next->dict_size );
      if ( dict->next->next != NULL ) {
        if ( verbose )
          fprintf( stderr, "%s dict loaded (size: %u)\n",
                   dict->next->next->dict_type, dict->next->next->dict_size );
      }
    }
    return dict;
  }
  if ( verbose )
    fprintf( stderr, "cfile status %d+%s, RDM status %d+%s flist status %d+%s\n",
            x, Err::err( x )->descr, y, Err::err( y )->descr,
            z, Err::err( z )->descr );
  return NULL;
}

}
}

#endif

