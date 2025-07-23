#include <stdio.h>
#define DEFINE_RWF_MSG_DECODER
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>
#include <raimd/sass.h>

using namespace rai;
using namespace md;

extern "C" {
MDMsg_t *
rwf_msg_unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                MDDict_t *d,  MDMsgMem_t *m )
{
  return RwfMsg::unpack_message( bb, off, end, h, (MDDict *)d, *(MDMsgMem *) m);
}

MDMsg_t *
rwf_msg_unpack_field_list( void *bb,  size_t off,  size_t end,
                           uint32_t h,  MDDict_t *d,  MDMsgMem_t *m )
{
  return RwfMsg::unpack_field_list( bb, off, end, h, (MDDict *)d,
                                    *(MDMsgMem *) m );
}

MDMsg_t *
md_msg_rwf_get_container_msg( MDMsg_t *m )
{
  return (MDMsg_t *) static_cast<RwfMsg *>( m )->get_container_msg();
}

uint32_t
md_msg_rwf_get_base_type_id( MDMsg_t *m )
{
  return static_cast<RwfMsg *>( m )->base.type_id;
}

bool
md_msg_rwf_get_flist( MDMsg_t *m, uint16_t *flist )
{
  if ( static_cast<MDMsg *>( m )->get_type_id() == RWF_FIELD_LIST_TYPE_ID ) {
    *flist = static_cast<RwfMsg *>( m )->fields.flist;
    return true;
  }
  return false;
}

bool
md_msg_rwf_get_msg_flags( MDMsg_t *m,  uint64_t *fl )
{
  if ( static_cast<MDMsg *>( m )->get_type_id() == RWF_MSG_TYPE_ID ) {
    *fl = static_cast<RwfMsg *>( m )->msg.flags;
    return true;
  }
  return false;
}

bool
md_msg_rwf_get_msg_seqnum( MDMsg_t *m, uint32_t *seqnum )
{
  if ( static_cast<MDMsg *>( m )->get_type_id() == RWF_MSG_TYPE_ID ) {
    *seqnum = static_cast<RwfMsg *>( m )->msg.seq_num;
    return true;
  }
  return false;
}
}

static const char RwfMsg_proto_string[]         = "RWF_MSG",
                  RwfFieldList_proto_string[]   = "RWF_FIELD_LIST",
                  RwfMap_proto_string[]         = "RWF_MAP",
                  RwfElementList_proto_string[] = "RWF_ELEMENT_LIST",
                  RwfFilterList_proto_string[]  = "RWF_FILTER_LIST",
                  RwfSeries_proto_string[]      = "RWF_SERIES",
                  RwfVector_proto_string[]      = "RWF_VECTOR",
                  RwfMsgKey_proto_string[]      = "RWF_MSG_KEY";
const char *
RwfMsg::get_proto_string( void ) noexcept
{
  switch ( this->base.type_id ) {
    default:
    case RWF_MSG:          return RwfMsg_proto_string;
    case RWF_FIELD_LIST:   return RwfFieldList_proto_string;
    case RWF_MAP:          return RwfMap_proto_string;
    case RWF_ELEMENT_LIST: return RwfElementList_proto_string;
    case RWF_FILTER_LIST:  return RwfFilterList_proto_string;
    case RWF_SERIES:       return RwfSeries_proto_string;
    case RWF_VECTOR:       return RwfVector_proto_string;
    case RWF_MSG_KEY:      return RwfMsgKey_proto_string;
  }
}

uint32_t
RwfMsg::get_type_id( void ) noexcept
{
  switch ( this->base.type_id ) {
    default:
    case RWF_MSG:          return RWF_MSG_TYPE_ID;
    case RWF_FIELD_LIST:   return RWF_FIELD_LIST_TYPE_ID;
    case RWF_MAP:          return RWF_MAP_TYPE_ID;
    case RWF_ELEMENT_LIST: return RWF_ELEMENT_LIST_TYPE_ID;
    case RWF_SERIES:       return RWF_SERIES_TYPE_ID;
    case RWF_VECTOR:       return RWF_VECTOR_TYPE_ID;
    case RWF_MSG_KEY:      return RWF_MSG_KEY_TYPE_ID;
  }
}

static MDMatch rwf_match[] = {
  {RwfFieldList_proto_string, 0,4,1,0, { 0x25, 0xcd, 0xab, 0xca }, { RWF_FIELD_LIST_TYPE_ID }, RwfMsg::is_rwf_field_list,   (md_msg_unpack_f) RwfMsg::unpack_field_list },
  {RwfMap_proto_string,0,0xff,1,0,         { 0 }, { RWF_MAP_TYPE_ID },          RwfMsg::is_rwf_map,          (md_msg_unpack_f) RwfMsg::unpack_map },
  {RwfElementList_proto_string,0,0xff,1,0, { 0 }, { RWF_ELEMENT_LIST_TYPE_ID }, RwfMsg::is_rwf_element_list, (md_msg_unpack_f) RwfMsg::unpack_element_list },
  {RwfFilterList_proto_string,0,0xff,1,0,  { 0 }, { RWF_FILTER_LIST_TYPE_ID },  RwfMsg::is_rwf_filter_list,  (md_msg_unpack_f) RwfMsg::unpack_filter_list },
  {RwfSeries_proto_string,0,0xff,1,0,      { 0 }, { RWF_SERIES_TYPE_ID },       RwfMsg::is_rwf_series,       (md_msg_unpack_f) RwfMsg::unpack_series },
  {RwfVector_proto_string,0,0xff,1,0,      { 0 }, { RWF_VECTOR_TYPE_ID },       RwfMsg::is_rwf_vector,       (md_msg_unpack_f) RwfMsg::unpack_vector },
  {RwfMsg_proto_string,0,0xff,1,0,         { 0 }, { RWF_MSG_TYPE_ID },          RwfMsg::is_rwf_message,      (md_msg_unpack_f) RwfMsg::unpack_message },
  {RwfMsgKey_proto_string,0,0xff,1,0,      { 0 }, { RWF_MSG_KEY_TYPE_ID },      RwfMsg::is_rwf_msg_key,      (md_msg_unpack_f) RwfMsg::unpack_msg_key }
};

void
RwfMsg::init_auto_unpack( void ) noexcept
{
  for ( size_t i = 0; i < sizeof( rwf_match ) / sizeof( rwf_match[ 0 ] ); i++ )
    MDMsg::add_match( rwf_match[ i ] );
}

template<class T>
static inline
bool is_rwf( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  T hdr;
  return hdr.parse( bb, off, end ) == 0;
}
bool
RwfMsg::is_rwf_field_list( void *bb,  size_t off,
                           size_t end,  uint32_t h ) noexcept
{
  RwfFieldListHdr hdr;
  if ( hdr.parse( bb, off, end ) == 0 )
    return true;
  if ( hdr.type_id == RWF_MAP )
    return RwfMsg::is_rwf_map( bb, off, end, h );
  if ( hdr.type_id == RWF_ELEMENT_LIST )
    return RwfMsg::is_rwf_element_list( bb, off, end, h );
  if ( hdr.type_id == RWF_FILTER_LIST )
    return RwfMsg::is_rwf_filter_list( bb, off, end, h );
  if ( hdr.type_id == RWF_SERIES )
    return RwfMsg::is_rwf_series( bb, off, end, h );
  if ( hdr.type_id == RWF_VECTOR )
    return RwfMsg::is_rwf_vector( bb, off, end, h );
  if ( hdr.type_id == RWF_MSG )
    return RwfMsg::is_rwf_message( bb, off, end, h );
  if ( hdr.type_id == RWF_MSG_KEY )
    return RwfMsg::is_rwf_msg_key( bb, off, end, h );
  return false;
}

bool RwfMsg::is_rwf_map( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept          { return is_rwf<RwfMapHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_element_list( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept { return is_rwf<RwfElementListHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_filter_list( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept  { return is_rwf<RwfFilterListHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_series( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept       { return is_rwf<RwfSeriesHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_vector( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept       { return is_rwf<RwfVectorHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_message( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept      { return is_rwf<RwfMsgHdr>( bb, off, end, h ); }
bool RwfMsg::is_rwf_msg_key( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept      { return is_rwf<RwfMsgKey>( bb, off, end, h ); }

bool
rai::md::rwf_primitive_to_md_type( uint8_t type,  MDType &ftype ) noexcept
{
  switch ( type ) {
    case RWF_INT:          ftype = MD_INT;      return true;
    case RWF_UINT:         ftype = MD_UINT;     return true;
    case RWF_FLOAT:
    case RWF_DOUBLE:       ftype = MD_REAL;     return true;
    case RWF_REAL:         ftype = MD_DECIMAL;  return true;
    case RWF_DATE:         ftype = MD_DATE;     return true;
    case RWF_TIME:         ftype = MD_TIME;     return true;
    case RWF_DATETIME:     ftype = MD_DATETIME; return true;
    case RWF_ENUM:         ftype = MD_ENUM;     return true;
    case RWF_ASCII_STRING: ftype = MD_STRING;   return true;
    case RWF_UTF8_STRING:  ftype = MD_STRING;   return true;
    case RWF_RMTES_STRING: ftype = MD_STRING;   return true;
    case RWF_ARRAY:        ftype = MD_ARRAY;    return true;
    case RWF_BUFFER:
    case RWF_QOS:
    case RWF_STATE:        ftype = MD_OPAQUE;   return true;
    default:               ftype = MD_NODATA;   return false;
  }
  return false;
}

bool
rai::md::rwf_type_size_to_md_type( uint8_t type,  MDType &ftype,  uint32_t &fsize ) noexcept
{
  switch ( type ) {
    case RWF_INT_1:       ftype = MD_INT;      fsize = 1;  return true;
    case RWF_UINT_1:      ftype = MD_UINT;     fsize = 1;  return true;
    case RWF_INT_2:       ftype = MD_INT;      fsize = 2;  return true;
    case RWF_UINT_2:      ftype = MD_UINT;     fsize = 2;  return true;
    case RWF_INT_4:       ftype = MD_INT;      fsize = 4;  return true;
    case RWF_UINT_4:      ftype = MD_UINT;     fsize = 4;  return true;
    case RWF_INT_8:       ftype = MD_INT;      fsize = 8;  return true;
    case RWF_UINT_8:      ftype = MD_UINT;     fsize = 8;  return true;
    case RWF_FLOAT_4:     ftype = MD_REAL;     fsize = 4;  return true;
    case RWF_DOUBLE_8:    ftype = MD_REAL;     fsize = 8;  return true;
    case RWF_REAL_4RB:    ftype = MD_DECIMAL;  fsize = 0;  return true;
    case RWF_REAL_8RB:    ftype = MD_DECIMAL;  fsize = 0;  return true;
    case RWF_DATE_4:      ftype = MD_DATE;     fsize = 4;  return true;
    case RWF_TIME_3:      ftype = MD_TIME;     fsize = 3;  return true;
    case RWF_TIME_5:      ftype = MD_TIME;     fsize = 5;  return true;
    case RWF_DATETIME_7:  ftype = MD_DATETIME; fsize = 7;  return true;
    case RWF_DATETIME_9:  ftype = MD_DATETIME; fsize = 9;  return true;
    case RWF_DATETIME_11: ftype = MD_DATETIME; fsize = 11; return true;
    case RWF_DATETIME_12: ftype = MD_DATETIME; fsize = 12; return true;
    case RWF_TIME_7:      ftype = MD_TIME;     fsize = 7;  return true;
    case RWF_TIME_8:      ftype = MD_TIME;     fsize = 8;  return true;
    default:
      fsize = 0;
      return rwf_primitive_to_md_type( type, ftype );
  }
  return false;
}

uint16_t
rai::md::rwf_to_sass_msg_type( RwfMsg &rwf ) noexcept
{
  if ( rwf.msg.msg_class == REFRESH_MSG_CLASS ) {
    return MD_INITIAL_TYPE;
  }
  if ( rwf.msg.msg_class == UPDATE_MSG_CLASS ) {
    switch ( rwf.msg.update_type ) {
      default:                   return MD_UPDATE_TYPE;  break;
      case UPD_TYPE_CLOSING_RUN: return MD_CLOSING_TYPE; break;
      case UPD_TYPE_CORRECTION:  return MD_CORRECT_TYPE; break;
      case UPD_TYPE_VERIFY:      return MD_VERIFY_TYPE;  break;
    }
  }
  else if ( rwf.msg.msg_class == STATUS_MSG_CLASS ) {
    return MD_TRANSIENT_TYPE;
  }
  return MD_UPDATE_TYPE;
}

uint16_t
rai::md::rwf_code_to_sass_rec_status( RwfMsg &rwf ) noexcept
{
  switch ( rwf.msg.state.code ) {
    default:                                       return MD_OK_STATUS;
    case STATUS_CODE_NOT_FOUND:                    return MD_NOT_FOUND_STATUS;
    case STATUS_CODE_TIMEOUT:                      return MD_TEMP_UNAVAIL_STATUS;
    case STATUS_CODE_NOT_ENTITLED:                 return MD_PERMISSION_DENIED_STATUS;
    case STATUS_CODE_INVALID_ARGUMENT:             return MD_BAD_NAME_STATUS;
    case STATUS_CODE_USAGE_ERROR:                  return MD_BAD_NAME_STATUS;
    case STATUS_CODE_PREEMPTED:                    return MD_PREEMPTED_STATUS;
    case STATUS_CODE_JIT_CONFLATION_STARTED:       return MD_OK_STATUS;
    case STATUS_CODE_REALTIME_RESUMED:             return MD_OK_STATUS;
    case STATUS_CODE_FAILOVER_STARTED:             return MD_REASSIGN_STATUS;
    case STATUS_CODE_FAILOVER_COMPLETED:           return MD_OK_STATUS;
    case STATUS_CODE_GAP_DETECTED:                 return MD_STALE_VALUE_STATUS;
    case STATUS_CODE_NO_RESOURCES:                 return MD_BAD_LINE_STATUS;
    case STATUS_CODE_TOO_MANY_ITEMS:               return MD_CACHE_FULL_STATUS;
    case STATUS_CODE_ALREADY_OPEN:                 return MD_OK_STATUS;
    case STATUS_CODE_SOURCE_UNKNOWN:               return MD_TEMP_UNAVAIL_STATUS;
    case STATUS_CODE_NOT_OPEN:                     return MD_BAD_LINE_STATUS;
    case STATUS_CODE_NON_UPDATING_ITEM:            return MD_OK_STATUS;
    case STATUS_CODE_UNSUPPORTED_VIEW_TYPE:        return MD_BAD_LINE_STATUS;
    case STATUS_CODE_INVALID_VIEW:                 return MD_BAD_LINE_STATUS;
    case STATUS_CODE_FULL_VIEW_PROVIDED:           return MD_OK_STATUS;
    case STATUS_CODE_UNABLE_TO_REQUEST_AS_BATCH:   return MD_BAD_LINE_STATUS;
    case STATUS_CODE_NO_BATCH_VIEW_SUPPORT_IN_REQ: return MD_BAD_LINE_STATUS;
    case STATUS_CODE_EXCEEDED_MAX_MOUNTS_PER_USER: return MD_CACHE_FULL_STATUS;
    case STATUS_CODE_ERROR:                        return MD_BAD_LINE_STATUS;
    case STATUS_CODE_DACS_DOWN:                    return MD_TEMP_UNAVAIL_STATUS;
    case STATUS_CODE_USER_UNKNOWN_TO_PERM_SYS:     return MD_PERMISSION_DENIED_STATUS;
    case STATUS_CODE_DACS_MAX_LOGINS_REACHED:      return MD_TEMP_UNAVAIL_STATUS;
    case STATUS_CODE_DACS_USER_ACCESS_DENIED:      return MD_PERMISSION_DENIED_STATUS;
    case STATUS_CODE_GAP_FILL:                     return MD_OK_STATUS;
    case STATUS_CODE_APP_AUTHORIZATION_FAILED:     return MD_PERMISSION_DENIED_STATUS;
  }
}

uint32_t
RwfBase::parse_type( RwfDecoder &dec ) noexcept
{
  uint32_t t = 0;
  this->set_size   = 0;
  this->set_start  = 0;
  this->data_start = 0;
  if ( dec.len() >= 8 ) {
    if ( dec.peek_u32( 0 ) == RWF_FIELD_LIST_TYPE_ID ) {
      t = dec.peek_u32( 1 );
      if ( is_rwf_container( (RWF_type) t ) )
        dec.incr( 8 );
      else
        t = 0;
    }
  }
  return this->type_id = t;
}

int
RwfMsgKey::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );

  this->type_id   = RWF_MSG_KEY;
  this->data      = (void *) dec.buf;
  this->data_size = dec.len();
  this->key_flags = 0;
  this->name      = NULL;
  this->name_len  = 0;
  this->attrib.container_type = RWF_NO_DATA;
  this->attrib.len  = 0;
  this->attrib.data = NULL;

  dec.u15( this->key_flags );
  this->flags = rwf_flags_map[ MSG_KEY_CLASS ]->serial_map( this->key_flags );

  if ( ( this->key_flags & HAS_SERVICE_ID ) != 0 )
    dec.z16( this->service_id );
  if ( (this->key_flags & HAS_NAME ) != 0 )
    dec.u8( this->name_len )
       .p ( this->name, this->name_len );
  if ( ( this->key_flags & HAS_NAME_TYPE ) != 0 )
    dec.u8( this->name_type );
  if ( ( this->key_flags & HAS_FILTER ) != 0 )
    dec.u32( this->filter );
  if ( ( this->key_flags & HAS_IDENTIFIER ) != 0 )
    dec.u32( this->identifier );

  if ( ( this->key_flags & HAS_ATTRIB ) != 0 ) {
    dec.u8( this->attrib.container_type );
    this->attrib.container_type += RWF_CONTAINER_BASE;
    dec.u15( this->attrib.len )
       .p  ( this->attrib.data, this->attrib.len );
  }
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfMsgHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder hdr( bb, off, end );
  uint32_t t = this->parse_type( hdr );

  if ( t != 0 && t != RWF_MSG )
    return Err::BAD_HEADER;

  this->type_id        = RWF_MSG;
  this->msg_class      = 0;
  this->domain_type    = 0;
  this->stream_id      = 0;
  this->container_type = RWF_NO_DATA;

  hdr.u16( this->header_size );

  this->data_start = hdr.offset( off + this->header_size );
  this->data_end   = end;

  hdr.u8 ( this->msg_class )
     .u8 ( this->domain_type )
     .u32( this->stream_id )
     .u15( this->msg_flags )
     .u8 ( this->container_type );

  this->msg_class      &= 0x1f;
  this->container_type += RWF_CONTAINER_BASE;

  if ( ! hdr.ok || ! is_valid_msg_class( this->msg_class ) )
    return Err::BAD_HEADER;

  RwfMsgDecode dec( *this, hdr );

  switch ( this->msg_class ) {
    case REQUEST_MSG_CLASS:
      dec.get_priority()
         .get_qos()
         .get_worst_qos()
         .get_msg_key()
         .get_extended();
      break;
    case REFRESH_MSG_CLASS:
      dec.get_seq_num()
         .get_state()
         .get_group_id()
         .get_perm()
         .get_qos()
         .get_msg_key()
         .get_extended()
         .get_post_user()
         .get_part_num()
         .get_req_msg_key();
      break;
    case STATUS_MSG_CLASS:
      dec.get_state()
         .get_group_id()
         .get_perm()
         .get_msg_key()
         .get_extended()
         .get_post_user()
         .get_req_msg_key();
      break;
    case UPDATE_MSG_CLASS:
      dec.get_update_type()
         .get_seq_num()
         .get_conflate_info()
         .get_perm()
         .get_msg_key()
         .get_extended()
         .get_post_user();
      break;
    case CLOSE_MSG_CLASS:
      dec.get_extended();
      break;
    case ACK_MSG_CLASS:
      dec.get_ack_id()
         .get_nak_code()
         .get_text()
         .get_seq_num()
         .get_msg_key()
         .get_extended();
      break;
    case GENERIC_MSG_CLASS:
      dec.get_seq_num()
         .get_second_seq_num()
         .get_perm()
         .get_msg_key()
         .get_extended()
         .get_part_num()
         .get_req_msg_key();
      break;
    case POST_MSG_CLASS:
      dec.get_post_user()
         .get_seq_num()
         .get_post_id()
         .get_perm()
         .get_msg_key()
         .get_extended()
         .get_part_num()
         .get_post_rights();
      break;
    default: break;
  }
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

bool
RwfQos::decode( void *buf,  size_t buflen ) noexcept
{
  uint8_t * start = (uint8_t *) buf,
          * end   = &start[ buflen ];
  RwfDecoder inp( start, end );
  inp.dec_qos( *this );
  return inp.ok;
}

bool
RwfState::decode( void *buf,  size_t buflen ) noexcept
{
  uint8_t * start = (uint8_t *) buf,
          * end   = &start[ buflen ];
  RwfDecoder inp( start, end );
  inp.dec_state( *this );
  return inp.ok;
}

static void
snprintf_qos( char *buf,  size_t len,  RwfQos &qos ) noexcept
{
  const char * t = rdm_qos_time_str[ qos.timeliness % RDM_QOS_TIME_COUNT ];
  const char * r = rdm_qos_rate_str[ qos.rate % RDM_QOS_RATE_COUNT ];
  ::snprintf( buf, len, "%s %s", t, r );
  if ( qos.dynamic != 0 )
    ::snprintf( buf, len - ::strlen( buf ), " dynamic" );
  if ( qos.time_info != 0 )
    ::snprintf( buf, len - ::strlen( buf ), " tinfo=%u", qos.rate_info );
  if ( qos.rate_info != 0 )
    ::snprintf( buf, len - ::strlen( buf ), " rinfo=%u", qos.rate_info );
  buf[ len - 1 ] = '\0';
}

bool
RwfMsgHdr::ref_iter( size_t which,  RwfFieldIter &iter ) noexcept
{
  static const char MSG_CLASS_STR[]      = "msg_class",      CONFLATE_INFO_STR[]  = "conflate_info",
                    DOMAIN_TYPE_STR[]    = "domain_type",    PRIORITY_STR[]       = "priority",
                    STREAM_ID_STR[]      = "stream_id",      PERM_STR[]           = "perm",
                    MSG_FLAGS_STR[]      = "msg_flags",      QOS_STR[]            = "qos",
                    UPDATE_TYPE_STR[]    = "update",         WORST_QOS_STR[]      = "worst_qos",
                    NAK_CODE_STR[]       = "nak_code",       MSG_KEY_STR[]        = "msg_key",
                    TEXT_STR[]           = "text",           EXTENDED_STR[]       = "extended",
                    SEQ_NUM_STR[]        = "seq_num",        POST_USER_STR[]      = "post_user",
                    SECOND_SEQ_NUM_STR[] = "second_seq_num", REQ_MSG_KEY_STR[]    = "req_msg_key",
                    POST_ID_STR[]        = "post_id",        PART_NUM_STR[]       = "part_num",
                    ACK_ID_STR[]         = "ack_id",         POST_RIGHTS_STR[]    = "post_rights",
                    STATE_STR[]          = "state",          CONTAINER_STR[]      = "data",
                    GROUP_ID_STR[]       = "group_id";
  #define Y( a ) a ## _STR
  #define Z( a ) sizeof( a ## _STR )
  #define N( a ) a
  #define X( z ) \
  z( MSG_CLASS ), z( DOMAIN_TYPE ), z( STREAM_ID ), z( MSG_FLAGS ), z( UPDATE_TYPE ), z( NAK_CODE ), z( TEXT ), z( SEQ_NUM ), z( SECOND_SEQ_NUM ), z( POST_ID ), \
  z( ACK_ID ), z( STATE ), z( GROUP_ID ), z( CONFLATE_INFO ), z( PRIORITY ), z( PERM ), z( QOS ), z( WORST_QOS ), z( MSG_KEY ), z( EXTENDED ), \
  z( POST_USER ), z( REQ_MSG_KEY ), z( PART_NUM ), z( POST_RIGHTS ), z( CONTAINER )
  static const char * fname[]    = { X( Y ) };
  static const size_t fnamelen[] = { X( Z ) };
  enum {
    X( N ), END_OF_ITER
  };
  #undef Y
  #undef Z
  #undef N
  #undef X
  RwfMessageIter & it = iter.u.msg;
  size_t i;
  char buf[ 256 ];
  if ( it.field_count == 0 ) {
    i = 0;
    it.index[ i++ ] = MSG_CLASS;
    it.index[ i++ ] = DOMAIN_TYPE;
    it.index[ i++ ] = STREAM_ID;
    it.index[ i++ ] = MSG_FLAGS;
#define z( s, t ) if ( this->test( s ) ) it.index[ i++ ] = t
    z( X_HAS_UPDATE_TYPE, UPDATE_TYPE ); z( X_HAS_NAK_CODE, NAK_CODE ); z( X_HAS_TEXT, TEXT ); z( X_HAS_SEQ_NUM, SEQ_NUM ); z( X_HAS_SECONDARY_SEQ_NUM, SECOND_SEQ_NUM ); z( X_HAS_POST_ID, POST_ID );
    z( X_HAS_ACK_ID, ACK_ID ); z( X_HAS_STATE, STATE ); z( X_HAS_GROUP_ID, GROUP_ID ); z( X_HAS_CONF_INFO, CONFLATE_INFO ); z( X_HAS_PRIORITY, PRIORITY ); z( X_HAS_PERM_DATA, PERM ); z( X_HAS_QOS, QOS );
    z( X_HAS_WORST_QOS, WORST_QOS ); z( X_HAS_MSG_KEY, MSG_KEY ); z( X_HAS_EXTENDED_HEADER, EXTENDED ); z( X_HAS_POST_USER_INFO, POST_USER ); z( X_HAS_REQ_MSG_KEY, REQ_MSG_KEY ); z( X_HAS_PART_NUM, PART_NUM );
    z( X_HAS_POST_USER_RIGHTS, POST_RIGHTS );
#undef z
    if ( this->container_type != RWF_NO_DATA )
      it.index[ i++ ] = CONTAINER;
    it.field_count = i;
    it.index[ i ] = END_OF_ITER;
  }
  i = it.index[ which ];
  it.name    = fname[ i ];
  it.namelen = fnamelen[ i ];
  switch ( i ) {
    case MSG_CLASS:
      if ( this->msg_class < RWF_MSG_CLASS_COUNT )
        iter.set_string( rwf_msg_class_str[ this->msg_class ] );
      else
        iter.set_uint( this->msg_class );
      break;
    case DOMAIN_TYPE:
      if ( this->domain_type < RDM_DOMAIN_COUNT )
        iter.set_string( rdm_domain_str[ this->domain_type ] );
      else
        iter.set_uint( this->domain_type );
      break;
    case STREAM_ID:      iter.set_uint( this->stream_id );      break;
    case MSG_FLAGS: {
      size_t off = 0;
      buf[ 0 ] = '\0';
      for ( int s = 0; s < RWF_MSG_SERIAL_COUNT; s++ ) {
        if ( ( rwf_msg_flag_only[ this->msg_class ] & ( (uint64_t) 1 << s ) ) != 0 ) {
          if ( this->test( (RwfMsgSerial) s ) ) {
            ::snprintf( &buf[ off ], sizeof( buf ) - off, "%s%s", off == 0 ? "" : " ", rwf_msg_serial_str[ s ] );
            buf[ sizeof( buf ) - 1 ] = '\0';
            off += ::strlen( &buf[ off ] );
          }
        }
      }
      iter.alloc_string( buf );
      break;
    }
    case UPDATE_TYPE:
      if ( this->update_type < RDM_UPDATE_TYPE_COUNT )
        iter.set_string( rdm_update_type_str[ this->update_type ] );
      else
        iter.set_uint( this->update_type );
      break;
    case NAK_CODE:       iter.set_uint( this->nak_code );       break;
    case TEXT:
      iter.msg_fptr = (uint8_t *) this->text.buf;
      iter.fsize    = this->text.len;
      iter.ftype    = MD_STRING;
      break;
    case SEQ_NUM:        iter.set_uint( this->seq_num );        break;
    case SECOND_SEQ_NUM: iter.set_uint( this->second_seq_num ); break;
    case POST_ID:        iter.set_uint( this->post_id );        break;
    case ACK_ID:         iter.set_uint( this->ack_id );         break;
    case STATE: {
      const char * d = rdm_data_state_str[ this->state.data_state % RDM_DATA_STATE_COUNT ];
      const char * s = rdm_stream_state_str[ this->state.stream_state % RDM_STREAM_STATE_COUNT ];
      ::snprintf( buf, sizeof( buf ), "%s %s", d, s );
      buf[ sizeof( buf ) - 1 ] = '\0';
      size_t off = ::strlen( buf );
      if ( this->state.code != 0 ) {
        ::snprintf( &buf[ off ], sizeof( buf ) - off, " code=%u", this->state.code );
        buf[ sizeof( buf ) - 1 ] = '\0';
        off += ::strlen( &buf[ off ] );
      }
      if ( this->state.text.len != 0 ) {
        ::snprintf( &buf[ off ], sizeof( buf ) - off, " text=%.*s",
                    this->state.text.len, this->state.text.buf );
      }
      buf[ sizeof( buf ) - 1 ] = '\0';
      iter.alloc_string( buf );
      break;
    }
    case GROUP_ID:  iter.set_opaque( this->group_id ); break;
    case CONFLATE_INFO:
      ::snprintf( buf, sizeof( buf ), "count=%u time=%u",
        this->conf_info.count, this->conf_info.time );
      buf[ sizeof( buf ) - 1 ] = '\0';
      iter.alloc_string( buf );
      break;
    case PRIORITY:
      ::snprintf( buf, sizeof( buf ), "class=%u count=%u",
        this->priority.clas, this->priority.count );
      buf[ sizeof( buf ) - 1 ] = '\0';
      iter.alloc_string( buf );
      break;
    case PERM:  iter.set_opaque( this->perm ); break;
    case QOS:
      snprintf_qos( buf, sizeof( buf ), this->qos );
      iter.alloc_string( buf );
      break;
    case WORST_QOS:
      snprintf_qos( buf, sizeof( buf ), this->worst_qos );
      iter.alloc_string( buf );
      break;
    case MSG_KEY:
      iter.msg_fptr = (uint8_t *) this->msg_key.data;
      iter.fsize    = this->msg_key.data_size;
      iter.ftype    = MD_MESSAGE;
      break;
    case EXTENDED:  iter.set_opaque( this->extended ); break;
    case POST_USER:
      ::snprintf( buf, sizeof( buf ), "user_addr=0x%x user_id=%u",
        this->post_user.user_addr, this->post_user.user_id );
      buf[ sizeof( buf ) - 1 ] = '\0';
      iter.alloc_string( buf );
      break;
    case REQ_MSG_KEY:
      iter.msg_fptr = (uint8_t *) this->req_msg_key.data;
      iter.fsize    = this->req_msg_key.data_size;
      iter.ftype    = MD_MESSAGE;
      break;
    case PART_NUM:    iter.set_uint( this->part_num );    break;
    case POST_RIGHTS: iter.set_uint( this->post_rights ); break;
    case CONTAINER:
      if ( this->data_end >= this->data_start ) {
        it.namelen = 0;
        if ( this->container_type == RWF_FIELD_LIST ) {
          uint8_t * hdr =
            &((uint8_t *) iter.iter_msg().msg_buf)[ this->data_start ];
          if ( ( hdr[ 0 ] & RwfFieldListHdr::HAS_FIELD_LIST_INFO ) != 0 ) {
            uint16_t flist = ( hdr[ 3 ] << 8 ) | hdr[ 4 ];
            if ( flist != 0 ) {
              int n = ::snprintf( buf, sizeof( buf ), "field_list [%u]", flist );
              it.name = iter.iter_msg().mem->stralloc( n, buf );
              it.namelen = n + 1;
            }
          }
        }
        if ( it.namelen == 0 ) {
          #define n( x ) it.name = x; it.namelen = sizeof( x )
          switch ( this->container_type ) {
            case RWF_FIELD_LIST:   n( "field_list" );   break;
            case RWF_ELEMENT_LIST: n( "element_list" ); break;
            case RWF_FILTER_LIST:  n( "filter_list" );  break;
            case RWF_VECTOR:       n( "vector" );       break;
            case RWF_MAP:          n( "map" );          break;
            case RWF_SERIES:       n( "series" );       break;
            default: break;
          }
          #undef n
        }
        iter.field_start = this->data_start;
        iter.fsize       = this->data_end - this->data_start;
        iter.field_end   = this->data_end;
        iter.ftype       = MD_MESSAGE;
        iter.data_start  = this->data_start;
        iter.msg_fptr    = NULL;
        break;
      }
      /* FALLTHRU */
    default:
      return false;
  }
  return true;
}

bool
RwfMsgKey::ref_iter( size_t which,  RwfFieldIter &iter ) noexcept
{
  static const char SERVICE_ID_STR[] = "service_id",
                    NAME_STR[]       = "name",
                    NAME_TYPE_STR[]  = "name_type",
                    FILTER_STR[]     = "filter",
                    IDENTIFIER_STR[] = "identifier",
                    ATTRIB_STR[]     = "attrib";
  #define Y( a ) a ## _STR
  #define Z( a ) sizeof( a ## _STR )
  #define N( a ) a
  #define X( z ) z( SERVICE_ID ), z( NAME ), z( NAME_TYPE ), z( FILTER ), z( IDENTIFIER ), z( ATTRIB )
  static const char * fname[]    = { X( Y ) };
  static const size_t fnamelen[] = { X( Z ) };
  enum {
    X( N ), END_OF_ITER
  };
  #undef Y
  #undef Z
  #undef N
  #undef X
  RwfMsgKeyIter & it = iter.u.msg_key;
  size_t i;
  if ( it.field_count == 0 ) {
    i = 0;
    if ( ( this->key_flags & HAS_SERVICE_ID ) != 0 )
      it.index[ i++ ] = SERVICE_ID;
    if ( ( this->key_flags & HAS_NAME ) != 0 )
      it.index[ i++ ] = NAME;
    if ( ( this->key_flags & HAS_NAME_TYPE ) != 0 )
      it.index[ i++ ] = NAME_TYPE;
    if ( ( this->key_flags & HAS_FILTER ) != 0 )
      it.index[ i++ ] = FILTER;
    if ( ( this->key_flags & HAS_IDENTIFIER ) != 0 )
      it.index[ i++ ] = IDENTIFIER;
    if ( ( this->key_flags & HAS_ATTRIB ) != 0 )
      it.index[ i++ ] = ATTRIB;
    it.field_count = i;
    it.index[ i ] = END_OF_ITER;
  }
  i = it.index[ which ];
  it.name    = fname[ i ];
  it.namelen = fnamelen[ i ];
  switch ( i ) {
    case SERVICE_ID: iter.set_uint( this->service_id ); break;
    case NAME:       iter.set_string( this->name, this->name_len ); break;
    case NAME_TYPE:  iter.set_uint( this->name_type );  break;
    case FILTER:     iter.set_uint( this->filter );     break;
    case IDENTIFIER: iter.set_uint( this->identifier ); break;
    case ATTRIB:
      iter.msg_fptr = (uint8_t *) this->attrib.data;
      iter.fsize    = this->attrib.len;
      iter.ftype    = MD_MESSAGE;
      break;
    default:
      return false;
  }
  return true;
}

int
RwfFieldIter::unpack_message_entry( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  if ( ! msg.msg.ref_iter( this->field_index, *this ) )
    return Err::NOT_FOUND;
  return 0;
}

int
RwfFieldIter::unpack_msg_key_entry( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  if ( ! msg.msg_key.ref_iter( this->field_index, *this ) )
    return Err::NOT_FOUND;
  return 0;
}

int
RwfFieldListHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_FIELD_LIST )
    return Err::BAD_HEADER;

  this->type_id   = RWF_FIELD_LIST;
  this->flags     = 0;
  this->dict_id   = 1;
  this->flist     = 0;
  this->set_id    = 0;
  this->field_cnt = 0;
  this->field_set = NULL;

  dec.u8( this->flags );
  /* which dict and flist */
  if ( ( this->flags & HAS_FIELD_LIST_INFO ) != 0 ) {
    size_t sz = 0;
    dec.u8( sz );
    if ( sz >= 3 ) {
      dec.u8 ( this->dict_id )
         .u16( this->flist );
      sz -= 3;
    }
    if ( sz > 0 )
      dec.incr( sz );
  }
  if ( ( this->flags & HAS_SET_DATA ) != 0 ) {
    if ( ( this->flags & HAS_SET_ID ) != 0 )
      dec.u15( this->set_id );
    /* no length, calculate item count on demenad */
    if ( ( this->flags & HAS_STANDARD_DATA ) == 0 ) {
      this->set_start = dec.offset( off );
    }
    else { /* both set and standard data, has size of set */
      dec.u15( this->set_size );
      this->set_start = dec.offset( off );
      dec.incr( this->set_size );
    }
  }
  /* standard data */
  if ( ( this->flags & HAS_STANDARD_DATA ) != 0 ) {
    dec.u16( this->field_cnt );
    this->data_start = dec.offset( off );
  }
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_field_list_entry( void ) noexcept
{
  uint8_t * buf = (uint8_t *) this->iter_msg().msg_buf,
          * eob = &buf[ this->field_end ];
  size_t    i   = this->field_start + 2,
            j   = get_fe_prefix( &buf[ i ], eob, this->fsize );

  if ( j == 0 || &buf[ i + j + (size_t) this->fsize ] > eob )
    return Err::BAD_FIELD_BOUNDS;

  /* <fid><fsize><fdata> */
  this->ftype       = MD_NODATA; /* uses lookup_fid() */
  this->u.field.fid = get_i16<MD_BIG>( &buf[ i - 2 ] );
  this->data_start  = i + j;
  this->field_end   = this->data_start + this->fsize;
  return 0;
}

int
RwfFieldIter::unpack_field_list_defn( void ) noexcept
{
  RwfMsg  & msg = (RwfMsg &) this->iter_msg();
  uint8_t * buf = (uint8_t *) msg.msg_buf,
          * eob = &buf[ this->field_end ];
  size_t    i   = this->field_start;
  uint8_t   rwf_type;

  rwf_type = msg.fields.field_set->get_fid( this->field_index, this->ftype,
                                            this->fsize, this->u.field.fid );
  this->u.field.rwf_type = rwf_type;
  if ( rwf_type == RWF_NONE )
    return Err::BAD_FIELD_TYPE;

  if ( this->iter_msg().dict != NULL ) {
    MDLookup by( this->u.field.fid );
    if ( this->iter_msg().dict->lookup( by ) ) {
      this->u.field.fname    = by.fname;
      this->u.field.fnamelen = by.fname_len;
    }
  }
  if ( this->fsize == 0 ) {
    if ( rwf_type == RWF_REAL_4RB ) {
      this->fsize = get_n32_prefix_len( &buf[ i ], eob );
      if ( this->fsize == 0 )
        return Err::BAD_FIELD_BOUNDS;
    }
    else if ( rwf_type == RWF_REAL_8RB ) {
      this->fsize = get_n64_prefix_len( &buf[ i ], eob );
      if ( this->fsize == 0 )
        return Err::BAD_FIELD_BOUNDS;
    }
    else {
      size_t sz = get_fe_prefix( &buf[ i ], eob, this->fsize );
      i += sz;
      if ( sz == 0 )
        return Err::BAD_FIELD_BOUNDS;
    }
  }
  if ( &buf[ i + (size_t) this->fsize ] > eob )
    return Err::BAD_FIELD_BOUNDS;

  /* <fdata> */
  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;
  return 0;
}

int
RwfMapHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_MAP )
    return Err::BAD_HEADER;

  this->type_id        = RWF_MAP;
  this->flags          = 0;
  this->key_type       = RWF_NONE;
  this->key_fid        = 0;
  this->container_type = RWF_NO_DATA;
  this->summary_start  = 0;
  this->summary_size   = 0;
  this->key_fname      = NULL;
  this->key_fnamelen   = 0;
  this->key_ftype      = MD_NODATA;

  dec.u8( this->flags )
     .u8( this->key_type )
     .u8( this->container_type );
  this->container_type += RWF_CONTAINER_BASE;

  if ( ( this->flags & HAS_KEY_FID ) != 0 )
    dec.u16( this->key_fid );

  if ( ( this->flags & HAS_SET_DEFS ) != 0 ) {
    dec.u15( this->set_size );
    this->set_start = dec.offset( off );
    dec.incr( this->set_size );
  }
  if ( ( this->flags & HAS_SUMMARY_DATA ) != 0 ) {
    dec.u15( this->summary_size );
    this->summary_start = dec.offset( off );
    dec.incr( this->summary_size );
  }
  if ( ( this->flags & HAS_COUNT_HINT ) != 0 )
    dec.u30( this->hint_cnt );
  dec.u16( this->entry_cnt );
  this->data_start = dec.offset( off );

  if ( ! rwf_primitive_to_md_type( this->key_type, this->key_ftype ) )
    return Err::BAD_FIELD_TYPE;
  if ( ! dec.ok || ! is_rwf_container( (RWF_type) this->container_type ) )
    return Err::BAD_HEADER;
  return 0;
}

template<class Hdr>
static inline size_t
unpack_summary( RwfFieldIter &iter,  Hdr &hdr )
{
  if ( iter.field_index == 0 ) {
    if ( hdr.summary_size != 0 ) {
      iter.field_start = hdr.summary_start;
      iter.fsize       = hdr.summary_size;
      iter.field_end   = iter.field_start + iter.fsize;
      iter.ftype       = MD_MESSAGE;
      iter.data_start  = iter.field_start;
      return 0;
    }
    return hdr.data_start;
  }
  if ( iter.field_index == 1 ) {
    if ( hdr.summary_size != 0 )
      return hdr.data_start;
  }
  return iter.field_start;
}

static inline size_t
unpack_perm( const uint8_t *buf,  const uint8_t *eob,  RwfPerm &perm )
{
  size_t sz = get_u15_prefix( buf, eob, perm.len );
  perm.buf = (void *) &buf[ sz ];
  return sz;
}

int
RwfFieldIter::unpack_map_entry( void ) noexcept
{
  RwfMapIter & it  = this->u.map;
  RwfMsg     & msg = (RwfMsg &) this->iter_msg();
  uint8_t    * buf = (uint8_t *) msg.msg_buf,
             * eob = &buf[ msg.msg_end ];
  size_t       i, sz, keyoff;
  /* first time, return summary message, if any */

  if ( (i = unpack_summary( *this, msg.map )) == 0 ) {
    it.flags  = 0;
    it.action = MAP_SUMMARY;
    return 0;
  }
  this->field_start = i;
  if ( &buf[ i ] >= eob )
    return Err::NOT_FOUND;

  it.flags   = buf[ i++ ];
  it.action  = (RwfMapAction) ( it.flags & 0xf );
  it.flags >>= 4;

  keyoff = i; /* after action and perm data, if any */
  if ( ((msg.map.flags | it.flags ) & RwfMapHdr::HAS_PERM_DATA) != 0 ) {
    if ( (sz = unpack_perm( &buf[ i ], eob, it.perm )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    keyoff = i + sz + it.perm.len;
  }
  else {
    ::memset( &it.perm, 0, sizeof( it.perm ) );
  }
  /* decode key len, is a primitive */
  if ( (sz = get_u15_prefix( &buf[ keyoff ], eob, it.keylen ) ) == 0 )
    return Err::BAD_FIELD_BOUNDS;

  it.key = &buf[ keyoff + sz ];
  i      = keyoff + sz + (size_t) it.keylen;

  this->fsize = 0; /* no data for delete */
  this->ftype = MD_OPAQUE;

  if ( it.action != MAP_DELETE_ENTRY &&
       msg.map.container_type != RWF_NO_DATA ) {
    if ( (sz = get_fe_prefix( &buf[ i ], eob, this->fsize )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz;
    if ( this->fsize > 0 )
      this->ftype = MD_MESSAGE;
  }
  this->field_end  = i + this->fsize;
  this->data_start = i;

  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

int
RwfElementListHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_ELEMENT_LIST )
    return Err::BAD_HEADER;

  this->type_id   = RWF_ELEMENT_LIST;
  this->flags     = 0;
  this->list_num  = 0;
  this->set_id    = 0;
  this->item_cnt  = 0;
  this->field_set = NULL;

  dec.u8( this->flags );
  if ( ( this->flags & HAS_ELEMENT_LIST_INFO ) != 0 ) {
    size_t sz;
    dec.u8( sz );
    if ( sz >= 2 ) {
      dec.u16( this->list_num );
      sz -= 2;
    }
    if ( sz > 0 )
      dec.incr( sz );
  }
  if ( ( this->flags & HAS_SET_DATA ) != 0 ) {
    if ( ( this->flags & HAS_SET_ID ) != 0 )
      dec.u15( this->set_id );
    /* no length, calculate item count on demenad */
    if ( ( this->flags & HAS_STANDARD_DATA ) == 0 ) {
      this->set_start = dec.offset( off );
    }
    else { /* both set and standard data, has size of set */
      dec.u15( this->set_size );
      this->set_start = dec.offset( off );
      dec.incr( this->set_size );
    }
  }
  /* standard data */
  if ( ( this->flags & HAS_STANDARD_DATA ) != 0 ) {
    dec.u16( this->item_cnt );
    this->data_start = dec.offset( off );
  }
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_element_list_entry( void ) noexcept
{
  RwfElementListIter & it = this->u.elist;
  uint8_t * buf = (uint8_t *) this->iter_msg().msg_buf,
          * eob = &buf[ this->field_end ];
  size_t    i   = this->field_start,
            sz;

  if ( &buf[ i ] >= eob )
    return Err::NOT_FOUND;
  sz = get_u15_prefix( &buf[ i ], eob, it.namelen );
  if ( sz == 0 || &buf[ i + sz + it.namelen + 1 ] > eob )
    return Err::BAD_FIELD_BOUNDS;

  it.name     = (char *) &buf[ i + sz ];
  i          += sz + it.namelen;
  it.type     = (RWF_type) buf[ i++ ];
  this->fsize = 0;
  this->ftype = MD_OPAQUE;

  if ( it.type != RWF_NO_DATA ) {
    if ( (sz = get_fe_prefix( &buf[ i ], eob, this->fsize )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz;
    if ( ! rwf_primitive_to_md_type( (RWF_type) it.type, this->ftype ) )
      return Err::BAD_FIELD_TYPE;
  }
  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;
  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

int
RwfFieldIter::unpack_element_list_defn( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  RwfElementListIter & it = this->u.elist;
  uint8_t * buf = (uint8_t *) msg.msg_buf,
          * eob = &buf[ this->field_end ];
  size_t    i   = this->field_start;

  if ( ! msg.elist.field_set->get_elem( this->field_index, this->ftype, this->fsize,
                                        it.name, it.namelen ) )
    return Err::BAD_FIELD_TYPE;

  if ( this->fsize == 0 ) {
    size_t sz;
    if ( (sz = get_fe_prefix( &buf[ i ], eob, this->fsize )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz;
  }
  if ( &buf[ i + this->fsize ] > eob )
    return Err::BAD_FIELD_BOUNDS;

  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;
  return 0;
}

int
RwfFilterListHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_FILTER_LIST )
    return Err::BAD_HEADER;

  this->type_id        = RWF_FILTER_LIST;
  this->flags          = 0;
  this->container_type = RWF_NO_DATA;
  this->hint_cnt       = 0;
  this->item_cnt       = 0;

  dec.u8( this->flags )
     .u8( this->container_type );
  this->container_type += RWF_CONTAINER_BASE;

  if ( ( this->flags & HAS_COUNT_HINT ) != 0 )
    dec.u8( this->hint_cnt );
  dec.u8( this->item_cnt );
  this->data_start = dec.offset( off );

  if ( ! dec.ok || ! is_rwf_container( (RWF_type) this->container_type ) )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_filter_list_entry( void ) noexcept
{
  RwfFilterListIter & it = this->u.flist;
  RwfMsg     & msg    = (RwfMsg &) this->iter_msg();
  uint8_t    * buf    = (uint8_t *) msg.msg_buf,
             * eob    = &buf[ this->field_end ];
  size_t       i      = this->field_start, sz;

  if ( &buf[ i + 2 ] > eob )
    return Err::NOT_FOUND;
  it.flags   = buf[ i++ ];
  it.id      = buf[ i++ ];
  it.action  = (RwfFilterAction) ( it.flags & 0xf );
  it.flags >>= 4;

  if ( ( it.flags & RwfFilterListHdr::ENTRY_HAS_CONTAINER_TYPE ) != 0 ) {
    if ( &buf[ i + 1 ] > eob )
      return Err::BAD_FIELD_BOUNDS;
    it.type = buf[ i++ ] + RWF_CONTAINER_BASE;
  }
  else
    it.type = msg.flist.container_type;

  if ( ( ( it.flags | msg.flist.flags ) &
       RwfFilterListHdr::HAS_PERM_DATA ) != 0 ) {
    if ( (sz = unpack_perm( &buf[ i ], eob, it.perm )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz + it.perm.len;
  }
  else {
    ::memset( &it.perm, 0, sizeof( it.perm ) );
  }

  this->fsize = 0;
  this->ftype = MD_OPAQUE;

  if ( it.type != RWF_NO_DATA && it.action != FILTER_CLEAR_ENTRY ) {
    if ( (sz = get_fe_prefix( &buf[ i ], eob, this->fsize )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz;
    if ( this->fsize > 0 )
      this->ftype = MD_MESSAGE;
  }
  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;

  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

int
RwfSeriesHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_SERIES )
    return Err::BAD_HEADER;
  this->type_id        = RWF_SERIES;
  this->flags          = 0;
  this->container_type = RWF_NO_DATA;
  this->summary_start  = 0;
  this->summary_size   = 0;
  this->hint_cnt       = 0;
  this->item_cnt       = 0;

  dec.u8( this->flags )
     .u8( this->container_type );
  this->container_type += RWF_CONTAINER_BASE;

  if ( ( this->flags & HAS_SET_DEFS ) != 0 ) {
    dec.u15( this->set_size );
    this->set_start = dec.offset( off );
    dec.incr( this->set_size );
  }
  if ( ( this->flags & HAS_SUMMARY_DATA ) != 0 ) {
    dec.u15( this->summary_size );
    this->summary_start = dec.offset( off );
    dec.incr( this->summary_size );
  }
  if ( ( this->flags & HAS_COUNT_HINT ) != 0 )
    dec.u30( this->hint_cnt );
  dec.u16( this->item_cnt );

  this->data_start = dec.offset( off );
  if ( ! dec.ok || ! is_rwf_container( (RWF_type) this->container_type ) )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_series_entry( void ) noexcept
{
  RwfSeriesIter & it = this->u.series;
  RwfMsg  & msg = (RwfMsg &) this->iter_msg();
  uint8_t * buf = (uint8_t *) msg.msg_buf,
          * eob = &buf[ msg.msg_end ];
  size_t    i, sz;

  if ( (i = unpack_summary( *this, msg.series )) == 0 ) {
    it.is_summary = true;
    return 0;
  }
  it.is_summary = false;
  this->field_start = i;
  if ( &buf[ i ] >= eob )
    return Err::NOT_FOUND;

  sz = get_fe_prefix( &buf[ i ], eob, this->fsize );

  if ( sz == 0 || &buf[ i + sz + (size_t) this->fsize ] > eob )
    return Err::BAD_FIELD_BOUNDS;

  /* <fid><fsize><fdata> */
  this->ftype      = MD_MESSAGE;
  this->data_start = i + sz;
  this->field_end  = this->data_start + this->fsize;

  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

int
RwfVectorHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  RwfDecoder dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_VECTOR )
    return Err::BAD_HEADER;

  this->type_id        = RWF_VECTOR;
  this->flags          = 0;
  this->container_type = RWF_NO_DATA;
  this->summary_start  = 0;
  this->summary_size   = 0;
  this->hint_cnt       = 0;
  this->item_cnt       = 0;

  dec.u8( this->flags )
     .u8( this->container_type );
  this->container_type += RWF_CONTAINER_BASE;

  if ( ( this->flags & HAS_SET_DEFS ) != 0 ) {
    dec.u15( this->set_size );
    this->set_start = dec.offset( off );
    dec.incr( this->set_size );
  }
  if ( ( this->flags & HAS_SUMMARY_DATA ) != 0 ) {
    dec.u15( this->summary_size );
    this->summary_start = dec.offset( off );
    dec.incr( this->summary_size );
  }
  if ( ( this->flags & HAS_COUNT_HINT ) != 0 )
    dec.u30( this->hint_cnt );
  dec.u16( this->item_cnt );

  this->data_start = dec.offset( off );
  if ( ! dec.ok || ! is_rwf_container( (RWF_type) this->container_type ) )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_vector_entry( void ) noexcept
{
  RwfVectorIter & it = this->u.vector;
  RwfMsg     & msg = (RwfMsg &) this->iter_msg();
  uint8_t    * buf = (uint8_t *) msg.msg_buf,
             * eob = &buf[ msg.msg_end ];
  size_t       i, sz;

  if ( (i = unpack_summary( *this, msg.vector )) == 0 ) {
    it.flags  = 0;
    it.action = VECTOR_SUMMARY;
    return 0;
  }
  this->field_start = i;
  if ( &buf[ i ] >= eob )
    return Err::NOT_FOUND;

  it.flags   = buf[ i++ ];
  it.action  = (RwfVectorAction) ( it.flags & 0xf );
  it.flags >>= 4;

  if ( (sz = get_u30_prefix( &buf[ i ], eob, it.index )) == 0)
    return Err::BAD_FIELD_BOUNDS;
  i += sz;

  if ( ( ( it.flags | msg.vector.flags ) & RwfVectorHdr::HAS_PERM_DATA ) != 0 ) {
    if ( (sz = unpack_perm( &buf[ i ], eob, it.perm )) == 0 )
      return Err::BAD_FIELD_BOUNDS;
    i += sz + it.perm.len;
  }
  else {
    ::memset( &it.perm, 0, sizeof( it.perm ) );
  }
  this->fsize = 0;
  this->ftype = MD_OPAQUE;

  if ( it.action != VECTOR_CLEAR_ENTRY && it.action != VECTOR_DELETE_ENTRY &&
       msg.vector.container_type != RWF_NO_DATA ) {
    sz = get_fe_prefix( &buf[ i ], eob, this->fsize );

    if ( sz == 0 || &buf[ i + sz + (size_t) this->fsize ] > eob )
      return Err::BAD_FIELD_BOUNDS;

    if ( this->fsize > 0 )
      this->ftype = MD_MESSAGE;
    i += sz;
  }
  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;

  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

RwfMsg *
RwfMsg::unpack_field_list( void *bb,  size_t off,  size_t end,  uint32_t h,
                           MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfFieldListHdr hdr;
  ::memset( &hdr, 0, sizeof( hdr ) );
  if ( hdr.parse( bb, off, end ) != 0 ) {
    switch ( hdr.type_id ) {
      case RWF_MAP:          return RwfMsg::unpack_map( bb, off, end, h, d, m );
      case RWF_ELEMENT_LIST: return RwfMsg::unpack_element_list( bb, off, end, h, d, m );
      case RWF_FILTER_LIST:  return RwfMsg::unpack_filter_list( bb, off, end, h, d, m );
      case RWF_SERIES:       return RwfMsg::unpack_series( bb, off, end, h, d, m );
      case RWF_VECTOR:       return RwfMsg::unpack_vector( bb, off, end, h, d, m );
      case RWF_MSG:          return RwfMsg::unpack_message( bb, off, end, h, d, m );
      case RWF_MSG_KEY:      return RwfMsg::unpack_msg_key( bb, off, end, h, d, m );
      default:               return NULL;
    }
  }
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( RwfMsg ), &ptr );
  for ( ; d != NULL; d = d->get_next() )
    if ( d->dict_type[ 0 ] == 'a' ) /* need app_a type */
      break;
  RwfMsg *msg = new ( ptr ) RwfMsg( bb, off, end, d, m );
  msg->fields = hdr;
  return msg;
}

template<class T>
static inline RwfMsg *
unpack_rwf( void *bb,  size_t off,  size_t end,  uint32_t, MDDict *&d,
            MDMsgMem &m,  T &hdr ) noexcept
{
  ::memset( &hdr, 0, sizeof( T ) );
  if ( hdr.parse( bb, off, end ) != 0 )
    return NULL;
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( RwfMsg ), &ptr );
  for ( ; d != NULL; d = d->get_next() )
    if ( d->dict_type[ 0 ] == 'a' ) /* need app_a type */
      break;
  RwfMsg *msg = new ( ptr ) RwfMsg( bb, off, end, d, m );
  return msg;
}

RwfMsg *
RwfMsg::unpack_map( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                    MDMsgMem &m ) noexcept
{
  RwfMapHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfMapHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL ) {
    if ( d != NULL && hdr.key_fid != 0 ) {
      MDLookup by( hdr.key_fid );
      if ( d->lookup( by ) ) {
        hdr.key_fnamelen = by.fname_len;
        hdr.key_fname    = by.fname;
      }
    }
    msg->map = hdr;
  }
  return msg;
}

RwfMsg *
RwfMsg::unpack_element_list( void *bb,  size_t off,  size_t end,  uint32_t h,
                             MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfElementListHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfElementListHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->elist = hdr;
  return msg;
}

RwfMsg *
RwfMsg::unpack_filter_list( void *bb,  size_t off,  size_t end,  uint32_t h,
                            MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfFilterListHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfFilterListHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->flist = hdr;
  return msg;
}

RwfMsg *
RwfMsg::unpack_series( void *bb,  size_t off,  size_t end,  uint32_t h,
                       MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfSeriesHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfSeriesHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->series = hdr;
  return msg;
}

RwfMsg *
RwfMsg::unpack_vector( void *bb,  size_t off,  size_t end,  uint32_t h,
                       MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfVectorHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfVectorHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->vector = hdr;
  return msg;
}

RwfMsg *
RwfMsg::unpack_message( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfMsgHdr hdr;
  RwfMsg * msg = unpack_rwf<RwfMsgHdr>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->msg = hdr;
  return msg;
}

RwfMsg *
RwfMsg::unpack_msg_key( void *bb,  size_t off,  size_t end,  uint32_t h,
                        MDDict *d,  MDMsgMem &m ) noexcept
{
  RwfMsgKey hdr;
  RwfMsg * msg = unpack_rwf<RwfMsgKey>( bb, off, end, h, d, m, hdr );
  if ( msg != NULL )
    msg->msg_key = hdr;
  return msg;
}

int
RwfMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( RwfFieldIter ), &ptr );
  iter = new ( ptr ) RwfFieldIter( *this );
  return 0;
}

RwfMsg *
RwfMsg::unpack_sub_msg( uint8_t type,  size_t start,  size_t end ) noexcept
{
  MDDict   * d  = this->dict;
  MDMsgMem & m  = *this->mem;
  void     * bb = this->msg_buf;

  switch ( type ) {
    case RWF_FIELD_LIST:   return RwfMsg::unpack_field_list( bb, start, end, 0, d, m );
    case RWF_ELEMENT_LIST: return RwfMsg::unpack_element_list( bb, start, end, 0, d, m );
    case RWF_FILTER_LIST:  return RwfMsg::unpack_filter_list( bb, start, end, 0, d, m );
    case RWF_VECTOR:       return RwfMsg::unpack_vector( bb, start, end, 0, d, m );
    case RWF_MAP:          return RwfMsg::unpack_map( bb, start, end, 0, d, m );
    case RWF_SERIES:       return RwfMsg::unpack_series( bb, start, end, 0, d, m );
    case RWF_MSG:          return RwfMsg::unpack_message( bb, start, end, 0, d, m );
    case RWF_MSG_KEY:      return RwfMsg::unpack_msg_key( bb, start, end, 0, d, m );
    default:
      return NULL;
  }
}

RwfMsg *
RwfMsg::get_container_msg( void ) noexcept
{
  if ( this->base.type_id != RWF_MSG )
    return NULL;
  uint8_t type = this->msg.container_type;
  return this->unpack_sub_msg( type, this->msg.data_start, this->msg.data_end );
}

RwfMsg *
RwfMsg::get_summary_msg( void ) noexcept
{
  size_t  start = 0,
          size = 0;
  uint8_t type = 0;
  switch ( this->base.type_id ) {
    case RWF_MAP:
      type  = this->map.container_type;
      start = this->map.summary_start;
      size  = this->map.summary_size;
      break;
    case RWF_SERIES:
      type  = this->series.container_type;
      start = this->series.summary_start;
      size  = this->series.summary_size;
      break;
    case RWF_VECTOR:
      type  = this->vector.container_type;
      start = this->vector.summary_start;
      size  = this->vector.summary_size;
      break;
    default:
      return NULL;
  }
  if ( size == 0 )
    return NULL;
  return this->unpack_sub_msg( type, start, start + size );
}

RwfMsg *
RwfMsg::get_msg_key_attributes( void ) noexcept
{
  if ( this->base.type_id != RWF_MSG )
    return NULL;
  uint8_t   type = this->msg.msg_key.attrib.container_type;
  uint8_t * data = (uint8_t *) this->msg.msg_key.attrib.data;
  size_t    len  = this->msg.msg_key.attrib.len;
  uint8_t * bb    = (uint8_t *) this->msg_buf;
  size_t    start = (size_t) ( data - bb );
  if ( len == 0 )
    return NULL;
  return this->unpack_sub_msg( type, start, start + len );
}

int
RwfMsg::get_sub_msg( MDReference &mref,  MDMsg *&msg,
                     MDFieldIter *iter ) noexcept
{
  uint8_t * bb    = (uint8_t *) this->msg_buf;
  size_t    start = (size_t) ( mref.fptr - bb );
  uint8_t   type  = RWF_NO_DATA;
  RwfMsg  * rwf_msg;

  if ( this->base.type_id == RWF_MAP )
    type = this->map.container_type;
  else if ( this->base.type_id == RWF_FILTER_LIST ) {
    if ( iter != NULL )
      type = ((RwfFieldIter *) iter)->u.flist.type;
  }
  else if ( this->base.type_id == RWF_SERIES )
    type = this->series.container_type;
  else if ( this->base.type_id == RWF_VECTOR )
    type = this->vector.container_type;
  else if ( this->base.type_id == RWF_MSG ) {
    type = this->msg.container_type;
    if ( mref.fptr == (uint8_t *) this->msg.msg_key.data ||
         mref.fptr == (uint8_t *) this->msg.req_msg_key.data )
      type = RWF_MSG_KEY;
  }
  else if ( this->base.type_id == RWF_MSG_KEY )
    type = this->msg_key.attrib.container_type;

  rwf_msg = this->unpack_sub_msg( type, start, start + mref.fsize );
  msg     = rwf_msg;
  if ( rwf_msg == NULL )
    return Err::BAD_SUB_MSG;
  rwf_msg->parent = this;
  return 0;
}

bool
RwfMsg::get_field_defn_db( void ) noexcept
{
  uint16_t my_set_id = 0;
  if ( this->base.type_id == RWF_FIELD_LIST ) {
    if ( this->fields.field_set != NULL )
      return true;
    my_set_id = this->fields.set_id;
  }
  else if ( this->base.type_id == RWF_ELEMENT_LIST ) {
    if ( this->elist.field_set != NULL )
      return true;
    my_set_id = this->elist.set_id;
  }
  else { /* no field data for others */
    return false;
  }

  for ( RwfMsg *p = this->parent; p != NULL; p = p->parent ) {
    if ( p->base.set_start != 0 ) {
      RwfDecoder dec( p->msg_buf, p->base.set_start, p->base.set_start + p->base.set_size );
      uint8_t flags, num_sets = 0;
      dec.u8( flags );
      dec.u8( num_sets );
      for (;;) {
        uint16_t set_id = 0,
                 set_cnt = 0,
                 name_len, i;
        uint8_t  type;

        dec.u15( set_id );
        dec.u8( set_cnt );
        if ( ! dec.ok )
          break;
        if ( set_id == my_set_id ) {
          if ( this->base.type_id == RWF_FIELD_LIST ) {
            this->mem->alloc( RwfFieldListSet::alloc_size( set_cnt ), &this->fields.field_set );
            for ( i = 0; i < set_cnt; i++ ) {
              dec.u16( this->fields.field_set->entry[ i ].fid );
              dec.u8( this->fields.field_set->entry[ i ].type );
            }
            if ( dec.ok ) {
              this->fields.field_set->count = set_cnt;
              this->fields.field_set->set_id = my_set_id;
            }
            else
              this->fields.field_set = NULL;
          }
          else {
            this->mem->alloc( RwfElementListSet::alloc_size( set_cnt ), &this->elist.field_set );
            for ( i = 0; i < set_cnt; i++ ) {
              dec.u15( name_len );
              this->elist.field_set->entry[ i ].name_len = name_len;
              this->elist.field_set->entry[ i ].name     = (char *) dec.buf;
              dec.incr( name_len );
              dec.u8( this->elist.field_set->entry[ i ].type );
            }
            if ( dec.ok ) {
              this->elist.field_set->count  = set_cnt;
              this->elist.field_set->set_id = my_set_id;
            }
            else
              this->elist.field_set = NULL;
          }
          return dec.ok;
        }
        if ( --num_sets == 0 )
          break;
        /* skip this set, go to next */
        if ( this->base.type_id == RWF_FIELD_LIST )
          dec.incr( 3 * set_cnt );
        else {
          for ( i = 0; i < set_cnt; i++ ) {
            dec.u15( name_len );
            dec.incr( name_len );
            dec.u8( type );
          }
        }
      }
    }
  }
  return false;
}

void
RwfFieldIter::lookup_fid( void ) noexcept
{
  if ( this->ftype == MD_NODATA ) {
    if ( this->iter_msg().dict != NULL ) {
      MDLookup by( this->u.field.fid );
      if ( this->iter_msg().dict->lookup( by ) ) {
        this->ftype = by.ftype;
        this->fsize = by.fsize;
        this->u.field.fname    = by.fname;
        this->u.field.fnamelen = by.fname_len;
        this->u.field.rwf_type = RWF_NONE;
      }
    }
    if ( this->ftype == MD_NODATA ) { /* undefined fid or no dictionary */
      this->ftype = MD_OPAQUE;
      this->u.field.fname    = NULL;
      this->u.field.fnamelen = 0;
      this->u.field.rwf_type = RWF_NONE;
    }
  }
}

int
RwfFieldIter::get_name( MDName &name ) noexcept
{
  static const char nul_str[] = "-nul", upd_str[] = "-upd", set_str[] = "-set",
                    clr_str[] = "-clr", ins_str[] = "-ins", del_str[] = "-del",
                    add_str[] = "-add";
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  name.fid      = 0;
  name.fnamelen = 0;
  name.fname    = NULL;
  switch ( msg.base.type_id ) {
    case RWF_FIELD_LIST:
      if ( this->ftype == MD_NODATA )
        this->lookup_fid();
      name.fid      = this->u.field.fid;
      name.fnamelen = this->u.field.fnamelen;
      name.fname    = this->u.field.fname;
      break;
    case RWF_ELEMENT_LIST:
      name.fnamelen = this->u.elist.namelen;
      name.fname    = this->u.elist.name;
      break;
    case RWF_FILTER_LIST: {
      RwfFilterListIter & it = this->u.flist;
      const char * op_str = nul_str;
      if ( it.action == FILTER_UPDATE_ENTRY )
        op_str = upd_str;
      else if ( it.action == FILTER_SET_ENTRY )
        op_str = set_str;
      else if ( it.action == FILTER_CLEAR_ENTRY )
        op_str = clr_str;

      char * tmp_buf = NULL;
      size_t digs    = uint_digs( it.id );
      msg.mem->alloc( digs + 5, &tmp_buf );
      uint_str( it.id, tmp_buf, digs );
      ::memcpy( &tmp_buf[ digs ], op_str, 5 );

      name.fname    = tmp_buf;
      name.fnamelen = digs + 5;
      break;
    }
    case RWF_VECTOR: {
      static const char summary_str[] = "vector-summary";
      if ( this->field_index == 0 && msg.vector.summary_size != 0 ) {
        name.fnamelen = sizeof( summary_str );
        name.fname    = summary_str;
      }
      else {
        RwfVectorIter & it = this->u.vector;
        const char    * op_str = nul_str;
        if ( it.action == VECTOR_UPDATE_ENTRY )
          op_str = upd_str;
        else if ( it.action == VECTOR_SET_ENTRY )
          op_str = set_str;
        else if ( it.action == VECTOR_CLEAR_ENTRY )
          op_str = clr_str;
        else if ( it.action == VECTOR_INSERT_ENTRY )
          op_str = ins_str;
        else if ( it.action == VECTOR_DELETE_ENTRY )
          op_str = del_str;

        char * tmp_buf = NULL;
        size_t digs    = uint_digs( it.index );
        msg.mem->alloc( digs + 5, &tmp_buf );
        uint_str( it.index, tmp_buf, digs );
        ::memcpy( &tmp_buf[ digs ], op_str, 5 );

        name.fname    = tmp_buf;
        name.fnamelen = digs + 5;
      }
      break;
    }
    case RWF_MAP: {
      static const char summary_str[] = "map-summary";
      if ( this->field_index == 0 && msg.map.summary_size != 0 ) {
        name.fnamelen = sizeof( summary_str );
        name.fname    = summary_str;
      }
      else {
        RwfMapIter & it = this->u.map;
        MDReference mref;
        const char * op_str = nul_str;
        char nul = 0;
        mref.ftype = msg.map.key_ftype;
        mref.fsize = it.keylen;
        mref.fptr  = (uint8_t *) it.key;

        char * tmp_buf = NULL;
        size_t tmp_len = 0;
        if ( mref.ftype != MD_NODATA ) {
          bool is_ascii = false;
          if ( mref.ftype == MD_STRING || mref.ftype == MD_OPAQUE ) {
            tmp_buf = (char *) mref.fptr;
            for ( tmp_len = 0; tmp_len < mref.fsize; tmp_len++ ) {
              if ( tmp_buf[ tmp_len ] < ' ' || tmp_buf[ tmp_len ] > '~' )
                break;
            }
            if ( tmp_len == mref.fsize )
              is_ascii = true;
            else if ( tmp_len == mref.fsize - 1 && tmp_buf[ tmp_len ] == '\0' ) {
              is_ascii = true;
              tmp_len--;
            }
            else {
              tmp_len = 0;
            }
          }
          if ( ! is_ascii ) {
            this->decode_ref( mref );
            msg.get_string( mref, tmp_buf, tmp_len );
          }
        }
        if ( tmp_len == 0 ) {
          tmp_buf = (char *) msg.map.key_fname;
          tmp_len = msg.map.key_fnamelen;
          if ( tmp_len > 0 && tmp_buf[ tmp_len - 1 ] == '\0' )
            tmp_len--;
        }
        if ( tmp_len == 0 ) {
          tmp_buf = &nul;
          tmp_len = 0;
        }
        if ( it.action == MAP_UPDATE_ENTRY )
          op_str = upd_str;
        else if ( it.action == MAP_ADD_ENTRY )
          op_str = add_str;
        else if ( it.action == MAP_DELETE_ENTRY )
          op_str = del_str;

        msg.mem->extend( tmp_len + 1, tmp_len + 5, &tmp_buf );
        ::memcpy( &tmp_buf[ tmp_len ], op_str, 5 );
        name.fname    = tmp_buf;
        name.fnamelen = tmp_len + 5;
        name.fid      = msg.map.key_fid;
      }
      break;
    }
    case RWF_SERIES: {
      static const char summary_str[] = "series-summary";
      if ( this->field_index == 0 && msg.series.summary_size != 0 ) {
        name.fnamelen = sizeof( summary_str );
        name.fname    = summary_str;
      }
      else {
        size_t idx     = this->field_index + ( msg.series.summary_size == 0 ? 1 : 0 );
        char * tmp_buf = NULL;
        size_t digs    = uint_digs( idx );
        msg.mem->alloc( digs + 1, &tmp_buf );
        uint_str( idx, tmp_buf, digs );

        tmp_buf[ digs ] = '\0';
        name.fname    = tmp_buf;
        name.fnamelen = digs + 1;
      }
      break;
    }
    case RWF_MSG:
      name.fnamelen = this->u.msg.namelen;
      name.fname    = this->u.msg.name;
      break;
    case RWF_MSG_KEY:
      name.fnamelen = this->u.msg_key.namelen;
      name.fname    = this->u.msg_key.name;
      break;
  }
  return 0;
}

int
RwfFieldIter::get_hint_reference( MDReference &mref ) noexcept
{
  mref.zero();
  return Err::NOT_FOUND;
}

int
RwfFieldIter::get_enum( MDReference &mref,  MDEnum &enu ) noexcept
{
  if ( mref.ftype == MD_ENUM ) {
    RwfMsg & msg = (RwfMsg &) this->iter_msg();
    if ( msg.dict != NULL && msg.base.type_id == RWF_FIELD_LIST ) {
      enu.val = get_uint<uint16_t>( mref );
      if ( this->iter_msg().dict->get_enum_text( this->u.field.fid, enu.val,
                                                 enu.disp, enu.disp_len ) )
        return 0;
    }
  }
  enu.zero();
  return Err::NO_ENUM;
}

int8_t
rai::md::rwf_to_md_decimal_hint( uint8_t hint ) noexcept
{
  static int8_t to_md[] = {
  /*  0 */ MD_DEC_LOGn10_10-4,/* ^10-14 */
  /*  1 */ MD_DEC_LOGn10_10-3,
  /*  2 */ MD_DEC_LOGn10_10-2,
  /*  3 */ MD_DEC_LOGn10_10-1,
  /*  4 */ MD_DEC_LOGn10_10,
  /*  5 */ MD_DEC_LOGn10_9,
  /*  6 */ MD_DEC_LOGn10_8,
  /*  7 */ MD_DEC_LOGn10_7,
  /*  8 */ MD_DEC_LOGn10_6,
  /*  9 */ MD_DEC_LOGn10_5,

  /* 10 */ MD_DEC_LOGn10_4,
  /* 11 */ MD_DEC_LOGn10_3,
  /* 12 */ MD_DEC_LOGn10_2,
  /* 13 */ MD_DEC_LOGn10_1,
  /* 14 */ MD_DEC_INTEGER,   /* n^0 */
  /* 15 */ MD_DEC_LOGp10_1,
  /* 16 */ MD_DEC_LOGp10_2,
  /* 17 */ MD_DEC_LOGp10_3,
  /* 18 */ MD_DEC_LOGp10_4,
  /* 19 */ MD_DEC_LOGp10_5,

  /* 20 */ MD_DEC_LOGp10_6,
  /* 21 */ MD_DEC_LOGp10_7,  /* ^10+7 */
  /* 22 */ MD_DEC_INTEGER,   /* n/1 */
  /* 23 */ MD_DEC_FRAC_2,
  /* 24 */ MD_DEC_FRAC_4,
  /* 25 */ MD_DEC_FRAC_8,
  /* 26 */ MD_DEC_FRAC_16,
  /* 27 */ MD_DEC_FRAC_32,
  /* 28 */ MD_DEC_FRAC_64,
  /* 29 */ MD_DEC_FRAC_128,

  /* 30 */ MD_DEC_FRAC_256,
  /* 31 */ MD_DEC_NULL,
  /* 32 */ MD_DEC_NULL,      /* blank */
  /* 33 */ MD_DEC_INF,
  /* 34 */ MD_DEC_NINF,
  /* 35 */ MD_DEC_NAN
  };
  return to_md[ hint < 36 ? hint : 35 ];
}

uint8_t
rai::md::md_to_rwf_decimal_hint( int8_t hint ) noexcept
{
  if ( hint >= 0 ) {
    if ( hint <= MD_DEC_INTEGER )
      return 14;
    if ( hint <= MD_DEC_FRAC_512 )
      return hint - MD_DEC_FRAC_2 + 23;
    return hint - MD_DEC_LOGp10_1 + 15;
  }
  /* hint < 0 */
  if ( hint > MD_DEC_LOGn10_1 ) {
    if ( hint == MD_DEC_INF )
      return 33;
    if ( hint == MD_DEC_NINF )
      return 34;
    return 35;
  }
  return hint - MD_DEC_LOGn10_1 + 13;
}

static inline bool
mref_to_value( MDReference &mref,  MDValue &val )
{
  if ( mref.fsize > 0 ) {
    bool sign_extend = false;
    size_t i = 1;
    uint64_t ival = mref.fptr[ 0 ];
    if ( ( ival & 0x80 ) != 0 ) {
      if ( mref.ftype == MD_DECIMAL || mref.ftype == MD_INT )
        sign_extend = true;
    }
    for ( ; i < mref.fsize; i++ )
      ival = ( ival << 8 ) | (uint64_t) mref.fptr[ i ];
    if ( sign_extend && i < 8 )
      ival |= ( ~(uint64_t) 0 ) << ( i * 8 );
    val.u64 = ival;
    return true;
  }
  return false;
}

static inline bool
to_digit( uint8_t c,  uint8_t &d )
{
  return (d = (c - '0')) <= 9;
}

static inline void
get_short( const uint8_t *buf,  size_t &i,  const uint8_t *eos,
           uint16_t &ival )
{
  uint8_t d;
  ival = 0;
  for ( i++; &buf[ i ] < eos && to_digit( buf[ i ], d ); i++ )
    ival = ( ival * 10 ) + d;
}

int
RwfFieldIter::get_reference( MDReference &mref ) noexcept
{
  mref.fentrysz = 0;
  mref.fentrytp = MD_NODATA;
  mref.fendian  = md_endian;

  if ( this->msg_fptr == NULL ) {
    RwfMsg & msg = (RwfMsg &) this->iter_msg();
    uint8_t * buf = &((uint8_t *) msg.msg_buf)[ this->data_start ];
    if ( msg.base.type_id == RWF_FIELD_LIST ) {
      if ( this->ftype == MD_NODATA )
        this->lookup_fid();
      if ( this->u.field.rwf_type == RWF_REAL_4RB ||
           this->u.field.rwf_type == RWF_REAL_8RB ) {
        mref.ftype = MD_DECIMAL;
        mref.fsize = this->field_end - this->data_start;
        mref.fptr  = buf;
        return this->get_real_ref( mref );
      }
    }
    mref.ftype = this->ftype;
    mref.fsize = this->field_end - this->data_start;
    mref.fptr  = buf;

    return this->decode_ref( mref );
  }
  mref.ftype = this->ftype;
  mref.fsize = this->fsize;
  mref.fptr  = this->msg_fptr;
  return 0;
}

int
RwfFieldIter::get_real_ref( MDReference &mref ) noexcept
{
  uint8_t * buf = mref.fptr;
  if ( mref.fsize == 1 ) {
    this->dec.ival = 0;
    this->dec.hint = MD_DEC_NULL;
  }
  else {
    this->dec.hint = rwf_to_md_decimal_hint( buf[ 0 ] & ~0xc0 );
    if ( mref.fsize == 2 )
      this->dec.ival = (int64_t) (char) buf[ 1 ];
    else if ( mref.fsize < 8 ) {
      if ( mref.fsize == 3 ) {
        this->dec.ival = get_u16<MD_BIG>( &buf[ 1 ] );
      }
      else if ( mref.fsize == 4 ) {
        this->dec.ival = get_u16<MD_BIG>( &buf[ 1 ] );
        this->dec.ival <<= 8;
        this->dec.ival |= buf[ 3 ];
      }
      else if ( mref.fsize >= 5 ) {
        this->dec.ival = get_u32<MD_BIG>( &buf[ 1 ] );
        if ( mref.fsize >= 6 ) {
          this->dec.ival <<= 8;
          this->dec.ival |= buf[ 5 ];
        }
        if ( mref.fsize == 7 ) {
          this->dec.ival <<= 8;
          this->dec.ival |= buf[ 6 ];
        }
      }
      int shft = ( mref.fsize - 1 ) * 8;
      if ( ( this->dec.ival >> ( shft - 1 ) ) != 0 )
        this->dec.ival |= ( ~(uint64_t) 0 ) << shft;
    }
    else if ( mref.fsize == 8 ) {
      this->dec.ival = (int64_t) get_u64<MD_BIG>( &buf[ 1 ] );
    }
  }
  mref.fptr    = (uint8_t *) (void *) &this->dec;
  mref.fsize   = sizeof( this->dec );
  mref.fendian = md_endian;
  return 0;
}

int
RwfMsg::get_array_ref( MDReference &mref,  size_t i,
                       MDReference &aref ) noexcept
{
  size_t num_entries = mref.fsize;
  uint8_t * buf      = mref.fptr;

  aref.zero();
  if ( mref.fentrysz != 0 ) {
    num_entries /= mref.fentrysz;
    if ( i < num_entries ) {
      aref.ftype   = mref.fentrytp;
      aref.fsize   = mref.fentrysz;
      aref.fendian = mref.fendian;
      aref.fptr    = &buf[ i * (size_t) mref.fentrysz ];
      return 0;
    }
    return Err::NOT_FOUND;
  }

  for ( ; ; i-- ) {
    size_t sz = buf[ 0 ];
    if ( i == 0 ) {
      aref.ftype   = mref.fentrytp;
      aref.fsize   = sz;
      aref.fendian = mref.fendian;
      aref.fptr    = &buf[ 1 ];
      return 0;
    }
    buf = &buf[ sz + 1 ];
  }
  return Err::NOT_FOUND;
}

int
RwfFieldIter::decode_ref( MDReference &mref ) noexcept
{
  uint8_t * buf = mref.fptr;
  switch ( mref.ftype ) {
    case MD_ARRAY: {
      uint8_t * eob = &buf[ mref.fsize ];
      if ( &buf[ 4 ] <= eob ) {
        uint32_t count = get_uint<uint16_t>( &buf[ 2 ], MD_BIG );
        if ( ! rwf_primitive_to_md_type( buf[ 0 ], mref.fentrytp ) )
          return Err::BAD_FIELD_TYPE;
        mref.fentrysz = buf[ 1 ];
        mref.fptr     = &buf[ 4 ];
        mref.fsize    = &buf[ mref.fsize ] - &buf[ 4 ];
        mref.fendian  = MD_BIG;
        if ( mref.fentrysz > 0 ) {
          size_t ar_size = count * mref.fentrysz;
          if ( mref.fsize > ar_size )
            mref.fsize = ar_size;
        }
        else {
          count = 0;
          for ( buf = &buf[ 4 ]; ; ) {
            if ( buf >= eob )
              break;
            size_t sz = buf[ 0 ];
            if ( &buf[ 1 + sz ] > eob )
              break;
            buf = &buf[ 1 + sz ];
            count++;
          }
          mref.fsize = count;
        }
      }
      else {
        mref.ftype = MD_OPAQUE;
      }
      break;
    }
    case MD_OPAQUE:
    case MD_MESSAGE:
      break;
    case MD_INT:
    case MD_UINT:
    case MD_ENUM:
    case MD_BOOLEAN:
      mref.fendian = MD_BIG;
      if ( ( mref.fsize & ( mref.fsize - 1 ) ) != 0 ) { /* not a power of 2 */
        if ( mref_to_value( mref, this->val ) ) {
          mref.fptr    = (uint8_t *) (void *) &this->val;
          mref.fendian = md_endian;
          do {
            mref.fsize++;
          } while ( ( mref.fsize & ( mref.fsize - 1 ) ) != 0 );
        }
      }
      break;

    case MD_TIME:
      if ( mref.fsize >= 2 ) {
        this->time.hour       = buf[ 0 ];
        this->time.minute     = buf[ 1 ];
        this->time.sec        = 0;
        this->time.fraction   = 0;
        this->time.resolution = MD_RES_MINUTES;

        if ( mref.fsize >= 3 ) {
          this->time.sec  = buf[ 2 ];
          this->time.resolution = MD_RES_SECONDS;

          if ( mref.fsize == 5 ) { /* millis */
            this->time.fraction = get_u16<MD_BIG>( &buf[ 3 ] );
            this->time.resolution = MD_RES_MILLISECS;
          }
          else if ( mref.fsize == 7 ) { /* micros */
            this->time.fraction = ( (uint32_t) get_u16<MD_BIG>( &buf[ 3 ] ) * 1000 ) +
                                  ( (uint32_t) get_u16<MD_BIG>( &buf[ 5 ] ) % 1000 );
            this->time.resolution = MD_RES_MICROSECS;
          }
          else if ( mref.fsize == 8 ) { /* nanos */
            this->time.fraction = ( (uint32_t) get_u16<MD_BIG>( &buf[ 3 ] ) * 1000000 );
            uint32_t micros = get_u16<MD_BIG>( &buf[ 5 ] ),
                     nanos  = buf[ 7 ];
            nanos += ( ( micros & 0x3800 ) >> 3 ); /* top 3 bits stored in micros */
            this->time.fraction += ( ( micros & 0x7ff ) * 1000 ) + nanos;
            this->time.resolution = MD_RES_NANOSECS;
          }
        }
        if ( this->time.hour == 0xff )
          this->time.resolution |= MD_RES_NULL;
      }
      else {
        this->time.zero();
        this->time.resolution = MD_RES_NULL;
      }
      mref.fptr    = (uint8_t *) (void *) &this->time;
      mref.fsize   = sizeof( this->time );
      mref.fendian = md_endian;
      break;

    case MD_DATE:
      if ( mref.fsize == 4 ) {
        this->date.day = buf[ 0 ];
        this->date.mon = buf[ 1 ];
        this->date.year = get_u16<MD_BIG>( &buf[ 2 ] );
      }
      else {
        this->date.zero();
      }
      mref.fptr    = (uint8_t *) (void *) &this->date;
      mref.fsize   = sizeof( this->date );
      mref.fendian = md_endian;
      break;

    case MD_DECIMAL:
      if ( mref.fsize >= 1 ) {
        this->dec.hint = rwf_to_md_decimal_hint( buf[ 0 ] );
        mref.fptr = &buf[ 1 ];
        mref.fsize -= 1;
        if ( mref_to_value( mref, this->val ) )
          this->dec.ival = this->val.i64;
        else
          this->dec.ival = 0;
      }
      else {
        this->dec.ival = 0;
        this->dec.hint = MD_DEC_NULL;
      }
      mref.fptr    = (uint8_t *) (void *) &this->dec;
      mref.fsize   = sizeof( this->dec );
      mref.fendian = md_endian;
      break;

    case MD_REAL:
      mref.ftype   = MD_REAL;
      mref.fendian = MD_BIG;
      break;

    case MD_PARTIAL:
      if ( mref.fsize > 3 ) {
        enum {
          CSI1 = 0x5bU, /* '[' control sequence inducer part */
          HPA  = 0x60U, /* '`' -- the horizontal position adjust char */
          REP  = 0x62U, /* 'b' -- repitition char */
          ESC  = 0x1bU, /* <ESC> <CSI1> n ` -- for partials */
          CSI2 = 0x1bU | 0x80U /* <CSI2> n ` -- second form of partials */
        };

        if ( ( buf[ 0 ] & 0x7fU ) == ESC ) {
          uint8_t *eos = &buf[ mref.fsize ];
          size_t off = 0;
          if ( buf[ 0 ] == CSI2 ) { /* 0x80 | ESC n ` */
            get_short( buf, off, eos, this->position );
          }
          else if ( buf[ 1 ] == CSI1 ) { /* ESC [ n ` */
            get_short( &buf[ 1 ], off, eos, this->position );
            off++;
          }
          if ( off > 0 && &buf[ off ] < eos && buf[ off ] == HPA ) {
            mref.fptr     = &buf[ off + 1 ];
            mref.fsize   -= off + 1;
            mref.fentrysz = (uint8_t) this->position;
            return 0;
          }
        }
      }
      /* FALLTHRU */
    case MD_STRING:
      if ( mref.fsize == 0 ) {
        static uint8_t nullchar[ 1 ] = { 0 };
        mref.fsize = 1;
        mref.fptr  = nullchar;
      }
      break;

    default:
      mref.ftype = MD_OPAQUE;
      break;
  }
  return 0;
}

int
RwfFieldIter::find( const char *name,  size_t name_len,
                    MDReference &mref ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  MDLookup by( name, name_len );
  int status = Err::NOT_FOUND;
  bool is_field_list = ( msg.base.type_id == RWF_FIELD_LIST );
  by.fid = 0;
  if ( is_field_list ) {
    if ( msg.dict == NULL || ! msg.dict->get( by ) )
      return Err::NOT_FOUND;
    if ( (status = this->first()) == 0 ) {
      do {
        if ( this->u.field.fid == by.fid )
          return this->get_reference( mref );
      } while ( (status = this->next()) == 0 );
    }
  }
  else {
    if ( (status = this->first()) == 0 ) {
      do {
        MDName nm;
        if ( this->get_name( nm ) == 0 &&
             MDDict::dict_equals( name, name_len, nm.fname, nm.fnamelen ) )
          return this->get_reference( mref );
      } while ( (status = this->next()) == 0 );
    }
  }
  return status;
}

int
RwfFieldIter::first( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  int k;

  this->field_end = msg.msg_end;
  this->field_index = 0;
  this->msg_fptr  = NULL;

  switch ( msg.base.type_id ) {
    case RWF_FIELD_LIST:
      if ( msg.fields.set_start != 0 )
        msg.get_field_defn_db();

      k = msg.fields.next_kind( 0 );
      if ( k == RWF_FIRST_ENTRY ) {
        this->field_start = msg.fields.data_start;
        return this->unpack_field_list_entry();
      }
      if ( k == RWF_FIRST_FIELD_DEFN ) {
        this->field_start = msg.fields.set_start;
        return this->unpack_field_list_defn();
      }
      break;
    case RWF_MAP:
      if ( ! msg.map.has_next( 0 ) )
        break;
      return this->unpack_map_entry();
    case RWF_ELEMENT_LIST:
      if ( msg.elist.set_start != 0 )
        msg.get_field_defn_db();

      k = msg.elist.next_kind( 0 );
      if ( k == RWF_FIRST_ENTRY ) {
        this->field_start = msg.elist.data_start;
        return this->unpack_element_list_entry();
      }
      if ( k == RWF_FIRST_FIELD_DEFN ) {
        this->field_start = msg.elist.set_start;
        return this->unpack_element_list_defn();
      }
      break;
    case RWF_FILTER_LIST:
      this->field_start = msg.flist.data_start;
      if ( ! msg.flist.has_next( 0 ) )
        break;
      return this->unpack_filter_list_entry();
    case RWF_SERIES:
      if ( ! msg.series.has_next( 0 ) )
        break;
      return this->unpack_series_entry();
    case RWF_VECTOR:
      if ( ! msg.vector.has_next( 0 ) )
        break;
      return this->unpack_vector_entry();
    case RWF_MSG:
      return this->unpack_message_entry();
    case RWF_MSG_KEY:
      return this->unpack_msg_key_entry();
    default:
      break;
  }
  this->field_end = this->field_start;
  return Err::NOT_FOUND;
}

int
RwfFieldIter::next( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg();
  int k;

  this->field_start = this->field_end;
  this->field_end   = msg.msg_end;
  this->field_index++;

  switch ( msg.base.type_id ) {
    case RWF_FIELD_LIST:
      k = msg.fields.next_kind( this->field_index );
      if ( ( k & RWF_NORMAL_ENTRY ) != 0 ) {
        if ( k == RWF_FIRST_ENTRY )
          this->field_start = msg.fields.data_start;
        return this->unpack_field_list_entry();
      }
      if ( ( k & RWF_FIELD_DEFN_ENTRY ) != 0 ) {
        if ( k == RWF_FIRST_FIELD_DEFN )
          this->field_start = msg.fields.set_start;
        return this->unpack_field_list_defn();
      }
      break;
    case RWF_MAP:
      if ( ! msg.map.has_next( this->field_index ) )
        break;
      return this->unpack_map_entry();
    case RWF_ELEMENT_LIST:
      k = msg.elist.next_kind( this->field_index );
      if ( (k & RWF_NORMAL_ENTRY) != 0 ) {
        if ( k == RWF_FIRST_ENTRY )
          this->field_start = msg.elist.data_start;
        return this->unpack_element_list_entry();
      }
      if ( (k & RWF_FIELD_DEFN_ENTRY) != 0 ) {
        if ( k == RWF_FIRST_FIELD_DEFN )
          this->field_start = msg.elist.set_start;
        return this->unpack_element_list_defn();
      }
      break;
    case RWF_FILTER_LIST:
      if ( ! msg.flist.has_next( this->field_index ) )
        break;
      return this->unpack_filter_list_entry();
    case RWF_SERIES:
      if ( ! msg.series.has_next( this->field_index ) )
        break;
      return this->unpack_series_entry();
    case RWF_VECTOR:
      if ( ! msg.vector.has_next( this->field_index ) )
        break;
      return this->unpack_vector_entry();
    case RWF_MSG:
      return this->unpack_message_entry();
    case RWF_MSG_KEY:
      return this->unpack_msg_key_entry();
    default:
      break;
  }

  return Err::NOT_FOUND;
}
