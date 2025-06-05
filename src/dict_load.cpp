#include <stdio.h>
#include <raimd/dict_load.h>

extern "C" {

MDDict_t *
md_load_dict_files( const char *path,  bool verbose )
{
  return (MDDict_t *) rai::md::load_dict_files( path, verbose );
}

}

namespace rai {
namespace md {

MDDict *
load_dict_files( const char *path,  bool verbose ) noexcept
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
    if ( dict->get_next() != NULL ) {
      if ( verbose )
        fprintf( stderr, "%s dict loaded (size: %u)\n", dict->get_next()->dict_type,
                 dict->get_next()->dict_size );
      if ( dict->get_next()->get_next() != NULL ) {
        if ( verbose )
          fprintf( stderr, "%s dict loaded (size: %u)\n",
                   dict->get_next()->get_next()->dict_type, dict->get_next()->get_next()->dict_size );
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
