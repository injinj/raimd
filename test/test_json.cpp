#include <stdio.h>
#include <raimd/json.h>
#include <raimd/json_msg.h>
#include <raimd/md_msg.h>
#include <raimd/rv_msg.h>

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
     "      connect: [ \"unknownhost\", \"host2\" ]\n"
     "      port:\n"
     "        - 1234\n"
     "        - 7890\n";

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

  {
    static char minput[] =
      "{\n"
      "  \"MSG_TYPE\" : \"INITIAL\",\n"
      "  \"SUB1\"     : {\n"
      "    \"SVAL\"   : \"hello world\",\n"
      "    \"FVAL\"   : 16.16,\n"
      "    \"IVAL\"   : 16,\n"
      "    \"BVAL\"   : true\n"
      "  },\n"
      "  \"SUB2\"     : {\n"
      "    \"IARRAY\" : [100,200,300,true,false],\n"
      "    \"FARRAY\" : [1.1,2.2,3],\n"
      "    \"SARRAY\" : [\"array\",\"hello\",\"world\",1,2,3]\n"
      "  }\n"
      "}";
    JsonMsgCtx   ctx;
    jmem.reuse();
    printf( "parsing:  %s\n", minput );
    n = ctx.parse( minput, 0, ::strlen( minput ), NULL, jmem, false );
    if ( n == 0 ) {
      printf( "printing:\n" ); ctx.msg->print( &jout );

      char buf[ sizeof( minput ) * 8 ];
      RvMsgWriter rvmsg( jmem, buf, sizeof( buf ) );
      n = rvmsg.convert_msg( *ctx.msg, false );
      if ( n == 0 ) {
        printf( "converting to rv:\n" );
        rvmsg.update_hdr();
        RvMsg * msg = RvMsg::unpack_rv( rvmsg.buf, 0, rvmsg.off, 0, NULL,
                                        jmem );
        msg->print( &jout );
      }
      else {
        printf( "status %d: %s\n", n, Err::err( n )->descr );
      }
    }
    else {
      printf( "status %d: %s\n", n, Err::err( n )->descr );
    }
  }

  return 0;
}

