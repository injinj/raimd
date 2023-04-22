#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <math.h>
#include <raimd/md_zset.h>

using namespace rai;
using namespace md;

static void
zprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDOutput mout;
  MDMsg  * m;

  printf( "zset:\n" );
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

  ZSetMsg::init_auto_unpack();

  size_t count    = 16,
         data_len = 128,
         asz      = ZSetData::alloc_size( count, data_len );

  if ( asz > sizeof( buf ) ) {
    fprintf( stderr, "too big\n" );
    return 1;
  }
  printf( "alloc size: %" PRIu64 "\n", asz );
  ::memset( buf, 0, asz );
  ZSetData zset( buf, asz );
  printf( "init: count=%" PRIu64 " data_len=%" PRIu64 "\n", count, data_len );
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
  zset.zadd( S( "jar" ), F( -6.66e3 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "super" ), F( 7.75 ), ZADD_INCR );
  zset.zadd( S( "godzilla" ), F( 7.7 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zadd( S( "dodge" ), F( 7.6 ), ZADD_INCR );
  zset.zadd( S( "ford" ), F( 3.6 ), ZADD_INCR );
  zset.zadd( S( "jar" ), F( 7000 ), ZADD_INCR );
  zprint( buf, asz );
  zset.zrem( S( "funk" ) );
  zset.zrem( S( "two" ) );
  zset.zrem( S( "super" ) );
  zset.zrem( S( "dodge" ) );
  zprint( buf, asz );

  size_t bsz;
  char buf2[ 1024 ];

  bsz = zset.used_size( count, data_len );
  if ( bsz > sizeof( buf2 ) ) {
    fprintf( stderr, "too big\n" );
    return 1;
  }
  ::memset( buf2, 0, bsz );
  ZSetData zset2( buf2, bsz );
  zset2.init( count, data_len );
  zset.copy( zset2 );
  zprint( buf2, bsz );

  printf( "used size %" PRIu64 " curr size %" PRIu64 "\n", bsz, zset.size );
  printf( "  count %" PRIu64 " data_len %" PRIu64 "\n", zset.count(),
          zset.data_len() );
  printf( "  used count %" PRIu64 " data_len %" PRIu64 "\n", count, data_len );

  MDOutput mout;
  mout.print_hex( buf, asz );

  mout.print_hex( buf2, bsz );

  return 0;
}

