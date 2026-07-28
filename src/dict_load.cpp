#include <stdio.h>
#include <raimd/dict_load.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

extern "C" {

MDDict_t *
md_load_dict_files( const char *path,  bool verbose )
{
  return (MDDict_t *) rai::md::load_dict_files( path, verbose );
}

MDDict_t *
md_load_sass_dict( MDMsg_t *m )
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  if ( CFile::unpack_sass( dict_build, static_cast<MDMsg *>( m ) ) != 0 ) {
    fprintf( stderr, "Dict index error\n" );
    return NULL;
  }
  dict_build.index_dict( "cfile", dict );
  dict_build.clear_build();
  return dict;
}

}

MDDict *
rai::md::load_dict_files( const char *path,  bool verbose ) noexcept
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
      fprintf( stderr, "%s dict loaded (size: %u id: %x)\n", dict->dict_type,
               dict->dict_size, dict->dict_hash_id );
    if ( dict->get_next() != NULL ) {
      if ( verbose )
        fprintf( stderr, "%s dict loaded (size: %u id: %x)\n", dict->get_next()->dict_type,
                 dict->get_next()->dict_size, dict->get_next()->dict_hash_id );
      if ( dict->get_next()->get_next() != NULL ) {
        if ( verbose )
          fprintf( stderr, "%s dict loaded (size: %u id: %x)\n",
                   dict->get_next()->get_next()->dict_type,
                   dict->get_next()->get_next()->dict_size,
                   dict->get_next()->get_next()->dict_hash_id );
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

bool
MDMsgDict::load( const char *path,  bool verbose ) noexcept
{
  if ( path != NULL )
    this->dict = load_dict_files( path, verbose );
  if ( this->dict != NULL ) {
    for ( MDDict *d = this->dict; d != NULL; d = d->get_next() ) {
      if ( d->dict_type[ 0 ] == 'c' )
        this->cfile_dict = d;
      else if ( d->dict_type[ 0 ] == 'a' )
        this->rdm_dict = d;
      else if ( d->dict_type[ 0 ] == 'f' )
        this->flist_dict = d;
    }
  }
  return this->dict != NULL;
}
