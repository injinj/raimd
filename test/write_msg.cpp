#include <stdio.h>
#include <stdlib.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/enum_def.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/rwf_msg.h>

using namespace rai;
using namespace md;

template< class Writer >
size_t
test_write( Writer &writer )
{
  static char msg_type[]   = "MSG_TYPE",
              rec_type[]   = "REC_TYPE",
              seq_no[]     = "SEQ_NO",
              rec_status[] = "REC_STATUS",
              symbol[]     = "SYMBOL",
              bid_size[]   = "BIDSIZE",
              ask_size[]   = "ASKSIZE",
              bid[]        = "BID",
              ask[]        = "ASK",
              timact[]     = "TIMACT",
              trade_date[] = "TRADE_DATE",
              row64_1[]    = "ROW64_1";
  MDDecimal dec;
  MDTime time;
  MDDate date;

  writer.append_int( msg_type, sizeof( msg_type ),  (short) 1 );
  writer.append_int( rec_type, sizeof( rec_type ),  (short) 0 );
  writer.append_int( seq_no, sizeof( seq_no ),  (short) 100 );
  writer.append_int( rec_status, sizeof( rec_status ), (short) 0 );
  writer.append_string( symbol, sizeof( symbol ), "INTC.O", 7 );
  writer.append_real( bid_size, sizeof( bid_size ), 10.0 );
  writer.append_real( ask_size, sizeof( ask_size ), 20.0 );

  dec.ival = 17500;
  dec.hint = MD_DEC_LOGn10_3; /* / 1000 */
  writer.append_decimal( bid, sizeof( bid ), dec );

  dec.ival = 17750;
  dec.hint = MD_DEC_LOGn10_3; /* / 1000 */
  writer.append_decimal( ask, sizeof( ask ), dec );

  time.hour       = 13;
  time.min        = 15;
  time.sec        = 0;
  time.resolution = MD_RES_MINUTES;
  time.fraction   = 0;
  writer.append_time( timact, sizeof( timact ), time );

  date.year = 2019;
  date.mon  = 4;
  date.day  = 9;
  writer.append_date( trade_date, sizeof( trade_date ), date );

  static char head[] = "WORLD HEADLINE";
  MDReference mref;
  mref.zero();
  mref.fptr     = (uint8_t *) head;
  mref.ftype    = MD_PARTIAL;
  mref.fsize    = sizeof( head ) - 1;
  mref.fentrysz = 8;
  writer.append_ref( row64_1, sizeof( row64_1 ), mref );

  return writer.update_hdr();
}

static MDDict *
load_dict_files( const char *path )
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  int x, y;
  if ( (x = CFile::parse_path( dict_build, path, "tss_fields.cf" )) == 0 ) {
    CFile::parse_path( dict_build, path, "tss_records.cf" );
    dict_build.index_dict( "cfile", dict ); /* dict contains index */
  }
  dict_build.clear_build(); /* frees temp memory used to index dict */
  if ( (y = AppA::parse_path( dict_build, path, "RDMFieldDictionary" )) == 0){
    EnumDef::parse_path( dict_build, path, "enumtype.def" );
    dict_build.index_dict( "app_a", dict ); /* dict is a list */
  }
  dict_build.clear_build();
  if ( dict != NULL ) { /* print which dictionaries loaded */
    printf( "%s dict loaded (size: %u)\n", dict->dict_type,
            dict->dict_size );
    if ( dict->next != NULL )
      printf( "%s dict loaded (size: %u)\n", dict->next->dict_type,
              dict->next->dict_size );
    return dict;
  }
  printf( "cfile status %d+%s, RDM status %d+%s\n",
          x, Err::err( x )->descr, y, Err::err( y )->descr );
  return NULL;
}

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test constructing and unpacking md msgs\n"
      "Always do rv, tib formats\n"
      "Only do tib_sass, rwf when cfile & RDM dicts are in $cfile_path\n" );
    return 1;
  }

  MDOutput mout;
  MDMsgMem mem;
  MDDict * dict;
  MDMsg  * m;
  char buf[ 1024 ];
  size_t sz;

  dict = load_dict_files( ::getenv( "cfile_path" ) );

  RvMsg::init_auto_unpack();
  TibMsg::init_auto_unpack();
  TibSassMsg::init_auto_unpack();
  RwfMsg::init_auto_unpack();

  TibMsgWriter tibmsg( buf, sizeof( buf ) );
  sz = test_write<TibMsgWriter>( tibmsg );
  printf( "TibMsg test:\n" );
  mout.print_hex( buf, sz );
  printf( "tibmsg sz %lu\n", sz );

  m = MDMsg::unpack( buf, 0, sz, 0, dict, &mem );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  RvMsgWriter rvmsg( buf, sizeof( buf ) );
  sz = test_write<RvMsgWriter>( rvmsg );
  printf( "RvMsg test:\n" );
  mout.print_hex( buf, sz );
  printf( "rvmsg sz %lu\n", sz );

  m = MDMsg::unpack( buf, 0, sz, 0, dict, &mem );
  if ( m != NULL )
    m->print( &mout );
  mem.reuse();

  if ( dict != NULL ) {
    TibSassMsgWriter tibsassmsg( dict, buf, sizeof( buf ) );
    sz = test_write<TibSassMsgWriter>( tibsassmsg );
    printf( "TibSassMsg test:\n" );
    mout.print_hex( buf, sz );
    printf( "tibsassmsg sz %lu\n", sz );

    m = MDMsg::unpack( buf, 0, sz, 0, dict, &mem );
    if ( m != NULL )
      m->print( &mout );
    mem.reuse();

    RwfMsgWriter rwfmsg( dict, buf, sizeof( buf ) );
    sz = test_write<RwfMsgWriter>( rwfmsg );
    printf( "RwfMsg test:\n" );
    mout.print_hex( buf, sz );
    printf( "rwfmsg sz %lu\n", sz );

    m = MDMsg::unpack( buf, 0, sz, 0, dict, &mem );
    if ( m != NULL )
      m->print( &mout );
    mem.reuse();
  }

  return 0;
}

