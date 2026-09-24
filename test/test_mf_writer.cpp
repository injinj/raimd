/* MktfdMsgWriter round trip: build the Marketfeed image / update the ADS
 * sends an SSL sink for INTC.O (from a capture), unpack it with MktfdMsg,
 * then convert an RWF field list into Marketfeed through append_iter().
 *
 *   test_mf_writer [dict_path]      (default $cfile_path or ../rmds-config)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raimd/md_msg.h>
#include <raimd/md_dict.h>
#include <raimd/dict_load.h>
#include <raimd/omm_flags.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/rwf_writer.h>

using namespace rai;
using namespace md;

static void
print_mf( const char *what,  const uint8_t *buf,  size_t len )
{
  printf( "%s (%u bytes):\n  ", what, (uint32_t) len );
  for ( size_t i = 0; i < len; i++ ) {
    uint8_t c = buf[ i ];
    switch ( c ) {
      case 0x1c: printf( "<FS>" ); break;
      case 0x1d: printf( "<GS>" ); break;
      case 0x1e: printf( "<RS>" ); break;
      case 0x1f: printf( "<US>" ); break;
      default:   printf( "%c", c >= ' ' && c < 0x7f ? c : '.' ); break;
    }
  }
  printf( "\n" );
}

int
main( int argc,  char **argv )
{
  const char * path = ( argc > 1 ? argv[ 1 ] : getenv( "cfile_path" ) );
  if ( path == NULL )
    path = "../rmds-config";
  MDMsgDict dicts;
  if ( ! dicts.load( path, false ) || dicts.rdm_dict == NULL ) {
    fprintf( stderr, "no RDM dictionary at %s\n", path );
    return 1;
  }
  MDDict * dict = dicts.dict;      /* list: cfile + app_a (+ flist) */
  MDDict * rdm  = dicts.rdm_dict;  /* the RWF side wants app_a alone */
  MDOutput mout;
  MDMsgMem mem;
  uint8_t  buf[ 4 * 1024 ];

  /* --- the ADS image for INTC.O, by fid ---------------------------- */
  MktfdMsgWriter w( mem, dict, buf, sizeof( buf ) );
  MDDecimal px, chg, vol;
  MDTime    tm;
  MDDate    dt;
  px.set( 505, MD_DEC_FRAC_8 );      /* 63 1/8 -> "+63 1/8" */
  chg.set( -20, MD_DEC_LOGn10_2 );   /* -0.20 */
  vol.set( 2740000, MD_DEC_INTEGER );/* +2740000 */
  tm.set( 17, 48, 0, 0, MD_RES_MINUTES );
  dt.set( 1994, 4, 26 );

  w.start( Rec_340, "INTC.O", 6 )
   .add_flist( 12 )
   .add_rtl( 1 );
  w.append_uint( 3857, (uint16_t) 8 )        /* MSG_TYPE */
   .append_uint( 3858, (uint16_t) 13886 )    /* REC_TYPE */
   .append_uint( 3859, (uint32_t) 241021378 )/* SEQ_NO */
   .append_string( 1008, "OK", 2 )           /* REC_STATUS */
   .append_uint( 2, (uint16_t) 66 )          /* RDNDISPLAY */
   .append_string( 3, "INTEL CORP", 10 )     /* DSPLY_NAME padded to 16 */
   .append_enum( 4, (uint16_t) 39 )          /* RDN_EXCHID */
   .append_time( 5, tm )                     /* TIMACT 17:48 */
   .append_decimal( 6, px )                  /* TRDPRC_1 +63 1/8 */
   .append_decimal( 11, chg )                /* NETCHNG_1 -0.20 */
   .append_enum( 15, (uint16_t) 840 )        /* CURRENCY */
   .append_date( 16, dt )                    /* TRADE_DATE 26 APR 1994 */
   .append_decimal( 32, vol );               /* ACVOL_1 +2740000 */
  size_t len = w.update_hdr();
  if ( w.err != 0 ) {
    fprintf( stderr, "writer error %d\n", w.err );
    return 2;
  }
  print_mf( "image", buf, len );

  MDMsg * m = MDMsg::unpack( buf, 0, len, MARKETFEED_TYPE_ID, dict, mem );
  if ( m == NULL ) {
    fprintf( stderr, "unpack failed\n" );
    return 3;
  }
  m->print( &mout );

  /* --- an update, empty rtl as the ADS sends them, by field name ---- */
  MDMsgMem mem2;
  uint8_t  buf2[ 1024 ];
  MktfdMsgWriter u( mem2, dict, buf2, sizeof( buf2 ) );
  u.start( Upd_316, "INTC.O", 6 ).add_rtl();
  u.append_uint( "MSG_TYPE", 8, (uint16_t) 1 )
   .append_decimal( 6, px )
   .append_uint( "ACVOL_1", 7, (uint32_t) 2740100 )
   .append_string( 15, "USD", 3 )          /* enum by display -> 840 */
   .append_string( "RDN_EXCHID", 10, "NMS", 3 ); /* -> 39 */
  len = u.update_hdr();
  print_mf( "update", buf2, len );
  m = MDMsg::unpack( buf2, 0, len, MARKETFEED_TYPE_ID, dict, mem2 );
  if ( m == NULL )
    return 4;
  m->print( &mout );

  /* --- RWF field list -> Marketfeed through the field iterator ------ */
  MDMsgMem mem3;
  uint8_t  rwf[ 1024 ];
  RwfMsgWriter rw( mem3, rdm, rwf, sizeof( rwf ), UPDATE_MSG_CLASS,
                   MARKET_PRICE_DOMAIN, 5 );
  rw.add_seq_num( 2 )
    .add_msg_key()
      .service_id( 1 )
      .name( "INTC.O" )
      .name_type( NAME_TYPE_RIC )
    .end_msg_key();
  rw.add_update( UPD_TYPE_QUOTE )
    .add_field_list()
      .append_decimal( 6, px )
      .append_decimal( 11, chg )
      .append_time( 5, tm )
      .append_uint( 32, (uint32_t) 2740200 )
      .append_string( 3, "INTEL CORP", 10 )
    .end_msg();
  size_t rwf_len = rw.off;
  if ( rw.err != 0 ) {
    fprintf( stderr, "rwf writer error %d\n", rw.err );
    return 5;
  }
  /* the msg header is written as a prefix: the message starts at rw.buf */
  RwfMsg * rm = RwfMsg::unpack_message( rw.buf, 0, rwf_len, RWF_MSG_TYPE_ID,
                                        rdm, mem3 );
  if ( rm == NULL ) {
    fprintf( stderr, "rwf unpack failed\n" );
    return 5;
  }
  uint8_t  buf3[ 1024 ];
  MktfdMsgWriter c( mem3, dict, buf3, sizeof( buf3 ) );
  c.start( Upd_316, "INTC.O", 6 ).add_rtl( 2 );
  /* the message iterator yields the header; the payload is the container */
  RwfMsg * fl = rm->get_container_msg();
  if ( fl == NULL || c.convert_msg( *fl, false ) != 0 ) {
    fprintf( stderr, "convert_msg failed\n" );
    return 6;
  }
  len = c.update_hdr();
  if ( c.err != 0 ) {
    fprintf( stderr, "convert error %d\n", c.err );
    return 6;
  }
  print_mf( "rwf -> mf", buf3, len );
  m = MDMsg::unpack( buf3, 0, len, MARKETFEED_TYPE_ID, dict, mem3 );
  if ( m == NULL )
    return 7;
  m->print( &mout );
  return 0;
}
