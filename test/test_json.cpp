#include <stdio.h>
#include <raimd/json.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr, "Test parsing and printing json\n" );
    return 1;
  }

  const char *sinput[] = { "[ 1.1, 2.34, 3.6789 ]",
                           "198.185",
                           "\"hello world\"",
                           "{ \"x\" : \"hello world\", y : [1,2,3], "
                           "\"z\" : [null,\"xxx\",10.11,[]] }",
                           "true",
                           "false",
                           "{}",
                           "[]",
                           "x : 1",
                           "\"hello",
                           "[1,2,3,4",
                           "{x : 1, y"
                         };
  MDMsgMem   jmem;
  JsonParser jparse( jmem );
  MDOutput   jout;
  int        n;

  for ( size_t i = 0; i < sizeof( sinput ) / sizeof( sinput[ 0 ] ); i++ ) {
    JsonBufInput input( sinput[ i ], 0, ::strlen( sinput[ i ] ) );
    jmem.reuse();
    printf( "parsing:  %s\n", sinput[ i ] );
    if ( (n = jparse.parse( input )) == 0 ) {
      printf( "printing: " ); jparse.value->print( &jout );
      printf( "\n" );
    }
    else {
      printf( "status %d: %s\n", n, Err::err( n )->descr );
    }
  }
  return 0;
}

