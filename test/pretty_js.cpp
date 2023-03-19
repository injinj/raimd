#include <stdio.h>
#include <raimd/json.h>
#include <raimd/json_msg.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  MDMsgMem   jmem;
  MDOutput   jout;
  JsonMsgCtx ctx;
  int        n;
  bool       do_yaml = false,
             do_md   = false;

  for ( n = 1; n < argc; n++ ) {
    if ( ::strcmp( argv[ n ], "-h" ) == 0 ) {
      fprintf( stderr, "%s [-y] [-m]\n", argv[ 0 ] );
      return 1;
    }
    if ( ::strcmp( argv[ n ], "-y" ) == 0 )
      do_yaml = true;
    else if ( ::strcmp( argv[ n ], "-m" ) == 0 )
      do_md = true;
  }
  n = ctx.parse_fd( 0, NULL, &jmem, true );
  if ( n == 0 ) {
    if ( do_yaml )
      ctx.msg->js->print_yaml( &jout );
    else if ( do_md )
      ctx.msg->print( &jout );
    else
      ctx.msg->js->print_json( &jout );
    printf( "\n" );
    return 0;
  }
  fprintf( stderr, "status %d: %s\n", n, Err::err( n )->descr );
  return 1;
}

