#ifndef __rai_raimd__sass_h__
#define __rai_raimd__sass_h__

namespace rai {
namespace md {
#define SASS_DEF_MSG_TYPE( DEF ) \
  DEF( VERIFY ) \
  DEF( UPDATE ) \
  DEF( CORRECT ) \
  DEF( CLOSING ) \
  DEF( DROP ) \
  DEF( AGGREGATE ) \
  DEF( STATUS ) \
  DEF( CANCEL ) \
  DEF( INITIAL ) \
  DEF( TRANSIENT ) \
  DEF( DERIVED ) \
  DEF( DELETE ) \
  DEF( SUBREINIT ) \
  DEF( SNAPSHOT ) \
  DEF( CONFIRM ) \
  DEF( BDS_CONFIRM ) \
  DEF( EDIT ) \
  DEF( EDIT_FORCE ) \
  DEF( RENAME ) \
  DEF( SERVICE_STATUS ) \
  DEF( CONTRIB_REPLY ) \
  DEF( GROUP_STATUS ) \
  DEF( GROUP_MERGE ) \
  DEF( GROUP_CHANGE ) \
  DEF( INITIAL_PASS_THRU ) \
  DEF( UPDATE_PASS_THRU ) \
  DEF( INITIAL_AGGREGATE ) \
  DEF( UPDATE_AGGREGATE ) \
  DEF( FINISH_AGGREGATE )

#define SASS_DEF_REC_STATUS( DEF ) \
  DEF( 0, OK ) \
  DEF( 1, BAD_NAME ) \
  DEF( 2, BAD_LINE ) \
  DEF( 3, CACHE_FULL ) \
  DEF( 4, PERMISSION_DENIED ) \
  DEF( 5, PREEMPTED ) \
  DEF( 6, BAD_ACCESS ) \
  DEF( 7, TEMP_UNAVAIL ) \
  DEF( 8, REASSIGN ) \
  DEF( 9, NOSUBSCRIBERS ) \
  DEF( 10, EXPIRED ) \
  DEF( 11, TIC_DOWN ) \
  DEF( 12, FEED_DOWN ) \
  DEF( 13, NOT_USED_13 ) \
  DEF( 14, GSM_DOWN ) \
  DEF( 15, SUBSC_DENIED ) \
  DEF( 16, SUBSC_TEMP_DENIED ) \
  DEF( 17, NOT_FOUND ) \
  DEF( 18, STALE_VALUE ) \
  DEF( 19, RELOCATE ) \
  DEF( 20, ENTITLEMENT_DENIED ) \
  DEF( 21, REC_OVERFLOW ) \
  DEF( 22, TIC_TUPLE_FAIL ) \
  DEF( 23, ENTITLEMENT_MIGRATED ) \
  DEF( 24, CI_DISCONNECTED ) \
  DEF( 25, CI_DIAG_START ) \
  DEF( 26, NO_CACHED_DATA ) \
  DEF( 27, NO_REPLY ) \
  DEF( 28, TMF_DOWN ) \
  DEF( 29, TPT_DISCONNECTED ) \
  DEF( 30, TIMEOUT ) \
  DEF( 64, PERIODIC_SNAPSHOT ) \
  DEF( 65, FEED_UP ) \
  DEF( 66, HL_ROUTER_DOWN ) \
  DEF( 67, DQA_SUSPECT ) \
  DEF( 68, DQA_ACTIVE ) \
  DEF( 69, GSM_UP ) \
  DEF( 71, HL_ROUTER_UP ) \
  DEF( 72, TIC_UP ) \
  DEF( 73, FEED_SWITCHOVER ) \
  DEF( 74, DATA_SUSPECT ) \
  DEF( 75, RECAP ) \
  DEF( 76, CI_RECONNECTED ) \
  DEF( 77, CI_DIAG_END ) \
  DEF( 80, RECOVER_SUBSC_DENIED ) \
  DEF( 81, CONTRIB_ACK ) \
  DEF( 82, CONTRIB_NACK ) \
  DEF( 83, TMF_UP ) \
  DEF( 84, TPT_CONNECTED ) \
  DEF( 85, FEED_NOT_ACCEPTING )

#define SASS_DEF_STRING( ENUM ) \
  static const char ENUM ## _STRING[] = #ENUM ;
#define SASS_DEF_ENUM_TYPE( ENUM ) \
  ENUM ## _TYPE ,
#define SASS_DEF_ENUM_STRING( ENUM ) \
  ENUM ## _STRING ,

SASS_DEF_MSG_TYPE( SASS_DEF_STRING )

enum SassMsgType {
  SASS_DEF_MSG_TYPE( SASS_DEF_ENUM_TYPE )
  MAX_MSG_TYPE
};

extern const char * sass_msg_type_string( unsigned short msg_type,
                                          char *buf ) noexcept;

#define SASS_DEF_ENUM_STATUS( N, ENUM ) \
  ENUM ## _STATUS = N,
#define SASS_DEF_STRING_2( N, ENUM ) \
  static const char ENUM ## _STRING[] = #ENUM ;
#define SASS_STR_CHAR2( c, d ) (uint16_t) \
                                 ( ( (uint16_t) (uint8_t) c ) | \
                                 ( ( (uint16_t) (uint8_t) d ) << 8 ) )
#define SASS_DEF_ENUM_STRING_2( N, ENUM ) \
  { N, SASS_STR_CHAR2( ( ENUM ## _STRING )[ 0 ], ( ENUM ## _STRING )[ 1 ] ), ENUM ## _STRING },
SASS_DEF_REC_STATUS( SASS_DEF_STRING_2 )

enum SassRecStatus {
  SASS_DEF_REC_STATUS( SASS_DEF_ENUM_STATUS )
  MAX_REC_STATUS
};

extern const char * sass_rec_status_string( unsigned short rec_status,
                                            char *buf ) noexcept;
extern uint16_t sass_rec_status_val( const char *str,
                                     size_t str_len ) noexcept;

extern const char   SASS_MSG_TYPE[],
                    SASS_REC_TYPE[],
                    SASS_SEQ_NO[],
                    SASS_REC_STATUS[],
                    SASS_SYMBOL[];
static const size_t SASS_MSG_TYPE_LEN   = sizeof( "MSG_TYPE" ),
                    SASS_REC_TYPE_LEN   = sizeof( "REC_TYPE" ),
                    SASS_SEQ_NO_LEN     = sizeof( "SEQ_NO" ),
                    SASS_REC_STATUS_LEN = sizeof( "REC_STATUS" ),
                    SASS_SYMBOL_LEN     = sizeof( "SYMBOL" );
static inline uint32_t w4c( char a, char b, char c, char d ) {
  return ( (uint32_t) (uint8_t) d << 24 ) |
         ( (uint32_t) (uint8_t) c << 16 ) |
         ( (uint32_t) (uint8_t) b <<  8 ) |
         ( (uint32_t) (uint8_t) a       );
}
static inline bool is_sass_hdr( const MDName &n ) {
  #define B( c ) ( 1U << ( c - 'M' ) )
  static const uint32_t mask = B( 'M' ) | B( 'R' ) | B( 'S' );
  if ( n.fnamelen >= SASS_SEQ_NO_LEN - 1 &&
       ( B( n.fname[ 0 ] ) & mask ) != 0 ) {
  #undef B
    static const uint32_t MSG_TYPE   = 0x545f4753U, /*w4c( 'S', 'G', '_', 'T' ),*/
                          REC_TYPE   = 0x545f4345U, /*w4c( 'E', 'C', '_', 'T' ),*/
                          SEQ_NO     = 0x4e5f5145U, /*w4c( 'E', 'Q', '_', 'N' ),*/
                          REC_STATUS = 0x535f4345U, /*w4c( 'E', 'C', '_', 'S' ),*/
                          SYMBOL     = 0x4f424d59U; /*w4c( 'Y', 'M', 'B', 'O' );*/
    uint32_t x = w4c( n.fname[ 1 ], n.fname[ 2 ], n.fname[ 3 ], n.fname[ 4 ] );
    if ( x >= SEQ_NO && x <= MSG_TYPE ) {
      if ( x == MSG_TYPE )
        return n.equals( SASS_MSG_TYPE, SASS_MSG_TYPE_LEN );
      if ( x == REC_TYPE )
        return n.equals( SASS_REC_TYPE, SASS_REC_TYPE_LEN );
      if ( x == SEQ_NO )
        return n.equals( SASS_SEQ_NO, SASS_SEQ_NO_LEN );
      if ( x == REC_STATUS )
        return n.equals( SASS_REC_STATUS, SASS_REC_STATUS_LEN );
      if ( x == SYMBOL )
        return n.equals( SASS_SYMBOL, SASS_SYMBOL_LEN );
    }
  }
  return false;
}

template<class Writer>
void append_sass_hdr( Writer &w,  MDFormClass *form,  uint16_t msg_type,
                      uint16_t rec_type,  uint16_t seqno,  uint16_t status,
                      const char *subj,  size_t sublen )
{
  if ( msg_type != INITIAL_TYPE || form == NULL ) {
    w.append_uint( SASS_MSG_TYPE  , SASS_MSG_TYPE_LEN  , msg_type );
    if ( rec_type != 0 )
      w.append_uint( SASS_REC_TYPE, SASS_REC_TYPE_LEN  , rec_type );
    w.append_uint( SASS_SEQ_NO    , SASS_SEQ_NO_LEN    , seqno )
     .append_uint( SASS_REC_STATUS, SASS_REC_STATUS_LEN, status );
  }
  else {
    const MDFormEntry * e = form->entries;
    MDLookup by;
    if ( form->get( by.nm( SASS_MSG_TYPE, SASS_MSG_TYPE_LEN ) ) == &e[ 0 ] )
      w.append_uint( by.fname, by.fname_len, msg_type );
    if ( form->get( by.nm( SASS_REC_TYPE, SASS_REC_TYPE_LEN ) ) == &e[ 1 ] )
      w.append_uint( by.fname, by.fname_len, rec_type );
    if ( form->get( by.nm( SASS_SEQ_NO, SASS_SEQ_NO_LEN ) ) == &e[ 2 ] )
      w.append_uint( by.fname, by.fname_len, seqno );
    if ( form->get( by.nm( SASS_REC_STATUS, SASS_REC_STATUS_LEN ) ) == &e[ 3 ] )
      w.append_uint( by.fname, by.fname_len, status );
    if ( form->get( by.nm( SASS_SYMBOL, SASS_SYMBOL_LEN ) ) == &e[ 4 ] )
      w.append_string( by.fname, by.fname_len, subj, sublen );
  }
}

#if 0
#include <stdio.h>
int
main( void )
{
  char buf[ 16 ];
  for ( unsigned short i = 0; i < 32; i++ ) {
    puts( sass_msg_type_string( i, buf ) );
    puts( sass_rec_status_string( i, buf ) );
  }
  return 0;
}
#endif
}
}
#endif
