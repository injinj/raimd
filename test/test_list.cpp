#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
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

  printf( "alloc size: %" PRIu64 "\n", asz );
  ::memset( buf, 0, asz );
  ListData list( buf, asz );
  printf( "init: count=%" PRIu64 " data_len=%" PRIu64 "\n", count, data_len );
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
  for ( int i = 0; i < 5; i++ ) {
    list.rpop( lv );
    printf( "rpop: %.*s%.*s\n", (int) lv.sz, (char *) lv.data,
                                (int) lv.sz2, (char *) lv.data2 );
  }
  size_t bsz;
  char buf2[ 1024 ];

  bsz = list.used_size( count, data_len );
  ::memset( buf2, 0, bsz );
  ListData list2( buf2, bsz );
  list2.init( count, data_len );
  list.copy( list2 );

  printf( "used size %" PRIu64 " curr size %" PRIu64 "\n", bsz, list.size );
  printf( "  count %" PRIu64 " data_len %" PRIu64 "\n", list.count(),
          list.data_len() );
  printf( "  used count %" PRIu64 " data_len %" PRIu64 "\n", count, data_len );

  MDOutput mout;
  lprint( buf, asz );
  mout.print_hex( buf, asz );

  lprint( buf2, bsz );
  mout.print_hex( buf2, bsz );

  return 0;
}

