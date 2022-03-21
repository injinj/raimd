#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_hash.h>

using namespace rai;
using namespace md;

static void
hprint( void *buf,  size_t asz )
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  m = MDMsg::unpack( buf, 0, asz, 0, NULL, &mem );
  if ( m != NULL ) {
    printf( "hash:\n" );
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
      "Test constructing and unpacking md hashes\n" );
    return 1;
  }

  char buf[ 1024 ];

  HashMsg::init_auto_unpack();

  size_t count    = 32,
         data_len = 256,
         asz      = HashData::alloc_size( count, data_len );

  printf( "alloc size: %" PRIu64 "\n", asz );
  ::memset( buf, 0, asz );
  HashData hash( buf, asz );
  printf( "init: count=%" PRIu64 " data_len=%" PRIu64 "\n", count, data_len );
  hash.init( count, data_len );

  #define S( str ) str, sizeof( str ) - 1
  hash.hset( S( "one" ), S( "1" ) );
  hash.hset( S( "two" ), S( "2" ) );
  hash.hset( S( "three" ), S( "3" ) );
  hash.hset( S( "four" ), S( "4" ) );
  hash.hset( S( "five" ), S( "5" ) );
  hprint( buf, asz );
  hash.hset( S( "six" ), S( "6" ) );
  hash.hset( S( "seven" ), S( "7" ) );
  hash.hset( S( "eight" ), S( "8" ) );
  hash.hset( S( "nine" ), S( "9" ) );
  hash.hset( S( "ten" ), S( "10" ) );
  hash.hset( S( "eleven" ), S( "11" ) );
  hash.hset( S( "twelve" ), S( "12" ) );
  hash.hset( S( "thirteen" ), S( "13" ) );
  hash.hset( S( "fourteen" ), S( "14" ) );
  hash.hset( S( "fifteen" ), S( "15" ) );
  hash.hset( S( "sixteen" ), S( "16" ) );
  hash.hset( S( "seventeen" ), S( "17" ) );
  hash.hset( S( "eighteen" ), S( "18" ) );
  hash.hset( S( "nineteen" ), S( "19" ) );
  hash.hset( S( "twenty" ), S( "20" ) );
  hash.hset( S( "twentyone" ), S( "21" ) );
  hash.hset( S( "twentytwo" ), S( "22" ) );
  hash.hset( S( "twentythree" ), S( "23" ) );
  hash.hset( S( "twentyfour" ), S( "24" ) );
  hprint( buf, asz );
  hash.hdel( S( "eight" ) );
  hash.hdel( S( "fourteen" ) );
  hash.hdel( S( "twenty" ) );
  hash.hdel( S( "ten" ) );
  hash.hdel( S( "six" ) );
  hash.hset( S( "twentyfive" ), S( "25" ) );
  hprint( buf, asz );
  hash.hdel( S( "one" ) );
  hash.hset( S( "two" ), S( "twotwotwotwotwotwotwo" ) );
  hprint( buf, asz );

  size_t bsz;
  char buf2[ 1024 ];

  bsz = hash.used_size( count, data_len );
  ::memset( buf2, 0, bsz );
  HashData hash2( buf2, bsz );
  hash2.init( count, data_len );
  hash.copy( hash2 );
  hprint( buf2, bsz );

  printf( "used size %" PRIu64 " curr size %" PRIu64 "\n", bsz, hash.size );
  printf( "  count %" PRIu64 " data_len %" PRIu64 "\n", hash.count(),
          hash.data_len() );
  printf( "  used count %" PRIu64 " data_len %" PRIu64 "\n", count, data_len );

  MDOutput mout;
  mout.print_hex( buf, asz );

  mout.print_hex( buf2, bsz );

  return 0;
}

