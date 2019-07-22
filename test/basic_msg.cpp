#include <stdio.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test unpacking md msg basic types\n" );
    return 1;
  }

  MDOutput mout;
  MDMsgMem mem;
  MDMsg  * m;

  int i = 10;
  m = MDMsg::unpack( &i, 0, sizeof( i ), MD_INT, NULL, &mem );
  printf( "Int test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  double d = 11.11;
  m = MDMsg::unpack( &d, 0, sizeof( d ), MD_REAL, NULL, &mem );
  printf( "Real test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  char buf[ 16 ];
  ::strcpy( buf, "hello world" );
  m = MDMsg::unpack( buf, 0, strlen( buf ) + 1, MD_STRING, NULL, &mem );
  printf( "String test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  MDDecimal dec;
  dec.ival = 10001;
  dec.hint = MD_DEC_LOGn10_3;
  m = MDMsg::unpack( &dec, 0, sizeof( dec ), MD_DECIMAL, NULL, &mem );
  printf( "Decimal test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  return 0;
}

