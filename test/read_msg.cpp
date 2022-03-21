#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/enum_def.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>

using namespace rai;
using namespace md;

static MDDict *
load_dict_files( const char *path ) noexcept
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  int x, y;
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
  if ( dict != NULL ) { /* print which dictionaries loaded */
    printf( "%s dict loaded (size: %u)\n", dict->dict_type,
            dict->dict_size );
    if ( dict->next != NULL )
      printf( "%s dict loaded (size: %u)\n", dict->next->dict_type,
              dict->next->dict_size );
    return dict;
  }
  printf( "cfile status %d+%s, RDM status %d+%s\n",
          x, Err::err( x )->descr, y, Err::err( y )->descr );
  return NULL;
}

static int
get_arg( const char *arg,  int argc,  char **argv ) noexcept
{
  for ( int i = 1; i < argc; i++ )
    if ( ::strcmp( argv[ i ], arg ) == 0 )
      return i;
  return -1;
}

int
main( int argc, char **argv )
{
  MDOutput mout; /* message output, uses printf to stdout */
  MDMsgMem mem;  /* memory for printing (converting numbers to strings) */
  MDDict * dict; /* dictinonaries, cfile and RDM/appendix_a */
  char subj[ 1024 ], size[ 128 ];
  size_t sz, buflen = 0;
  int fn_arg = get_arg( "-f", argc, argv ),
      path_arg = get_arg( "-p", argc, argv ),
      quiet_arg = get_arg( "-q", argc, argv );
  char * buf = NULL;
  const char * path, * fn;
  uint64_t msg_count = 0;

  if ( get_arg( "-h", argc, argv ) > 0 ) {
    fprintf( stderr,
      "Usage: %s [-f file] [-p cfile_path] [-q]\n"
      "Test reading messages from files on command line\n"
      "Each file is formatted as any number of these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n", argv[ 0 ] );
    return 1;
  }
  if ( path_arg > 0 && path_arg + 1 < argc )
    path = argv[ path_arg + 1 ];
  else
    path = ::getenv( "cfile_path" );
  if ( fn_arg > 0 && fn_arg + 1 < argc )
    fn = argv[ fn_arg + 1 ];
  else
    fn = NULL;
  dict = load_dict_files( path );
  printf( "reading from %s\n", ( fn == NULL ? "stdin" : fn ) );
  /* unpack auto recognizes messages by magic numbers, etc */
  /*md_init_auto_unpack();*/

  /* read subject, size, message data */
  if ( argc > 1 ) {
    FILE *fp = ( fn == NULL ? stdin : fopen( fn, "rb" ) );
    if ( fp == NULL ) {
      perror( fn );
      return 1;
    }
    for (;;) {
      if ( fgets( subj, sizeof( subj ), fp ) == NULL ||
           fgets( size, sizeof( size ), fp ) == NULL )
        break;
      if ( (sz = atoi( size )) == 0 )
        break;
      if ( sz > buflen ) {
        buf = (char *) ::realloc( buf, sz );
        buflen = sz;
      }
      if ( fread( buf, 1, sz, fp ) != sz ) /* message data */
        break;
      /* skip msgs filtered by argv[ 2 ]
      if ( 2 < argc && ::strstr( subj, argv[ 2 ] ) == NULL )
        continue; */
      /* try to unpack it */
      MDMsg * m = MDMsg::unpack( buf, 0, sz, 0, dict, &mem );
#if 0
      MDFieldIter *iter;
      if ( m->get_field_iter( iter ) == 0 ) {
        if ( iter->first() == 0 ) {
          do {
            MDName name;
            if ( iter->get_name( name ) == 0 && name.fid < 100 )
              cnt++;
          } while ( iter->next() == 0 );
        }
      }
#endif
      /* print it */
      if ( quiet_arg < 0 ) {
        printf( "\n## %s", subj );
        if ( m != NULL )
          m->print( &mout );
      }
      msg_count++;
      mem.reuse(); /* reset mem for next msg */
    }
    printf( "found %" PRIu64 " messages\n", msg_count );
    if ( fn != NULL && fp != NULL )
      fclose( fp );
  }
  return 0;
}

