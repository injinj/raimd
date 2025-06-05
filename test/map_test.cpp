#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/dict_load.h>
#include <raimd/md_replay.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/sass.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_hash_tab.h>

using namespace rai;
using namespace md;

struct MsgDict {
  MDDict * dict,
         * cfile_dict,
         * rdm_dict,
         * flist_dict;
  MsgDict() : dict( 0 ), cfile_dict( 0 ), rdm_dict( 0 ), flist_dict( 0 ) {}
};

static const char *
get_arg( int argc, char *argv[], int b, const char *f,
         const char *def ) noexcept
{
  for ( int i = 1; i < argc - b; i++ )
    if ( ::strcmp( f, argv[ i ] ) == 0 ) /* -p port */
      return argv[ i + b ];
  return def; /* default value */
}

static const char fmp_symbol[]     = "s",
                  fmp_timestamp[]  = "t",
                  fmp_ask[]        = "ap",
                  fmp_ask_size[]   = "as",
                  fmp_bid[]        = "bp",
                  fmp_bid_size[]   = "bs",
                  fmp_last[]       = "lp",
                  fmp_last_size[]  = "ls";
struct fmp_msg {
  char      symbol[ 16 ];
  MDDecimal timestamp,
            ask,
            ask_size,
            bid,
            bid_size,
            last,
            last_size;
  void zero() {
    ::memset( (void *) this, 0, sizeof( *this ) );
  }
  size_t iter_map( MDIterMap *mp ) {
    size_t n = 0;
    mp[ n++ ].string ( fmp_symbol   , symbol   , sizeof( symbol ) );
    mp[ n++ ].decimal( fmp_timestamp, timestamp );
    mp[ n++ ].decimal( fmp_ask      , ask       );
    mp[ n++ ].decimal( fmp_ask_size , ask_size  );
    mp[ n++ ].decimal( fmp_bid      , bid       );
    mp[ n++ ].decimal( fmp_bid_size , bid_size  );
    mp[ n++ ].decimal( fmp_last     , last      );
    mp[ n++ ].decimal( fmp_last_size, last_size );
    return n;
  }
};


int
main( int argc, char *argv[] )
{
  MsgDict dict; /* dictinonaries, cfile and RDM/appendix_a */
  const char * fn     = get_arg( argc, argv, 1, "-f", NULL ),
             * path   = get_arg( argc, argv, 1, "-p", ::getenv( "cfile_path" ));

  if ( get_arg( argc, argv, 0, "-h", NULL ) != NULL ) {
    fprintf( stderr,
      "Usage: %s [-f file] [-p cfile_path] [-q]\n"
      "Test reading messages from files on command line\n"
      "  -f file           = replay file to read\n"
      "  -o file           = output file to write\n"
      "  -p cfile_path     = load dictionary files from path\n"
      "A file is a binary dump of messages, each with these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n", argv[ 0 ] );
    return 1;
  }
  if ( path != NULL )
    dict.dict = load_dict_files( path );
  if ( dict.dict != NULL ) {
    for ( MDDict *d = dict.dict; d != NULL; d = d->get_next() ) {
      if ( d->dict_type[ 0 ] == 'c' )
        dict.cfile_dict = d;
      else if ( d->dict_type[ 0 ] == 'a' )
        dict.rdm_dict = d;
      else if ( d->dict_type[ 0 ] == 'f' )
        dict.flist_dict = d;
    }
  }
  FILE *filep = ( fn == NULL ? stdin : fopen( fn, "rb" ) );
  if ( filep == NULL ) {
    perror( fn );
    return 1;
  }
  MDReplay replay( filep );
  MDOutput mout;
  size_t count = 0;
  MDIterMap map[ 10 ];
  fmp_msg fmsg;
  size_t nflds = fmsg.iter_map( map );
  char ts[ 24 ], ask[ 16 ], ask_size[ 16 ],
                 bid[ 16 ], bid_size[ 16 ],
                 last[ 16 ], last_size[ 16 ];
  for ( bool b = replay.first(); b; b = replay.next() ) {
    MDMsgMem mem;
    MDMsg * m =
      MDMsg::unpack( replay.msgbuf, 0, replay.msglen, 0, dict.dict, mem );

    if ( m != NULL ) {
      MDSubjectKey k( replay.subj, replay.subjlen );
      printf( "\n## %.*s (fmt %s)\n",
              (int) replay.subjlen, replay.subj, m->get_proto_string() );
      m->print( &mout );
      count++;

      fmsg.zero();
      MDIterMap::get_map( *m, map, nflds );

      fmsg.timestamp.get_string( ts, sizeof( ts ) );
      fmsg.ask      .get_string( ask, sizeof( ask ) );
      fmsg.ask_size .get_string( ask_size, sizeof( ask_size ) );
      fmsg.bid      .get_string( bid, sizeof( bid ) );
      fmsg.bid_size .get_string( bid_size, sizeof( bid_size ) );
      fmsg.last     .get_string( last, sizeof( last ) );
      fmsg.last_size.get_string( last_size, sizeof( last_size ) );
      printf( "sym=%s "
               "ts=%s "
               "ask=%s ask_size=%s "
               "bid=%s bid_size=%s "
               "last=%s last_size=%s\n", fmsg.symbol,
               ts, ask, ask_size, bid, bid_size, last, last_size );
    }
  }
  printf( "%ld messages\n", (long) count );

  return 0;
}
