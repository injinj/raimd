#include <stdio.h>
#include <raimd/md_list.h>

using namespace rai;
using namespace md;

static void
lprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

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
      "Test constructing and unpacking md lists\n" );
    return 1;
  }

  char buf[ 1024 ];

  ListMsg::init_auto_unpack();

  size_t count    = 16,
         data_len = 128,
         asz      = ListData::alloc_size( count, data_len );

  printf( "alloc size: %lu\n", asz );
  ::memset( buf, 0, asz );
  ListData list( buf, asz );
  printf( "init: count=%lu data_len=%lu\n", count, data_len );
  list.init( count, data_len );

  #define S( str ) str, sizeof( str ) - 1
  list.lpush( S( "one" ) );
  list.rpush( S( "two" ) );
  list.lset( 0, S( "googletime" ) );
  lprint( buf, asz );
  list.lpush( S( "jumbo" ) );
  list.lset( 0, S( "jumbotron" ) );
  list.rpush( S( "tree" ) );
  lprint( buf, asz );
  list.lset( 2, S( "twofor" ) );
  list.lset( 3, S( "threetrees" ) );
  lprint( buf, asz );
  list.lpush( S( "funk" ) );
  list.lset( 0, S( "funky" ) );
  list.lpush( S( "jar" ) );
  lprint( buf, asz );
  list.lset( 1, S( "funkadelic" ) );
  lprint( buf, asz );
  list.lset( 1, S( "fun" ) );
  lprint( buf, asz );
  list.lpush( S( "super" ) );
  lprint( buf, asz );
  list.rpush( S( "fun" ) );
  lprint( buf, asz );
  list.lpush( S( "godzilla" ) );
  list.rpush( S( "mothra" ) );
  lprint( buf, asz );
  list.lset( 1, S( "jarjarbikini" ) );
  list.lset( 2, S( "funkytown" ) );
  list.lpush( S( "dodge" ) );
  list.rpush( S( "bullet" ) );
  lprint( buf, asz );

  ListVal lv;
  void * s;
  char   buf2[ 64 ];
  bool   is_a;
  size_t buflen;

  for ( int i = 0; i < 5; i++ ) {
    list.rpop( lv );
    buflen = lv.unitary( s, buf2, sizeof( buf2 ), is_a );
    printf( "rpop: %.*s\n", (int) buflen, (char *) s );
  }

  lprint( buf, asz );

  MDOutput mout;
  mout.print_hex( buf, asz );

  return 0;
}

