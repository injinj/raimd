#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <math.h>
#include <raimd/md_geo.h>

using namespace rai;
using namespace md;

static void
gprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  printf( "geo:\n" );
  m = MDMsg::unpack( buf, 0, asz, 0, NULL, mem );
  if ( m != NULL ) {
    m->print( &mout );
  }
  else {
    printf( "unpack failed\n" );
  }
  mout.print_hex( buf, asz );
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

  GeoMsg::init_auto_unpack();

  size_t count    = 16,
         data_len = 128,
         asz      = GeoData::alloc_size( count, data_len );

  if ( asz > sizeof( buf ) ) {
    fprintf( stderr, "too big\n" );
    return 1;
  }
  printf( "alloc size: %" PRIu64 "\n", asz );
  ::memset( buf, 0, asz );
  GeoData geo( buf, asz );
  printf( "init: count=%" PRIu64 " data_len=%" PRIu64 "\n", count, data_len );
  geo.init( count, data_len );

  #define S( str ) str, sizeof( str ) - 1
  
  geo.geoadd( S( "one" ), 0x11 );
  geo.geoadd( S( "two" ), 0x22 );
  gprint( buf, asz );
  geo.geoadd( S( "jumbo" ), 0x13 );
  geo.georem( S( "one" ) );
  geo.geoadd( S( "tree" ), 0x24 );
  gprint( buf, asz );
  geo.geoadd( S( "funk" ), 0x642 );
  geo.georem( S( "tree" ) );
  geo.geoadd( S( "jar" ), 0x8888 );
  gprint( buf, asz );
  geo.geoadd( S( "super" ), 0x33 );
  geo.geoadd( S( "godzilla" ), 0x4 );
  gprint( buf, asz );
  geo.geoadd( S( "dodge" ), 0x98 );
  geo.geoadd( S( "ford" ), 0x99 );
  gprint( buf, asz );
  geo.georem( S( "funk" ) );
  geo.georem( S( "two" ) );
  geo.georem( S( "super" ) );
  geo.georem( S( "dodge" ) );
  gprint( buf, asz );

  return 0;
}

