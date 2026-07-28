#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_msg.h>
#include <raimd/dict_load.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>

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

  static uint8_t ival[ 8 ] = { 1, 2, 3, 4, 5, 6, 7, 8 };
  uint8_t itest[ 8 ], itest2[ 8 ];

  uint32_t z, a;
  z = get_u32_md_little( ival );
  a = get_u32<MD_LITTLE>( ival );
  set_u32_md_little( itest, 0x01020304 );
  set_u32<MD_LITTLE>( itest2, 0x01020304 );
  printf( "32 little test: %x %x %d %d %d\n", z, a, z == a,
    itest[ 0 ] == 4 && itest[ 3 ] == 1, ::memcmp( itest, itest2, 4 ) == 0 );
  z = get_u32_md_big( ival );
  a = get_u32<MD_BIG>( ival );
  set_u32_md_big( itest, 0x01020304 );
  set_u32<MD_BIG>( itest2, 0x01020304 );
  printf( "32 big test: %x %x %d %d %d\n", z, a, z == a,
    itest[ 0 ] == 1 && itest[ 3 ] == 4, ::memcmp( itest, itest2, 4 ) == 0 );

  uint64_t zz, aa;
  zz = get_u64_md_little( ival );
  aa = get_u64<MD_LITTLE>( ival );
  set_u64_md_little( itest, 0x0102030405060708ULL );
  set_u64<MD_LITTLE>( itest2, 0x0102030405060708ULL );
  printf( "64 little test: %lx %lx %d %d %d\n", zz, aa, zz == aa,
    itest[ 0 ] == 8 && itest[ 7 ] == 1, ::memcmp( itest, itest2, 8 ) == 0 );
  zz = get_u64_md_big( ival );
  aa = get_u64<MD_BIG>( ival );
  set_u64_md_big( itest,  0x0102030405060708ULL );
  set_u64<MD_BIG>( itest2,  0x0102030405060708ULL );
  printf( "64 big test: %lx %lx %d %d %d\n", zz, aa, zz == aa,
    itest[ 0 ] == 1 && itest[ 7 ] == 8, ::memcmp( itest, itest2, 8 ) == 0 );

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
    printf( "Decimal test (%s): degrade %u (%" PRId64 " hint %d)\n", str, i, dec.ival,
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

  MDMsgDict dict;
  dict.load( ::getenv( "cfile_path" ), false );
  const uint8_t mf[] = {
0x1c,0x33,0x31,0x36,0x1f,0x32,0x4d,0x1d,0x49,0x4e,
0x54,0x43,0x2e,0x4f,0x1f,0x32,0x38,0x34,0x37,0x35,0x1e,0x36,0x1f,0x2b,0x36,0x33,
0x20,0x32,0x2f,0x38,0x1e,0x31,0x31,0x1f,0x2b,0x30,0x20,0x33,0x2f,0x38,0x1e,0x35,
0x36,0x1f,0x2b,0x30,0x2e,0x36,0x30,0x1e,0x31,0x38,0x1f,0x31,0x37,0x3a,0x30,0x35,
0x1e,0x33,0x32,0x1f,0x2b,0x32,0x36,0x31,0x38,0x30,0x30,0x30,0x1e,0x31,0x34,0x1f,
0x55,0x1e,0x35,0x1f,0x31,0x37,0x3a,0x30,0x35,0x1e,0x31,0x37,0x38,0x1f,0x2b,0x31,
0x30,0x30,0x1e,0x33,0x37,0x39,0x1f,0x31,0x37,0x3a,0x30,0x35,0x3a,0x31,0x36,0x1e,
0x31,0x30,0x32,0x31,0x1f,0x2b,0x39,0x32,0x34,0x33,0x36,0x1c};
  m = MDMsg::unpack( (void *) mf, 0, sizeof( mf ),
                     (uint8_t) MARKETFEED_TYPE_ID, dict.dict, mem );
  printf( "mf test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

const uint8_t rwf[] = {
0x25,0xcd,0xab,0xca,0x00,0x00,0x00,0x8d,0x00,0x21,0x04,0x06,0x3c,0x35,0x4a,0x65,
0x80,0x18,0x04,0x00,0x00,0x00,0x6f,0x3a,0x80,0x11,0x06,0x0e,0x52,0x53,0x46,0x2e,
0x52,0x45,0x43,0x2e,0x49,0x4e,0x54,0x43,0x2e,0x4f,0x01,0x09,0x03,0x01,0x00,0x0c,
0x00,0x0a,0x00,0x06,0x03,0x19,0x01,0xfa,0x00,0x0b,0x02,0x19,0x03,0x00,0x38,0x02,
0x0c,0x3c,0x00,0x12,0x02,0x11,0x05,0x00,0x20,0x04,0x0e,0x27,0xf2,0x2c,0x00,0x0e,
0x01,0x00,0x00,0x05,0x02,0x11,0x05,0x00,0xb2,0x02,0x0e,0x64,0x01,0x7b,0x03,0x11,
0x05,0x10,0x03,0xfd,0x04,0x0e,0x01,0x69,0x0f};
  m = MDMsg::unpack( (void *) rwf, 0, sizeof( rwf ),
                     (uint8_t) RWF_FIELD_LIST_TYPE_ID, dict.dict, mem );
  printf( "rwf msg test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  m = MDMsg::unpack( (void *) &rwf[ 8 ], 0, sizeof( rwf ) - 8,
                     (uint8_t) RWF_MSG_TYPE_ID, dict.dict, mem );
  printf( "rwf2 msg test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

const uint8_t rwf2[] = {
0x25,0xcd,0xab,0xca,0x00,0x00,0x00,0x84,0x08,0x00,0x0e,0x0f,0x11,0x01,
0x01,0x0f,0x12,0x02,0x36,0x71,0x0f,0x13,0x02,0x2d,0x00,0x03,0xf0,0x02,0x4f,0x4b,
0x00,0x16,0x03,0x0c,0x15,0xf7,0x00,0x19,0x03,0x0c,0x15,0xfc,0x00,0x1e,0x03,0x0c,
0x01,0x90,0x00,0x1f,0x02,0x0c,0x64,0x00,0x76,0x01,0x3c,0x01,0x25,0x06,0x20,0x20,
0x20,0x20,0x00,0x00,0x01,0x28,0x06,0x20,0x20,0x20,0x20,0x00,0x00,0x04,0x01,0x05,
0x12,0x1a,0x11,0xff,0xff,0x0c,0xc0,0x01,0x3c,0x0f,0x0f,0x04,0x03,0xf4,0xd7,0x18};
  m = MDMsg::unpack( (void *) rwf2, 0, sizeof( rwf2 ),
                     (uint8_t) RWF_FIELD_LIST_TYPE_ID, dict.dict, mem );
  printf( "rwf field list test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();
  m = MDMsg::unpack( (void *) &rwf2[ 8 ], 0, sizeof( rwf2 ) - 8,
                     (uint8_t) RWF_FIELD_LIST_TYPE_ID, dict.dict, mem );
  printf( "rwf2 field list test:\n" );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  return 0;
}

