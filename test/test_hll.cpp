#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_hll.h>
/*#include "../meow_intrinsics.h"
#include "../meow_hash.h"*/
/*#include "../aes_hash.h"*/

using namespace rai;
using namespace md;

struct XoroRand {
  uint64_t state[ 2 ];

  static inline uint64_t rotl( const uint64_t x,  int k ) {
    return (x << k) | (x >> (64 - k));
  }
  XoroRand( uint64_t s1,  uint64_t s2 ) {
    this->state[ 0 ] = s1;
    this->state[ 1 ] = s2;
  }
  uint64_t current( void ) const {
    return this->state[ 0 ] + this->state[ 1 ];
  }
  void incr( void ) {
    const uint64_t s0 = this->state[ 0 ];
    const uint64_t s1 = this->state[ 1 ] ^ s0;
    this->state[ 0 ] = rotl(s0, 55) ^ s1 ^ (s1 << 14); /* a, b */
    this->state[ 1 ] = rotl(s1, 36); /* c */
  }
  uint64_t next( void ) {
    const uint64_t result = this->current();
    this->incr();
    return result;
  }
};

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr, "Test hyper log log\n" );
    return 1;
  }
  HyperLogLog test1;
  XoroRand xrand( 0x9e3779b9ULL * 13, 0x7f4a7c13ULL * 37 );
  const double est = 1.04 / sqrt( test1.HTSZ ); /* 1 std dev (66%) */
  uint32_t i, j, n = 0, total = 0;
  static const char red[]   = "\033[91m";
  static const char green[] = "\033[92m";
  static const char norm[]  = "\033[0m";
  test1.init();
  printf( "size %" PRId64 "\n", sizeof( test1 ) );
  for ( j = 0; j < 100; j++ ) {
    n += 1000;
    for ( i = 0; i < n; i++ )
      test1.add( xrand.next() );
    total += n;
    double e1 = test1.estimate(),
           ct = (double) total,
           pt = ( e1 - ct ) / (double) ct;
    const char *p;
    if ( fabs( pt ) > est )
      p = red;
    else
      p = green;
    printf( "total:%u est:%.1f err:%s%.6f%s (%.1f%%)\n", total, test1.estimate(),
            p, pt, norm, 100.0 - pt * 100 );
  }
  HLLMsg::init_auto_unpack();

  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  m = MDMsg::unpack( &test1, 0, sizeof( test1 ), 0, NULL, &mem );
  if ( m != NULL ) {
    m->print( &mout );
  }
  else {
    printf( "unpack failed\n" );
  }
#if 0
  HyperLogLog test2, test3, test4;
  uint64_t k;
  test1.init();
  test2.init();
  test3.init();
  test4.init();
  k = 0;
  uint64_t kr[ 5 ], kg[ 5 ];
  ::memset( kr, 0, sizeof( kr ) );
  ::memset( kg, 0, sizeof( kg ) );
  for ( j = 0; j < 10000; j++ ) {
    for ( i = 0; i < 1000000; i++ ) {
      /*meow_hash h = MeowHash_Accelerated( 0, 0, sizeof( k ), &k );*/
      uint64_t h[ 2 ];
      h[ 0 ] = h[ 1 ] = 0;
      AESHash128( &k, sizeof( k ), &h[ 0 ], &h[ 1 ] );
      test1.add( h[ 0 ] );
      test2.add( h[ 1 ] );
      test3.add( ( (uint64_t) h[ 0 ] >> 32 ) | ( (uint64_t) h[ 1 ] << 32 ) );
      test4.add( ( (uint64_t) h[ 0 ] << 32 ) | ( (uint64_t) h[ 1 ] >> 32 ) );
      k++;
    }
    double e1 = test1.estimate(),
           e2 = test2.estimate(),
           e3 = test3.estimate(),
           e4 = test4.estimate(),
           e5 = ( e1 + e2 + e3 + e4 ) / 4.0;
    double ke[ 5 ];
    ke[ 0 ] = ( e1 - (double) k ) / (double) k;
    ke[ 1 ] = ( e2 - (double) k ) / (double) k;
    ke[ 2 ] = ( e3 - (double) k ) / (double) k;
    ke[ 3 ] = ( e4 - (double) k ) / (double) k;
    ke[ 4 ] = ( e5 - (double) k ) / (double) k;
    printf( "%lum est: ", k / 1000000L );
    for ( int x = 0; x < 5; x++ ) {
      const char *p;
      if ( fabs( ke[ x ] ) > est ) {
        p = red;
        kr[ x ]++;
      }
      else {
        p = green;
        kg[ x ]++;
      }
      printf( "%s%.6f%s ", p, ke[ x ], norm );
    }
    printf( "\n" );
  }
  printf( "green: %lu, %lu, %lu, %lu, %lu\n",
           kg[ 0 ], kg[ 1 ], kg[ 2 ], kg[ 3 ], kg[ 4 ] );
  printf( "red:   %lu, %lu, %lu, %lu, %lu\n",
           kr[ 0 ], kr[ 1 ], kr[ 2 ], kr[ 3 ], kr[ 4 ] );
  double kp[ 5 ];
  for ( int y = 0; y < 5; y++ )
    kp[ y ] = (double) kg[ y ] / ( (double) k / 1000000.0 );
  printf( "pct: %.02f, %.02f, %.02f, %.02f, %.02f\n",
           kp[ 0 ], kp[ 1 ], kp[ 2 ], kp[ 3 ], kp[ 4 ] );
#endif
  return 0;
}
