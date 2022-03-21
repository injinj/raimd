#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_set.h>

using namespace rai;
using namespace md;

static void
sprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  m = MDMsg::unpack( buf, 0, asz, 0, NULL, &mem );
  if ( m != NULL ) {
    m->print( &mout );
  }
  else {
    printf( "unpack failed\n" );
  }
}

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test constructing and unpacking md set\n" );
    return 1;
  }

  char buf[ 1024 ];

  SetMsg::init_auto_unpack();

  size_t count    = 16,
         data_len = 128,
         asz      = SetData::alloc_size( count, data_len );

  printf( "alloc size: %" PRIu64 "\n", asz );
  ::memset( buf, 0, asz );
  SetData set( buf, asz );
  printf( "init: count=%" PRIu64 " data_len=%" PRIu64 "\n", count, data_len );
  set.init( count, data_len );

  #define S( str ) str, sizeof( str ) - 1
  set.sadd( S( "one" ) );
  set.sadd( S( "two" ) );
  sprint( buf, asz );
  set.sadd( S( "jumbo" ) );
  set.srem( S( "one" ) );
  set.sadd( S( "tree" ) );
  sprint( buf, asz );
  set.sadd( S( "funk" ) );
  set.srem( S( "tree" ) );
  set.sadd( S( "jar" ) );
  sprint( buf, asz );
  set.spopn( 3 );
  set.sadd( S( "super" ) );
  set.sadd( S( "godzilla" ) );
  sprint( buf, asz );
  set.sadd( S( "dodge" ) );
  sprint( buf, asz );

  size_t bsz;
  char buf2[ 1024 ];

  bsz = set.used_size( count, data_len );
  ::memset( buf2, 0, bsz );
  SetData set2( buf2, bsz );
  set2.init( count, data_len );
  set.copy( set2 );

  printf( "used size %" PRIu64 " curr size %" PRIu64 "\n", bsz, set.size );
  printf( "  count %" PRIu64 " data_len %" PRIu64 "\n", set.count(),
          set.data_len() );
  printf( "  used count %" PRIu64 " data_len %" PRIu64 "\n", count, data_len );

  MDOutput mout;
  mout.print_hex( buf, asz );

  sprint( buf2, bsz );
  mout.print_hex( buf2, bsz );

  return 0;
}

