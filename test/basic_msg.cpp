#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

static void
print_date( MDMsg *m ) noexcept
{
  if ( m != NULL ) {
    MDReference mref;
    if ( m->get_reference( mref ) == 0 ) {
      MDDate d;
      char   buf[ 24 ];
      if ( d.get_date( mref ) == 0 ) {
        size_t len = d.get_string( buf, sizeof( buf ) );
        if ( len > 0 ) {
          printf( "%.*s\n", (int) len, buf );
          return;
        }
      }
    }
  }
  printf( "failed, no date\n" );
}

static void
print_time( MDMsg *m ) noexcept
{
  if ( m != NULL ) {
    MDReference mref;
    if ( m->get_reference( mref ) == 0 ) {
      MDTime t;
      char   buf[ 24 ];
      if ( t.get_time( mref ) == 0 ) {
        size_t len = t.get_string( buf, sizeof( buf ) );
        if ( len > 0 ) {
          printf( "%.*s\n", (int) len, buf );
          return;
        }
      }
    }
  }
  printf( "failed, no time\n" );
}

static void
print_stamp( MDMsg *m ) noexcept
{
  if ( m != NULL ) {
    MDReference mref;
    if ( m->get_reference( mref ) == 0 ) {
      MDStamp t;
      char   buf[ 24 ];
      if ( t.get_stamp( mref ) == 0 ) {
        size_t len = t.get_string( buf, sizeof( buf ) );
        if ( len > 0 ) {
          printf( "%.*s\n", (int) len, buf );
          return;
        }
      }
    }
  }
  printf( "failed, no stamp\n" );
}

static void
scale_stamp( MDMsg *m ) noexcept
{
  if ( m != NULL ) {
    MDReference mref;
    if ( m->get_reference( mref ) == 0 ) {
      MDStamp t;
      if ( t.get_stamp( mref ) == 0 ) {
        printf( "seconds: %" PRIu64 "\n", t.seconds() );
        printf( "millis : %" PRIu64 "\n", t.millis() );
        printf( "micros : %" PRIu64 "\n", t.micros() );
        printf( "nanos  : %" PRIu64 "\n", t.nanos() );
        return;
      }
    }
  }
  printf( "failed, no stamp\n" );
}

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
  m = MDMsg::unpack( &i, 0, sizeof( i ), MD_INT, NULL, mem );
  printf( "Int test: (%d)\n", i );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  double d = 11.11;
  m = MDMsg::unpack( &d, 0, sizeof( d ), MD_REAL, NULL, mem );
  printf( "Real test: (11.11)\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  char buf[ 16 ];
  ::strcpy( buf, "hello world" );
  m = MDMsg::unpack( buf, 0, strlen( buf ) + 1, MD_STRING, NULL, mem );
  printf( "String test (%s):\n", buf );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  MDDecimal dec;
  dec.ival = 1001010101;
  dec.hint = MD_DEC_LOGn10_6;
  for ( i = 0; i < 10; i++ ) {
    char str[ 16 ];
    size_t n;
    m = MDMsg::unpack( &dec, 0, sizeof( dec ), MD_DECIMAL, NULL, mem );
    n = dec.get_string( str, sizeof( str ) );
    str[ n ] = '\0';
    printf( "Decimal test (%s): degrade %u (%ld hint %d)\n", str, i, dec.ival,
            dec.hint );
    if ( m != NULL )
      m->print( &mout );
    dec.degrade();
    mem.reuse();
  }

  static char date1[] = "01/10/21";
  m = MDMsg::unpack( date1, 0, sizeof( date1 ), MD_STRING, NULL, mem );
  printf( "Date test (%s):\n", date1 );
  print_date( m );
  mem.reuse();

  static char time1[] = "01:10:21";
  m = MDMsg::unpack( time1, 0, sizeof( time1 ), MD_STRING, NULL, mem );
  printf( "Time test (%s):\n", time1 );
  print_time( m );
  mem.reuse();

  static const char *stamp1[] = {
    "1618867065.123456789",
    "30 days",
    "30 ms",
    "1.5 weeks",
    "04/19/2021",
    "May 2021",
    "17:00:01.505 May 1, 2021",
    "17:00:01.505 05/01/2021",
    "17:00 May 11, 2021",
    "5 hours",
    NULL
  };
  for ( size_t k = 0; stamp1[ k ] != NULL; k++ ) {
    char stmp[ 32 ];
    ::strcpy( stmp, stamp1[ k ] );
    m = MDMsg::unpack( stmp, 0, ::strlen( stmp ), MD_STRING, NULL, mem );
    printf( "String stamp (%s):\n", stmp );
    print_stamp( m );
    mem.reuse();
  }

  uint64_t x = 1618867065123456789ULL;
  m = MDMsg::unpack( &x, 0, sizeof( x ), MD_UINT, NULL, mem );
  printf( "Int64 stamp (%" PRIu64 "):\n", x );
  print_stamp( m );
  mem.reuse();

  uint64_t o = 100;
  m = MDMsg::unpack( &o, 0, sizeof( o ), MD_UINT, NULL, mem );
  printf( "Int64 stamp (%" PRIu64 "):\n", o );
  print_stamp( m );
  scale_stamp( m );
  mem.reuse();

  double y = 1618867065.123456789;
  m = MDMsg::unpack( &y, 0, sizeof( y ), MD_REAL, NULL, mem );
  printf( "Real stamp (%.9f):\n", y );
  print_stamp( m );
  scale_stamp( m );
  mem.reuse();

  return 0;
}

