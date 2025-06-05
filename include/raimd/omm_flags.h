#ifndef __rai_raimd__omm_flags_h__
#define __rai_raimd__omm_flags_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MSG_KEY_CLASS     = 0, /* fake class */
  REQUEST_MSG_CLASS = 1,
  REFRESH_MSG_CLASS = 2,
  STATUS_MSG_CLASS  = 3,
  UPDATE_MSG_CLASS  = 4,
  CLOSE_MSG_CLASS   = 5,
  ACK_MSG_CLASS     = 6,
  GENERIC_MSG_CLASS = 7,
  POST_MSG_CLASS    = 8
} RwfMsgClass;

typedef enum { /* always included in *_MSG_CLASS */
  NO_DOMAIN                      = 0,
  LOGIN_DOMAIN                   = 1,
  RSVD_DOMAIN_2                  = 2,
  RSVD_DOMAIN_3                  = 3,
  SOURCE_DOMAIN                  = 4,
  DICTIONARY_DOMAIN              = 5,
  MARKET_PRICE_DOMAIN            = 6,
  MARKET_BY_ORDER_DOMAIN         = 7,
  MARKET_BY_PRICE_DOMAIN         = 8,
  MARKET_MAKER_DOMAIN            = 9,
  SYMBOL_LIST_DOMAIN             = 10,
  SERVICE_PROVIDER_STATUS_DOMAIN = 11,
  HISTORY_DOMAIN                 = 12,
  HEADLINE_DOMAIN                = 13,
  STORY_DOMAIN                   = 14,
  REPLAYHEADLINE_DOMAIN          = 15,
  REPLAYSTORY_DOMAIN             = 16,
  TRANSACTION_DOMAIN             = 17,
  RSVD_DOMAIN_18                 = 18,
  RSVD_DOMAIN_19                 = 19,
  RSVD_DOMAIN_20                 = 20,
  RSVD_DOMAIN_21                 = 21,
  YIELD_CURVE_DOMAIN             = 22,
  RSVD_DOMAIN_23                 = 23,
  RSVD_DOMAIN_24                 = 24,
  RSVD_DOMAIN_25                 = 25,
  RSVD_DOMAIN_26                 = 26,
  CONTRIBUTION_DOMAIN            = 27,
  RSVD_DOMAIN_28                 = 28,
  PROVIDER_ADMIN_DOMAIN          = 29,
  ANALYTICS_DOMAIN               = 30,
  REFERENCE_DOMAIN               = 31,
  RSVD_DOMAIN_32                 = 32,
  NEWS_TEXT_ANALYTICS_DOMAIN     = 33,
  ECONOMIC_INDICATOR             = 34,
  POLL_DOMAIN                    = 35,
  FORECAST_DOMAIN                = 36,
  MARKET_BY_TIME_DOMAIN          = 37
} RdmDomainType;

typedef enum {
  DIR_SVC_INFO_ID       = 1,
  DIR_SVC_STATE_ID      = 2,
  DIR_SVC_GROUP_ID      = 3,
  DIR_SVC_LOAD_ID       = 4,
  DIR_SVC_DATA_ID       = 5,
  DIR_SVC_LINK_ID       = 6,
  DIR_SVC_SEQ_MCAST_ID  = 7,
} RdmDirSvcInfoId;
#define rdm_svc_filter( id ) ( 1U << ( (int) id - 1 ) )
typedef enum {
  DIR_SVC_INFO_FILTER       = rdm_svc_filter( DIR_SVC_INFO_ID ),  /* source info */
  DIR_SVC_STATE_FILTER      = rdm_svc_filter( DIR_SVC_STATE_ID ), /* svc state */
  DIR_SVC_GROUP_FILTER      = rdm_svc_filter( DIR_SVC_GROUP_ID ), /* group info */
  DIR_SVC_LOAD_FILTER       = rdm_svc_filter( DIR_SVC_LOAD_ID ),  /* load info */
  DIR_SVC_DATA_FILTER       = rdm_svc_filter( DIR_SVC_DATA_ID ),  /* data info */
  DIR_SVC_LINK_FILTER       = rdm_svc_filter( DIR_SVC_LINK_ID ),  /* link state */
  DIR_SVC_SEQ_MCAST_FILTER  = rdm_svc_filter( DIR_SVC_SEQ_MCAST_ID ), /* mcast nfo*/
  DIR_SVC_ALL_FILTERS       = 0x7f
} RdmDirSvcFilterFlags;

typedef enum { /* msg key name type values */
  NAME_TYPE_UNSPEC  = 0,
  NAME_TYPE_RIC     = 1,
  NAME_TYPE_CONTRIB = 2
} RdmNameType;

typedef enum { /* login user name type values */
  NAME_TYPE_USER_NAME  = 1,
  NAME_TYPE_EMAIL_ADDR = 2,
  NAME_TYPE_USER_TOKEN = 3,
  NAME_TYPE_COOKIE     = 4,
  NAME_TYPE_AUTHN      = 5
} RdmLoginType;

typedef enum { /* always in UPDATE_MSG_CLASS */
  UPD_TYPE_UNSPECIFIED      = 0,
  UPD_TYPE_QUOTE            = 1,
  UPD_TYPE_TRADE            = 2,
  UPD_TYPE_NEWS_ALERT       = 3,
  UPD_TYPE_VOLUME_ALERT     = 4,
  UPD_TYPE_ORDER_INDICATION = 5,
  UPD_TYPE_CLOSING_RUN      = 6,
  UPD_TYPE_CORRECTION       = 7,
  UPD_TYPE_MARKET_DIGEST    = 8,
  UPD_TYPE_QUOTES_TRADE     = 9,
  UPD_TYPE_MULTIPLE         = 10,
  UPD_TYPE_VERIFY           = 11
} RdmUpdateType;

typedef enum {
  POST_NONE        = 0,
  POST_CREATE      = 1,
  POST_DELETE      = 2,
  POST_MODIFY_PERM = 4
} RdmPostRights;

typedef enum {
  QOS_TIME_UNSPECIFIED     = 0,
  QOS_TIME_REALTIME        = 1,
  QOS_TIME_DELAYED_UNKNOWN = 2,
  QOS_TIME_DELAYED         = 3
} RdmQosTime;

typedef enum {
  QOS_RATE_UNSPECIFIED    = 0,
  QOS_RATE_TICK_BY_TICK   = 1,
  QOS_RATE_JIT_CONFLATED  = 2,
  QOS_RATE_TIME_CONFLATED = 3
} RdmQosRate;

typedef enum {
  STREAM_STATE_UNSPECIFIED    = 0,
  STREAM_STATE_OPEN           = 1,
  STREAM_STATE_NON_STREAMING  = 2,
  STREAM_STATE_CLOSED_RECOVER = 3,
  STREAM_STATE_CLOSED         = 4,
  STREAM_STATE_REDIRECTED     = 5
} RdmStreamState;

typedef enum {
  DATA_STATE_NO_CHANGE = 0,
  DATA_STATE_OK        = 1,
  DATA_STATE_SUSPECT   = 2
} RdmDataState;

typedef enum {
  STATUS_CODE_NONE                         = 0,
  STATUS_CODE_NOT_FOUND                    = 1,
  STATUS_CODE_TIMEOUT                      = 2,
  STATUS_CODE_NOT_ENTITLED                 = 3,
  STATUS_CODE_INVALID_ARGUMENT             = 4,
  STATUS_CODE_USAGE_ERROR                  = 5,
  STATUS_CODE_PREEMPTED                    = 6,
  STATUS_CODE_JIT_CONFLATION_STARTED       = 7,
  STATUS_CODE_REALTIME_RESUMED             = 8,
  STATUS_CODE_FAILOVER_STARTED             = 9,
  STATUS_CODE_FAILOVER_COMPLETED           = 10,
  STATUS_CODE_GAP_DETECTED                 = 11,
  STATUS_CODE_NO_RESOURCES                 = 12,
  STATUS_CODE_TOO_MANY_ITEMS               = 13,
  STATUS_CODE_ALREADY_OPEN                 = 14,
  STATUS_CODE_SOURCE_UNKNOWN               = 15,
  STATUS_CODE_NOT_OPEN                     = 16,
  STATUS_CODE_RSVD_17                      = 17,
  STATUS_CODE_RSVD_18                      = 18,
  STATUS_CODE_NON_UPDATING_ITEM            = 19,
  STATUS_CODE_UNSUPPORTED_VIEW_TYPE        = 20,
  STATUS_CODE_INVALID_VIEW                 = 21,
  STATUS_CODE_FULL_VIEW_PROVIDED           = 22,
  STATUS_CODE_UNABLE_TO_REQUEST_AS_BATCH   = 23,
  STATUS_CODE_RSVD_24                      = 24,
  STATUS_CODE_RSVD_25                      = 25,
  STATUS_CODE_NO_BATCH_VIEW_SUPPORT_IN_REQ = 26,
  STATUS_CODE_EXCEEDED_MAX_MOUNTS_PER_USER = 27,
  STATUS_CODE_ERROR                        = 28,
  STATUS_CODE_DACS_DOWN                    = 29,
  STATUS_CODE_USER_UNKNOWN_TO_PERM_SYS     = 30,
  STATUS_CODE_DACS_MAX_LOGINS_REACHED      = 31,
  STATUS_CODE_DACS_USER_ACCESS_DENIED      = 32,
  STATUS_CODE_RSVD_33                      = 33,
  STATUS_CODE_GAP_FILL                     = 34,
  STATUS_CODE_APP_AUTHORIZATION_FAILED     = 35
} RdmStatusCode;
#define RDM_STATUS_CODE_COUNT 36

typedef enum {
  LINK_INTERACTIVE = 1,
  LINK_BROADCAST   = 2
} RdmLinkType;

typedef enum {
  LINK_DOWN = 0,
  LINK_UP   = 1
} RdmLinkState;

typedef enum {
  LINK_OK               = 1,
  LINK_RECOVER_START    = 2,
  LINK_RECOVER_COMPLETE = 3
} RdmLinkCode;

typedef enum {
  X_ACK_FLAG              = 0,  /* Close, Post flags */
  X_CLEAR_CACHE           = 1,  /* Refresh, Status flags */
  X_CONF_INFO_IN_UPDATES  = 2,  /* Request flags */
  X_DISCARDABLE           = 3,  /* Update flags */
  X_DO_NOT_CACHE          = 4,  /* Refresh, Update flags */
  X_DO_NOT_CONFLATE       = 5,  /* Update flags */
  X_DO_NOT_RIPPLE         = 6,  /* Update flags */
  X_HAS_ACK_ID            = 7,  /* msg.ack_id */
  X_HAS_ATTRIB            = 8,  /* msg_key.attrib */
  X_HAS_BATCH             = 9,  /* supports batch */
  X_HAS_CONF_INFO         = 10, /* msg.conf_info */
  X_HAS_EXTENDED_HEADER   = 11, /* msg.extended */
  X_HAS_FILTER            = 12, /* msg_key.filter */
  X_HAS_GROUP_ID          = 13, /* msg.group_id */
  X_HAS_IDENTIFIER        = 14, /* msg_key.identifier */
  X_HAS_MSG_KEY           = 15, /* msg.msg_key */
  X_HAS_NAK_CODE          = 16, /* msg.nak_code */
  X_HAS_NAME              = 17, /* msg_key.name */
  X_HAS_NAME_TYPE         = 18, /* msg_key.name_type */
  X_HAS_PART_NUM          = 19, /* msg.part_num */
  X_HAS_PERM_DATA         = 20, /* msg.perm */
  X_HAS_POST_ID           = 21, /* msg.post_id */
  X_HAS_POST_USER_INFO    = 22, /* msg.post_user */
  X_HAS_POST_USER_RIGHTS  = 23, /* msg.post_rights */
  X_HAS_PRIORITY          = 24, /* msg.priority */
  X_HAS_QOS               = 25, /* msg.qos */
  X_HAS_REQ_MSG_KEY       = 26, /* msg.req_key */
  X_HAS_SECONDARY_SEQ_NUM = 27, /* msg.second_seq_num */
  X_HAS_SEQ_NUM           = 28, /* msg.seq_num */
  X_HAS_SERVICE_ID        = 29, /* msg_key.service_id */
  X_HAS_STATE             = 30, /* msg.state */
  X_HAS_TEXT              = 31, /* msg.text */
  X_HAS_UPDATE_TYPE       = 32, /* msg.update_type */
  X_HAS_VIEW              = 33, /* Request flags */
  X_HAS_WORST_QOS         = 34, /* msg.worstQos */
  X_MESSAGE_COMPLETE      = 35, /* Generic flags */
  X_MSG_KEY_IN_UPDATES    = 36, /* Request flags */
  X_NO_REFRESH            = 37, /* Request flags */
  X_PAUSE_FLAG            = 38, /* Request flags */
  X_POST_COMPLETE         = 39, /* Request flags */
  X_PRIVATE_STREAM        = 40, /* Ack, Request, Refresh, Status flags */
  X_QUALIFIED_STREAM      = 41, /* Ack, Request, Refresh, Status flags */
  X_REFRESH_COMPLETE      = 42, /* Refresh flags */
  X_SOLICITED             = 43, /* Refresh flags */
  X_STREAMING             = 44, /* Request flags */

  /* space holders */
  X_U4, X_U5, X_U6, X_U7, X_U8, X_U9, X_U10, X_U11, X_U12, X_U13, X_U14, X_U15
} RwfMsgSerial;

#ifdef __cplusplus
}
namespace rai {
namespace md {

#define RWF_MSG_CLASS_COUNT 9
static inline bool is_valid_msg_class( uint8_t x ) {
  return ( x >= REQUEST_MSG_CLASS && x <= POST_MSG_CLASS );
}
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rwf_msg_class_str[ RWF_MSG_CLASS_COUNT ];
#else
const char *rwf_msg_class_str[ RWF_MSG_CLASS_COUNT ] = {
  "msg_key",
  "request",
  "refresh",
  "status",
  "update",
  "close",
  "ack",
  "generic",
  "post"
};
#endif

#define RDM_DOMAIN_COUNT 38
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_domain_str[ RDM_DOMAIN_COUNT ];
#else
const char *rdm_domain_str[ RDM_DOMAIN_COUNT ] = {
/*0 */ "no",
/*1 */ "login",
/*2 */ "rsvd_2",
/*3 */ "rsvd_3",
/*4 */ "source",
/*5 */ "dictionary",
/*6 */ "market_price",
/*7 */ "market_by_order",
/*8 */ "market_by_price",
/*9 */ "market_maker",
/*10*/ "symbol_list",
/*11*/ "service_provider_status",
/*12*/ "history",
/*13*/ "headline",
/*14*/ "story",
/*15*/ "replayheadline",
/*16*/ "replaystory",
/*17*/ "transaction",
/*18*/ "rsvd_18",
/*19*/ "rsvd_19",
/*20*/ "rsvd_20",
/*21*/ "rsvd_21",
/*22*/ "yield_curve",
/*23*/ "rsvd_23",
/*24*/ "rsvd_24",
/*25*/ "rsvd_25",
/*26*/ "rsvd_26",
/*27*/ "contribution",
/*28*/ "rsvd_28",
/*29*/ "provider_admin",
/*30*/ "analytics",
/*31*/ "reference",
/*32*/ "rsvd_32",
/*33*/ "news_text_analytics",
/*34*/ "economic_indicator",
/*35*/ "poll",
/*36*/ "forecast",
/*37*/ "market_by_time"
};
#endif

#define RDM_UPDATE_TYPE_COUNT 12
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_update_type_str[ RDM_UPDATE_TYPE_COUNT ];
#else
const char *rdm_update_type_str[ RDM_UPDATE_TYPE_COUNT ] = {
  "unspecified",
  "quote",
  "trade",
  "news_alert",
  "volume_alert",
  "order_indication",
  "closing_run",
  "correction",
  "market_digest",
  "quotes_trade",
  "multiple",
  "verify"
};
#endif

#define RDM_QOS_TIME_COUNT 4
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_qos_time_str[ RDM_QOS_TIME_COUNT ];
#else
const char *rdm_qos_time_str[ RDM_QOS_TIME_COUNT ] = {
  "unspecified",
  "realtime",
  "delayed_unknown",
  "delayed"
};
#endif

#define RDM_QOS_RATE_COUNT 4
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_qos_rate_str[ RDM_QOS_RATE_COUNT ];
#else
const char *rdm_qos_rate_str[ RDM_QOS_RATE_COUNT ] = {
  "unspecified",
  "tick_by_tick",
  "jit_conflated",
  "time_conflated"
};
#endif

#define RDM_STREAM_STATE_COUNT 6
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_stream_state_str[ RDM_STREAM_STATE_COUNT ];
#else
const char *rdm_stream_state_str[ RDM_STREAM_STATE_COUNT ] = {
  "unspecified",
  "open",
  "non_streaming",
  "closed_recover",
  "closed",
  "redirected",
};
#endif

#define RDM_DATA_STATE_COUNT 3
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_data_state_str[ RDM_DATA_STATE_COUNT ];
#else
const char *rdm_data_state_str[ RDM_DATA_STATE_COUNT ] = {
  "no_change",
  "ok",
  "suspect"
};
#endif

#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rdm_status_code_str[ RDM_STATUS_CODE_COUNT ];
#else
const char *rdm_status_code_str[ RDM_STATUS_CODE_COUNT ] = {
/* 0 */ "No state code",
/* 1 */ "Not found",
/* 2 */ "Timeout",
/* 3 */ "Not entitled",
/* 4 */ "Invalid argument",
/* 5 */ "Usage error",
/* 6 */ "Preempted",
/* 7 */ "Conflation started",
/* 8 */ "Realtime resumed",
/* 9 */ "Failover started",
/* 10*/ "Failover completed",
/* 11*/ "Gap detected",
/* 12*/ "No resources",
/* 13*/ "Too many items open",
/* 14*/ "Item already open",
/* 15*/ "Unknown source",
/* 16*/ "Not open",
/* 17*/ "Rsvd 17",
/* 18*/ "Rsvd 18",
/* 19*/ "Item was requested as streaming but does not update",
/* 20*/ "View Type requested is not supported for this domain",
/* 21*/ "An invalid view was requested",
/* 22*/ "Although a view was requested, the full view is being provided",
/* 23*/ "Although a batch of items were requested, the batch was split into individual request messages",
/* 24*/ "Rsvd 24",
/* 25*/ "Rsvd 25",
/* 26*/ "Request does not support batch and view",
/* 27*/ "Login rejected, exceeded maximum number of mounts per user",
/* 28*/ "Internal error from sender",
/* 29*/ "Connection to DACS down",
/* 30*/ "User unknown to permissioning system",
/* 31*/ "Maximum logins reached.",
/* 32*/ "Access Denied",
/* 33*/ "Rsvd 33",
/* 34*/ "Content fill a recognized gap",
/* 35*/ "Authorization failed"
};
#endif

#define RWF_MSG_SERIAL_COUNT 45
#ifndef DEFINE_RWF_MSG_DECODER
extern const char *rwf_msg_serial_str[ RWF_MSG_SERIAL_COUNT ];
#else
const char *rwf_msg_serial_str[ RWF_MSG_SERIAL_COUNT ] = {
  "ack_flag",
  "clear_cache",
  "conf_info_in_updates",
  "discardable",
  "do_not_cache",
  "do_not_conflate",
  "do_not_ripple",
  "has_ack_id",
  "has_attrib",
  "has_batch",
  "has_conf_info",
  "has_extended_header",
  "has_filter",
  "has_group_id",
  "has_identifier",
  "has_msg_key",
  "has_nak_code",
  "has_name",
  "has_name_type",
  "has_part_num",
  "has_perm_data",
  "has_post_id",
  "has_post_user_info",
  "has_post_user_rights",
  "has_priority",
  "has_qos",
  "has_req_msg_key",
  "has_secondary_seq_num",
  "has_seq_num",
  "has_service_id",
  "has_state",
  "has_text",
  "has_update_type",
  "has_view",
  "has_worst_qos",
  "message_complete",
  "msg_key_in_updates",
  "no_refresh",
  "pause_flag",
  "post_complete",
  "private_stream",
  "qualified_stream",
  "refresh_complete",
  "solicited",
  "streaming"
};
#endif

struct RwfSerialMask {
  uint16_t serial, /* serialized flags */
           mask;   /* message header flags */
};

struct RwfMsgFlagMap {
  const RwfSerialMask * map;
  uint16_t              map_size;
  RwfMsgClass           msg_class;

  uint64_t serial_map( uint16_t mask ) const {
    uint64_t bits = 0;
    for ( uint16_t i = 0; i < this->map_size; i++ ) {
      if ( ( mask & this->map[ i ].mask ) != 0 )
        bits |= (uint64_t) 1 << this->map[ i ].serial;
    }
    return bits;
  }
  uint16_t to_mask( RwfMsgSerial which ) const {
    for ( uint16_t i = 0; i < this->map_size; i++ ) {
      if ( which == this->map[ i ].serial )
        return this->map[ i ].mask;
    }
    return 0;
  }
  uint16_t to_flags( uint64_t all ) const {
    uint16_t mask = 0;
    for ( uint16_t i = 0; i < this->map_size; i++ ) {
      if ( ( ( (uint64_t) 1 << this->map[ i ].serial ) & all ) != 0 )
        mask |= this->map[ i ].mask;
    }
    return mask;
  }
};

#ifndef DEFINE_RWF_MSG_DECODER
extern const RwfMsgFlagMap * rwf_flags_map[ RWF_MSG_CLASS_COUNT ];
#endif

#define E( x, y, z ) RWF_ ## x ## _ ## y = z
#define F( y, z ) { X ## _ ## y, z }

#define E3( x, y, z, x2, y2, z2, x3, y3, z3 ) \
E( x, y, z ), E( x2, y2, z2 ), E( x3, y3, z3 )

#define F3( y, z, y2, z2, y3, z3 ) \
F( y, z ), F( y2, z2 ), F( y3, z3 )
#define Z( x, n ) \
static const uint16_t x ## _FLAG_CNT = n;

#ifdef DEFINE_RWF_MSG_DECODER
/* this defines map_REQUEST[] = {} and enum */
#define M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13, y14, z14, y15, z15 ) \
enum { \
  E3( x, y, z, x, y2, z2, x, y3, z3 ), \
  E3( x, y4, z4, x, y5, z5, x, y6, z6 ), \
  E3( x, y7, z7, x, y8, z8, x, y9, z9 ), \
  E3( x, y10, z10, x, y11, z11, x, y12, z12 ), \
  E3( x, y13, z13, x, y14, z14, x, y15, z15 ) \
}; \
const RwfSerialMask map_ ## x[] = { \
  F3( y, z,  y2, z2,  y3, z3 ), \
  F3( y4, z4, y5, z5, y6, z6 ), \
  F3( y7, z7, y8, z8, y9, z9 ), \
  F3( y10, z10, y11, z11, y12, z12 ), \
  F3( y13, z13, y14, z14, y15, z15 ) \
}
#else
/* this only defines enum { RWF_REQUEST_STREAMING ... } */
#define M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13, y14, z14, y15, z15 ) \
enum { \
  E3( x, y, z, x, y2, z2, x, y3, z3 ), \
  E3( x, y4, z4, x, y5, z5, x, y6, z6 ), \
  E3( x, y7, z7, x, y8, z8, x, y9, z9 ), \
  E3( x, y10, z10, x, y11, z11, x, y12, z12 ), \
  E3( x, y13, z13, x, y14, z14, x, y15, z15 ) \
};
#endif

#define M14( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13, y14, z14 ) \
  Z( x, 14 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13, y14, z14, U15, 0x8000 )
#define M13( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13 ) \
  Z( x, 13 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, y11, z11, y12, z12, y13, z13, U14, 0x4000, U15, 0x8000 )
#define M10( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10 ) \
  Z( x, 10 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, y10, z10, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )
#define  M9( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9 ) \
  Z( x,  9 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, y9, z9, U10, 0x400, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )
#define  M8( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8 ) \
  Z( x,  8 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, y8, z8, U9, 0x200, U10, 0x400, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )
#define  M7( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7 ) \
  Z( x,  7 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, y7, z7, U8, 0x100, U9, 0x200, U10, 0x400, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )
#define  M6( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6 ) \
  Z( x,  6 ); M15( x, y, z, y2, z2, y3, z3, y4, z4, y5, z5, y6, z6, U7, 0x80, U8, 0x100, U9, 0x200, U10, 0x400, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )
#define  M3( x, y, z, y2, z2, y3, z3 ) \
  Z( x,  3 ); M15( x, y, z, y2, z2, y3, z3, U4, 0x10, U5, 0x20, U6, 0x40, U7, 0x80, U8, 0x100, U9, 0x200, U10, 0x400, U11, 0x800, U12, 0x1000, U13, 0x2000, U14, 0x4000, U15, 0x8000 )

M7 ( ACK,     HAS_EXTENDED_HEADER  , 0x01,
              HAS_TEXT             , 0x02,
              PRIVATE_STREAM       , 0x04,
              HAS_SEQ_NUM          , 0x08,
              HAS_MSG_KEY          , 0x10,
              HAS_NAK_CODE         , 0x20,
              QUALIFIED_STREAM     , 0x40 );

M3 ( CLOSE,   HAS_EXTENDED_HEADER  , 0x01,
              ACK_FLAG             , 0x02,
              HAS_BATCH            , 0x04 );

M8 ( GENERIC, HAS_EXTENDED_HEADER  , 0x01,
              HAS_PERM_DATA        , 0x02,
              HAS_MSG_KEY          , 0x04,
              HAS_SEQ_NUM          , 0x08,
              MESSAGE_COMPLETE     , 0x10,
              HAS_SECONDARY_SEQ_NUM, 0x20,
              HAS_PART_NUM         , 0x40,
              HAS_REQ_MSG_KEY      , 0x80 );

M6 ( MSGKEY,  HAS_SERVICE_ID       , 0x01,
              HAS_NAME             , 0x02,
              HAS_NAME_TYPE        , 0x04,
              HAS_FILTER           , 0x08,
              HAS_IDENTIFIER       , 0x10,
              HAS_ATTRIB           , 0x20 );

M9 ( POST,    HAS_EXTENDED_HEADER , 0x001,
              HAS_POST_ID         , 0x002,
              HAS_MSG_KEY         , 0x004,
              HAS_SEQ_NUM         , 0x008,
              POST_COMPLETE       , 0x020,
              ACK_FLAG            , 0x040,
              HAS_PERM_DATA       , 0x080,
              HAS_PART_NUM        , 0x100,
              HAS_POST_USER_RIGHTS, 0x200 );

M14( REFRESH, HAS_EXTENDED_HEADER , 0x0001,
              HAS_PERM_DATA       , 0x0002,
              HAS_MSG_KEY         , 0x0008,
              HAS_SEQ_NUM         , 0x0010,
              SOLICITED           , 0x0020,
              REFRESH_COMPLETE    , 0x0040,
              HAS_QOS             , 0x0080,
              CLEAR_CACHE         , 0x0100,
              DO_NOT_CACHE        , 0x0200,
              PRIVATE_STREAM      , 0x0400,
              HAS_POST_USER_INFO  , 0x0800,
              HAS_PART_NUM        , 0x1000,
              HAS_REQ_MSG_KEY     , 0x2000,
              QUALIFIED_STREAM    , 0x4000 );

M13( REQUEST, HAS_EXTENDED_HEADER , 0x0001,
              HAS_PRIORITY        , 0x0002,
              STREAMING           , 0x0004,
              MSG_KEY_IN_UPDATES  , 0x0008,
              CONF_INFO_IN_UPDATES, 0x0010,
              NO_REFRESH          , 0x0020,
              HAS_QOS             , 0x0040,
              HAS_WORST_QOS       , 0x0080,
              PRIVATE_STREAM      , 0x0100,
              PAUSE_FLAG          , 0x0200,
              HAS_VIEW            , 0x0400,
              HAS_BATCH           , 0x0800,
              QUALIFIED_STREAM    , 0x1000 );

M10( UPDATE,  HAS_EXTENDED_HEADER, 0x001,
              HAS_PERM_DATA      , 0x002,
              HAS_MSG_KEY        , 0x008,
              HAS_SEQ_NUM        , 0x010,
              HAS_CONF_INFO      , 0x020,
              DO_NOT_CACHE       , 0x040,
              DO_NOT_CONFLATE    , 0x080,
              DO_NOT_RIPPLE      , 0x100,
              HAS_POST_USER_INFO , 0x200,
              DISCARDABLE        , 0x400 );

M10( STATUS,  HAS_EXTENDED_HEADER, 0x001,
              HAS_PERM_DATA      , 0x002,
              HAS_MSG_KEY        , 0x008,
              HAS_GROUP_ID       , 0x010,
              HAS_STATE          , 0x020,
              CLEAR_CACHE        , 0x040,
              PRIVATE_STREAM     , 0x080,
              HAS_POST_USER_INFO , 0x100,
              HAS_REQ_MSG_KEY    , 0x200,
              QUALIFIED_STREAM   , 0x400 );

#ifdef DEFINE_RWF_MSG_DECODER
const RwfMsgFlagMap
msg_key_class     = { map_MSGKEY , MSGKEY_FLAG_CNT , MSG_KEY_CLASS     },
request_msg_class = { map_REQUEST, REQUEST_FLAG_CNT, REQUEST_MSG_CLASS },
refresh_msg_class = { map_REFRESH, REFRESH_FLAG_CNT, REFRESH_MSG_CLASS },
status_msg_class  = { map_STATUS , STATUS_FLAG_CNT , STATUS_MSG_CLASS  },
update_msg_class  = { map_UPDATE , UPDATE_FLAG_CNT , UPDATE_MSG_CLASS  },
close_msg_class   = { map_CLOSE  , CLOSE_FLAG_CNT  , CLOSE_MSG_CLASS   },
ack_msg_class     = { map_ACK    , ACK_FLAG_CNT    , ACK_MSG_CLASS     },
generic_msg_class = { map_GENERIC, GENERIC_FLAG_CNT, GENERIC_MSG_CLASS },
post_msg_class    = { map_POST   , POST_FLAG_CNT   , POST_MSG_CLASS    };

const RwfMsgFlagMap * rwf_flags_map[ RWF_MSG_CLASS_COUNT ] = {
  &msg_key_class,
  &request_msg_class,
  &refresh_msg_class,
  &status_msg_class,
  &update_msg_class,
  &close_msg_class,
  &ack_msg_class,
  &generic_msg_class,
  &post_msg_class
};
#endif

#undef E
#undef F
#undef Z
#undef E3
#undef F3
#undef M15
#undef M14
#undef M13
#undef M10
#undef M9
#undef M8
#undef M7
#undef M3

}
}
#endif
#endif
