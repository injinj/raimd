#include <stdio.h>
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
load_dict_files( const char *path )
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  int x, y;
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

int
main( int argc, char **argv )
{
  MDOutput mout; /* message output, uses printf to stdout */
  MDMsgMem mem;  /* memory for printing (converting numbers to strings) */
  MDDict * dict; /* dictinonaries, cfile and RDM/appendix_a */
  char subj[ 1024 ], size[ 128 ];
  size_t sz, buflen = 0;
  char * buf = NULL;

  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test reading messages from files on command line\n"
      "Each file is formatted as any number of these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n" );
    return 1;
  }

  dict = load_dict_files( ::getenv( "cfile_path" ) );

  /* unpack auto recognizes messages by magic numbers, etc */
  JsonMsg::init_auto_unpack();
  RvMsg::init_auto_unpack();
  TibMsg::init_auto_unpack();
  TibSassMsg::init_auto_unpack();
  MktfdMsg::init_auto_unpack();
  RwfMsg::init_auto_unpack();

  /* read subject, size, message data */
  if ( argc > 1 ) {
    FILE *fp = fopen( argv[ 1 ], "r" );
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
      /* skip msgs filtered by argv[ 2 ] */
      if ( 2 < argc && ::strstr( subj, argv[ 2 ] ) == NULL )
        continue;
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
      printf( "\n## %s", subj );
      if ( m != NULL )
        m->print( &mout );
      mem.reuse(); /* reset mem for next msg */
    }
    fclose( fp );
  }
  return 0;
}

