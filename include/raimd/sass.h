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
  DEF( OK ) \
  DEF( BAD_NAME ) \
  DEF( BAD_LINE ) \
  DEF( CACHE_FULL ) \
  DEF( PERMISSION_DENIED ) \
  DEF( PREEMPTED ) \
  DEF( BAD_ACCESS ) \
  DEF( TEMP_UNAVAIL ) \
  DEF( REASSIGN ) \
  DEF( NOSUBSCRIBERS ) \
  DEF( EXPIRED ) \
  DEF( TIC_DOWN ) \
  DEF( FEED_DOWN ) \
  DEF( NOT_USED_13 ) \
  DEF( GSM_DOWN ) \
  DEF( SUBSC_DENIED ) \
  DEF( SUBSC_TEMP_DENIED ) \
  DEF( NOT_FOUND ) \
  DEF( STALE_VALUE ) \
  DEF( RELOCATE ) \
  DEF( ENTITLEMENT_DENIED ) \
  DEF( REC_OVERFLOW ) \
  DEF( TIC_TUPLE_FAIL ) \
  DEF( ENTITLEMENT_MIGRATED ) \
  DEF( CI_DISCONNECTED ) \
  DEF( CI_DIAG_START ) \
  DEF( NO_CACHED_DATA ) \
  DEF( NO_REPLY ) \
  DEF( TMF_DOWN ) \
  DEF( TPT_DISCONNECTED ) \
  DEF( TIMEOUT )

#if 0
  DEF( PERIODIC_SNAPSHOT ) \
  DEF( FEED_UP ) \
  DEF( HL_ROUTER_DOWN ) \
  DEF( DQA_SUSPECT ) \
  DEF( DQA_ACTIVE ) \
  DEF( GSM_UP ) \
  DEF( HL_ROUTER_UP ) \
  DEF( TIC_UP ) \
  DEF( FEED_SWITCHOVER ) \
  DEF( DATA_SUSPECT ) \
  DEF( RECAP ) \
  DEF( CI_RECONNECTED ) \
  DEF( CI_DIAG_END ) \
  DEF( RECOVER_SUBSC_DENIED ) \
  DEF( CONTRIB_ACK ) \
  DEF( CONTRIB_NACK ) \
  DEF( TMF_UP ) \
  DEF( TPT_CONNECTED ) \
  DEF( FEED_NOT_ACCEPTING )
#endif

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

#define SASS_DEF_ENUM_STATUS( ENUM ) \
  ENUM ## _STATUS ,

SASS_DEF_REC_STATUS( SASS_DEF_STRING )

enum SassRecStatus {
  SASS_DEF_REC_STATUS( SASS_DEF_ENUM_STATUS )
  MAX_REC_STATUS
};

extern const char * sass_rec_status_string( unsigned short rec_status,
                                            char *buf ) noexcept;

extern const char   SASS_MSG_TYPE[],
                    SASS_REC_TYPE[],
                    SASS_SEQ_NO[],
                    SASS_REC_STATUS[];
static const size_t SASS_MSG_TYPE_LEN   = sizeof( "MSG_TYPE" ),
                    SASS_REC_TYPE_LEN   = sizeof( "REC_TYPE" ),
                    SASS_SEQ_NO_LEN     = sizeof( "SEQ_NO" ),
                    SASS_REC_STATUS_LEN = sizeof( "REC_STATUS" );
static inline bool is_sass_hdr( const MDName &n ) {
  if ( n.fnamelen >= SASS_SEQ_NO_LEN - 1 ) {
    if ( n.fname[ 3 ] == '_' &&
         ( n.fname[ 0 ] == 'R' || n.fname[ 0 ] == 'M' || n.fname[ 0 ] == 'S' ) ) {
      return ( n.equals( SASS_MSG_TYPE, SASS_MSG_TYPE_LEN ) ||
               n.equals( SASS_REC_TYPE, SASS_REC_TYPE_LEN ) ||
               n.equals( SASS_SEQ_NO, SASS_SEQ_NO_LEN ) ||
               n.equals( SASS_REC_STATUS, SASS_REC_STATUS_LEN ) );
    }
  }
  return false;
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
