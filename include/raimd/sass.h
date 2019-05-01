#ifndef __rai_raimd__sass_h__
#define __rai_raimd__sass_h__

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

static void
short_string( unsigned short x,  char *buf )
{
  for ( unsigned short i = 10000; i >= 10; i /= 10 ) {
    if ( i < x )
      *buf++ = ( ( x / i ) % 10 ) + '0';
  }
  *buf++ = ( x % 10 ) + '0';
  *buf = '\0';
}

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

static const char *
sass_msg_type_string( unsigned short msg_type,  char *buf )
{
  if ( msg_type < MAX_MSG_TYPE ) {
    static const char *msg_type_string[] = { SASS_DEF_MSG_TYPE( SASS_DEF_ENUM_STRING ) };
    return msg_type_string[ msg_type ];
  }
  short_string( msg_type, buf );
  return buf;
}

#define SASS_DEF_ENUM_STATUS( ENUM ) \
  ENUM ## _STATUS ,

SASS_DEF_REC_STATUS( SASS_DEF_STRING )

enum SassRecStatus {
  SASS_DEF_REC_STATUS( SASS_DEF_ENUM_STATUS )
  MAX_REC_STATUS
};

static const char *
sass_rec_status_string( unsigned short rec_status,  char *buf )
{
  if ( rec_status < MAX_REC_STATUS ) {
    static const char *rec_status_string[] = { SASS_DEF_REC_STATUS( SASS_DEF_ENUM_STRING ) };
    return rec_status_string[ rec_status ];
  }
  short_string( rec_status, buf );
  return buf;
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
#endif
