#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#if defined( _MSC_VER ) || defined( __MINGW32__ )
#define NOMINMAX
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <raimd/md_stream.h>

using namespace rai;
using namespace md;

void
xprint( void *buf,  size_t asz ) noexcept
{
  MDMsgMem mem;
  MDMsg  * m;
  MDOutput mout;

  m = MDMsg::unpack( buf, 0, asz, 0, NULL, mem );
  if ( m != NULL ) {
    m->print( &mout );
  }
  else {
    printf( "unpack failed\n" );
  }
}

#if defined( _MSC_VER ) || defined( __MINGW32__ )
static const uint64_t DELTA_EPOCH = 116444736ULL *
                                    1000000000ULL;
static void
get_current_time( time_t &sec,  uint64_t &usec ) noexcept
{
  FILETIME ft;
  GetSystemTimeAsFileTime( &ft );
  usec = ( (uint64_t) ft.dwHighDateTime << 32 ) | (uint64_t) ft.dwLowDateTime;
  usec = ( usec - DELTA_EPOCH ) / 10;
  sec  = usec / 1000000;
  usec = usec % 1000000;
}
uint64_t
ns_time( void ) noexcept
{
  FILETIME ft;
  uint64_t nsec;
  GetSystemTimeAsFileTime( &ft );
  nsec = ( (uint64_t) ft.dwHighDateTime << 32 ) | (uint64_t) ft.dwLowDateTime;
  nsec = ( nsec - DELTA_EPOCH ) * 100;
  return nsec;
}
#else
static void
get_current_time( time_t &sec,  uint64_t &usec ) noexcept
{
  struct timeval tv;
  gettimeofday( &tv, NULL );
  sec  = tv.tv_sec;
  usec = tv.tv_usec;
}
uint64_t
ns_time( void ) noexcept
{
  struct timespec ts;
  clock_gettime( CLOCK_REALTIME, &ts );
  return (uint64_t) ts.tv_sec * 1000000000 + (uint64_t) ts.tv_nsec;
}
#endif

void
new_id( StreamId &id,  MDMsgMem &tmp ) noexcept
{
  time_t   sec;
  uint64_t usec;
  get_current_time( sec, usec );

  uint64_t x  = id.x,
           y  = id.y,
           x2 = sec * 1000 + usec / 1000,
           y2 = ( x == x2 ? y + 1 : 0 );

  id.xy_to_id( x2, y2, tmp );
}

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test constructing and unpacking md stream\n" );
    return 1;
  }

  StreamMsg::init_auto_unpack();

  MDOutput         mout;
  MDMsgMem         tmp;
  char           * buf;
  StreamId         id;
  StreamGeom       geom;
  StreamArgs       sa;
  ListVal          lv;
  StreamGroupQuery grp;
  char             consumer[ 2 ];

  geom.add( NULL, 200, 4, 100, 1, 200, 4 );
  size_t asz = geom.asize();

  printf( "alloc size: %" PRIu64 "\n", asz );
  buf = (char *) ::malloc( asz );
  ::memset( buf, 0, asz );
  geom.print();
  if ( geom.stream.asize + geom.group.asize + geom.pending.asize > asz ) {
    fprintf( stderr, "too big\n" );
    return 1;
  }
  StreamData strm(
           buf, geom.stream.asize,
           &buf[ geom.stream.asize ], geom.group.asize,
           &buf[ geom.stream.asize + geom.group.asize ], geom.pending.asize );

  strm.init( geom.stream.count, geom.stream.data_len,
             geom.group.count, geom.group.data_len,
             geom.pending.count, geom.pending.data_len );
  id.x = id.y = 0;

  new_id( id, tmp );
  ListData *xl = id.construct_entry( 2, 6, tmp );
  xl->rpush( "one", 3 );
  xl->rpush( "two", 3 );
  strm.add( *xl );
  tmp.reuse();

  new_id( id, tmp );
  xl = id.construct_entry( 2, 9, tmp );
  xl->rpush( "three", 5 );
  xl->rpush( "four", 4 );
  strm.add( *xl );
  tmp.reuse();

  new_id( id, tmp );
  xl = id.construct_entry( 4, 17, tmp );
  xl->rpush( "five", 4 );
  xl->rpush( "six", 3 );
  xl->rpush( "seven", 5 );
  xl->rpush( "eight", 5 );
  strm.add( *xl );
  tmp.reuse();
  
  new_id( id, tmp );
  xl = id.construct_entry( 2, 7, tmp );
  xl->rpush( "nine", 4 );
  xl->rpush( "ten", 3 );
  strm.add( *xl );
  tmp.reuse();
  
  for ( size_t i = 0; i < 4; i++ ) {
    ListData ld;
    if ( strm.sindex( strm.stream, i, ld, tmp ) == STRM_OK ) {
      printf( "%" PRId64 ". ", i );
      for ( size_t j = 0; ld.lindex( j, lv ) == LIST_OK; j++ ) {
        printf( " %.*s%.*s", (int) lv.sz, (char *) lv.data,
                             (int) lv.sz2, (char *) lv.data2 );
      }
      printf( "\n" );
    }
    tmp.reuse();
  }
  printf( "stream cnt %" PRId64 ", data_len %" PRId64 "\n", strm.stream.count(),
          strm.stream.data_len() );

  sa.zero();
  sa.gname = "g";
  sa.glen  = 1;
  sa.idval = "0";
  sa.idlen = 1;
  strm.update_group( sa, tmp );
  tmp.reuse();

  printf( "group cnt %" PRId64 ", data_len %" PRId64 "\n", strm.group.count(),
          strm.group.data_len() );

  consumer[ 0 ] = 'c';
  consumer[ 1 ] = '1';
  sa.cname = consumer;
  sa.clen  = 2;
  for (;;) {
    sa.idval = ">";
    sa.idlen = 1;
    if ( strm.group_query( sa, 1, grp, tmp ) != STRM_OK || grp.count == 0 )
      break;
    ListData ld;
    if ( strm.sindex( strm.stream, grp.first, ld, tmp ) == STRM_OK ) {
      printf( "%.2s: ", consumer );
      for ( size_t n = 0; ld.lindex( n, lv ) == LIST_OK; n++ ) {
        printf( " %.*s%.*s",
                 (int) lv.sz, (char *) lv.data,
                 (int) lv.sz2, (char *) lv.data2 );
      }
      printf( "\n" );
    }
    if ( ld.lindex( 0, lv ) == LIST_OK ) {
      /* add consumer to pending list */
      lv.unite( tmp );
      sa.idval = (const char *) lv.data;
      sa.idlen = lv.sz;
      sa.ns    = ns_time();
      xl = sa.construct_pending( tmp );
      strm.pending.rpush( xl->listp, xl->size );
      /* update group next available */
      xl->lindex( 0, lv );
      sa.idlen = lv.dup( tmp, &sa.idval );
      strm.sindex( strm.group, grp.pos, ld, tmp );
      if ( ld.lset( 1, sa.idval, sa.idlen ) != LIST_OK )
        strm.update_group( sa, tmp );
    }
    sa.reuse( tmp );
    consumer[ 1 ]++;
  }
  mout.print_hex( buf, asz );

  xprint( buf, asz );

  return 0;
}

