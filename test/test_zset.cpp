#include <stdio.h>
#include <math.h>
#include <raimd/md_zset.h>

using namespace rai;
using namespace md;

static void
zprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  printf( "zset:\n" );
  m = MDMsg::unpack( buf, 0, asz, 0, NULL, &mem );
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

  ZSetMsg::init_auto_unpack();

  size_t count    = 16,
         data_len = 128,
         asz      = ZSetData::alloc_size( count, data_len );

  printf( "alloc size: %lu\n", asz );
  ::memset( buf, 0, asz );
  ZSetData zset( buf, asz );
  printf( "init: count=%lu data_len=%lu\n", count, data_len );
  zset.init( count, data_len );

  #define S( str ) str, sizeof( str ) - 1
  #define F( f ) Decimal64::ftod( f )
  
  zset.zadd( S( "one" ), F( 0.011 ), ZADD_INCR );
  zset.zadd( S( "two" ), F( INFINITY ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "jumbo" ), F( 1.3 ), ZADD_INCR );
  zset.zrem( S( "one" ) );
  zset.zadd( S( "tree" ), F( 4.4 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "funk" ), F( 2.2 ), ZADD_INCR );
  zset.zrem( S( "tree" ) );
  zset.zadd( S( "jar" ), F( -6.66e18 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "super" ), F( 7.75 ), ZADD_INCR );
  zset.zadd( S( "godzilla" ), F( 7.7 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "dodge" ), F( 7.6 ), ZADD_INCR );
  zset.zadd( S( "ford" ), F( 3.6 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zrem( S( "funk" ) );
  zset.zrem( S( "two" ) );
  zset.zrem( S( "super" ) );
  zset.zrem( S( "dodge" ) );
  zprint( buf, asz );

  return 0;
}

