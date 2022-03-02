#include <stdio.h>
#include <inttypes.h>
#include <raimd/md_types.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test real to decimal conversion and string to decimal parsing\n" );
    return 1;
  }

  static double x[] = { 10.0, 10.01, 0.0001, 123.456, 1e-6,
                        1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 248e+20,
                        1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 1e-16 };
  const char   *y[] = { "-0.11", "10.0", "10.01", "0.0001", "123.456", "123 456",
       "1e-6", "1e+16", "1e+17", "1e+18", "1e+19", "1e+20", "1e+21", "1e+22",
       "1e-10", "1e-11", "1e-12", "1e-13", "1e-14", "1e-15", "1e-16" };
  char buf[ 64 ];
  MDDecimal fp;
  size_t i, n;
  
  for ( i = 0; i < sizeof( x ) / sizeof( x[ 0 ] ); i++ ) {
    fp.set_real( x[ i ] );
    n = fp.get_string( buf, sizeof( buf ) );
    printf( "set_real(%f) = [%.*s] (%" PRIu64 ") (%" PRIu64 "/h=%d)\n", x[ i ],
            (int) n, buf, n, fp.ival, fp.hint );
  }
  for ( i = 0; i < sizeof( y ) / sizeof( y[ 0 ] ); i++ ) {
    fp.parse( y[ i ], ::strlen( y[ i ] ) );
    n = fp.get_string( buf, sizeof( buf ) );
    printf( "parse   (%s) = [%.*s] (%" PRIu64 ") (%" PRIu64 "/h=%d)\n", y[ i ],
            (int) n, buf, n, fp.ival, fp.hint );
  }
  return 0;
}

