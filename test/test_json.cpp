#include <stdio.h>
#include <raimd/json.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  MDMsgMem jmem;
  MDOutput jout;
  int      n;
  bool     read_stdin = false,
           do_yaml = false;

  for ( n = 1; n < argc; n++ ) {
    if ( ::strcmp( argv[ n ], "-h" ) == 0 ) {
      fprintf( stderr, "%s [-i] [-y]\n", argv[ 0 ] );
      return 1;
    }
    if ( ::strcmp( argv[ n ], "-i" ) == 0 )
      read_stdin = true;
    else if ( ::strcmp( argv[ n ], "-y" ) == 0 )
      do_yaml = true;
  }
  if ( read_stdin ) {
    JsonStreamInput input;
    JsonParser jparse2( jmem );
    printf( "parsing stdin %s\n", do_yaml ? "yaml" : "json" );
    if ( do_yaml ) {
      n = jparse2.parse_yaml( input );
    }
    else {
      n = jparse2.parse( input );
    }
    if ( n == 0 ) {
      printf( "printing: " ); jparse2.value->print( &jout );
      printf( "\n" );
    }
    else {
      printf( "status %d: %s\n", n, Err::err( n )->descr );
    }
    return 0;
  }

  const char *sinput[] = {
    "[ 1.1, 2.34, 3.6789 ]",
     "198.185",
     "\"hello world\"",
     "{ \"x\" : \"hello world\", y : [1,2,3], "
     "\"z\" : [null,\"xxx\",10.11,[]] }",
     "true",
     "false",
     "{session : \"newuser1.NONCE\", seqno : 1, challenge : \"HMAC-SHA256\"}",
     "{}",
     "[]",
     "x : 1",
     "\"hello",
     "[1,2,3,4",
     "{x : 1, y"
   };
   static char yinput[] =
     "\"field\":\n"
     "  - \"tport\": \"unknown\"\n"
     "    type: tcp\n"
     "    route:\n"
     "      listen: '*'\n"
     "      connect: unknownhost\n";

  JsonParser jparse( jmem );

  if ( do_yaml ) {
    JsonBufInput input( yinput, 0, ::strlen( yinput ) );
    printf( "parsing:  yaml\n" );
    if ( (n = jparse.parse_yaml( input )) == 0 ) {
      printf( "printing: " ); jparse.value->print( &jout );
      printf( "\n" );
    }
    else {
      printf( "status %d: %s\n", n, Err::err( n )->descr );
    }
  }
  else {
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
  }
  return 0;
}

