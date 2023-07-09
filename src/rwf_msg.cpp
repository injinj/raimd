#include <stdio.h>
#include <math.h>
#define DEFINE_RWF_MSG_DECODER
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

const char * RwfMsg::get_proto_string( void ) noexcept { return "RWF"; }
uint32_t     RwfMsg::get_type_id( void ) noexcept      { return RWF_FIELD_LIST_TYPE_ID; }

static MDMatch rwf_match[] = {
  {0,   4,1,0, { 0x25, 0xcd, 0xab, 0xca }, { RWF_FIELD_LIST_TYPE_ID },
                                                    RwfMsg::is_rwf_field_list,   (md_msg_unpack_f) RwfMsg::unpack_field_list },
  {0,0xff,1,0, { 0 }, { RWF_MAP_TYPE_ID },          RwfMsg::is_rwf_map,          (md_msg_unpack_f) RwfMsg::unpack_map },
  {0,0xff,1,0, { 0 }, { RWF_ELEMENT_LIST_TYPE_ID }, RwfMsg::is_rwf_element_list, (md_msg_unpack_f) RwfMsg::unpack_element_list },
  {0,0xff,1,0, { 0 }, { RWF_FILTER_LIST_TYPE_ID },  RwfMsg::is_rwf_filter_list,  (md_msg_unpack_f) RwfMsg::unpack_filter_list },
  {0,0xff,1,0, { 0 }, { RWF_SERIES_TYPE_ID },       RwfMsg::is_rwf_series,       (md_msg_unpack_f) RwfMsg::unpack_series },
  {0,0xff,1,0, { 0 }, { RWF_VECTOR_TYPE_ID },       RwfMsg::is_rwf_vector,       (md_msg_unpack_f) RwfMsg::unpack_vector },
  {0,0xff,1,0, { 0 }, { RWF_MSG_TYPE_ID },          RwfMsg::is_rwf_message,      (md_msg_unpack_f) RwfMsg::unpack_message },
  {0,0xff,1,0, { 0 }, { RWF_MSG_KEY_TYPE_ID },      RwfMsg::is_rwf_msg_key,      (md_msg_unpack_f) RwfMsg::unpack_msg_key }
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

static MDType
rwf_primitive_to_md_type( RWF_type type ) noexcept
{
  switch ( type ) {
    case RWF_INT:           return MD_INT;
    case RWF_UINT:          return MD_UINT;
    case RWF_FLOAT:
    case RWF_DOUBLE:        return MD_REAL;
    case RWF_REAL:          return MD_DECIMAL;
    case RWF_DATE:          return MD_DATE;
    case RWF_TIME:          return MD_TIME;
    case RWF_DATETIME:      return MD_DATETIME;
    case RWF_ENUM:          return MD_ENUM;
    case RWF_ASCII_STRING:  return MD_STRING;
    case RWF_UTF8_STRING:   return MD_STRING;
    case RWF_RMTES_STRING:  return MD_STRING;

    default:
    case RWF_ARRAY:
    case RWF_BUFFER:
    case RWF_QOS:
      return MD_OPAQUE;
  }
}

uint32_t
RwfBase::parse_type( DecodeHdr &dec ) noexcept
{
  uint32_t t = 0;
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
  DecodeHdr dec( bb, off, end );

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
  DecodeHdr hdr( bb, off, end );
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
  RwfMsg & msg = (RwfMsg &) this->iter_msg;
  if ( ! msg.msg.ref_iter( this->field_idx, *this ) )
    return Err::NOT_FOUND;
  return 0;
}

int
RwfFieldIter::unpack_msg_key_entry( void ) noexcept
{
  RwfMsg & msg = (RwfMsg &) this->iter_msg;
  if ( ! msg.msg_key.ref_iter( this->field_idx, *this ) )
    return Err::NOT_FOUND;
  return 0;
}

int
RwfFieldListHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  DecodeHdr dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_FIELD_LIST )
    return Err::BAD_HEADER;

  this->type_id   = RWF_FIELD_LIST;
  this->flags     = 0;
  this->dict_id   = 1;
  this->flist     = 0;
  this->field_cnt = 0;

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
  /* no decoders for these */
  if ( ( this->flags & ( HAS_SET_DATA | HAS_SET_ID ) ) != 0 )
    return Err::BAD_HEADER;
  if ( ( this->flags & HAS_STANDARD_DATA ) != 0 )
    dec.u16( this->field_cnt );
  this->data_start = dec.offset( off );
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_field_list_entry( void ) noexcept
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf,
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
RwfMapHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  DecodeHdr dec( bb, off, end );
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

  if ( ( this->flags & HAS_SUMMARY_DATA ) != 0 ) {
    dec.u15( this->summary_size );
    this->summary_start = dec.offset( off );
    dec.incr( this->summary_size );
  }
  if ( ( this->flags & HAS_COUNT_HINT ) != 0 )
    dec.u30( this->hint_cnt );
  if ( ( this->flags & HAS_SET_DEFS ) != 0 )
    return Err::BAD_HEADER;

  dec.u16( this->entry_cnt );
  this->data_start = dec.offset( off );
  this->key_ftype  = rwf_primitive_to_md_type( (RWF_type) this->key_type );

  if ( ! dec.ok || ! is_rwf_container( (RWF_type) this->container_type ) ||
       ! is_rwf_primitive( (RWF_type) this->key_type ) )
    return Err::BAD_HEADER;
  return 0;
}

template<class Hdr>
static inline size_t
unpack_summary( RwfFieldIter &iter,  Hdr &hdr )
{
  if ( iter.field_idx == 0 ) {
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
  if ( iter.field_idx == 1 ) {
    if ( hdr.summary_size != 0 )
      return hdr.data_start;
  }
  return iter.field_end;
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
  RwfMsg     & msg = (RwfMsg &) this->iter_msg;
  uint8_t    * buf = (uint8_t *) msg.msg_buf,
             * eob = &buf[ msg.msg_end ];
  size_t       i, sz, keyoff;
  /* first time, return summary message, if any */

  if ( (i = unpack_summary( *this, msg.map )) == 0 )
    return 0;

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
  DecodeHdr dec( bb, off, end );
  uint32_t t = this->parse_type( dec );

  if ( t != 0 && t != RWF_ELEMENT_LIST )
    return Err::BAD_HEADER;

  this->type_id  = RWF_ELEMENT_LIST;
  this->flags    = 0;
  this->list_num = 0;
  this->set_id   = 0;
  this->item_cnt = 0;

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
  if ( ( this->flags & ( HAS_SET_DATA | HAS_SET_ID ) ) != 0 )
    return Err::BAD_HEADER;
  if ( ( this->flags & HAS_STANDARD_DATA ) != 0 )
    dec.u16( this->item_cnt );
  this->data_start = dec.offset( off );
  if ( ! dec.ok )
    return Err::BAD_HEADER;
  return 0;
}

int
RwfFieldIter::unpack_element_list_entry( void ) noexcept
{
  RwfElementListIter & it = this->u.elist;
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf,
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
    this->ftype = rwf_primitive_to_md_type( (RWF_type) it.type );
  }
  this->data_start = i;
  this->field_end  = this->data_start + this->fsize;
  if ( &buf[ this->field_end ] > eob )
    return Err::BAD_FIELD_BOUNDS;
  return 0;
}

int
RwfFilterListHdr::parse( const void *bb,  size_t off,  size_t end ) noexcept
{
  DecodeHdr dec( bb, off, end );
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
  RwfMsg     & msg    = (RwfMsg &) this->iter_msg;
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
  DecodeHdr dec( bb, off, end );
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

  if ( ( this->flags & HAS_SET_DEFS ) != 0 )
    return Err::BAD_HEADER;

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
  RwfMsg  & msg = (RwfMsg &) this->iter_msg;
  uint8_t * buf = (uint8_t *) msg.msg_buf,
          * eob = &buf[ msg.msg_end ];
  size_t    i, sz;

  if ( (i = unpack_summary( *this, msg.series )) == 0 )
    return 0;

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
  DecodeHdr dec( bb, off, end );
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

  if ( ( this->flags & HAS_SET_DEFS ) != 0 )
    return Err::BAD_HEADER;

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
  RwfMsg     & msg = (RwfMsg &) this->iter_msg;
  uint8_t    * buf = (uint8_t *) msg.msg_buf,
             * eob = &buf[ msg.msg_end ];
  size_t       i, sz;

  if ( (i = unpack_summary( *this, msg.vector )) == 0 )
    return 0;

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
    if ( hdr.type_id == RWF_MAP )
      return RwfMsg::unpack_map( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_ELEMENT_LIST )
      return RwfMsg::unpack_element_list( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_FILTER_LIST )
      return RwfMsg::unpack_filter_list( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_SERIES )
      return RwfMsg::unpack_series( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_VECTOR )
      return RwfMsg::unpack_vector( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_MSG )
      return RwfMsg::unpack_message( bb, off, end, h, d, m );
    if ( hdr.type_id == RWF_MSG_KEY )
      return RwfMsg::unpack_msg_key( bb, off, end, h, d, m );
    return NULL;
  }
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( RwfMsg ), &ptr );
  for ( ; d != NULL; d = d->next )
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
  for ( ; d != NULL; d = d->next )
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

int
RwfMsg::get_sub_msg( MDReference &mref,  MDMsg *&msg,
                     MDFieldIter *iter ) noexcept
{
  uint8_t * bb    = (uint8_t *) this->msg_buf;
  size_t    start = (size_t) ( mref.fptr - bb );
  RWF_type  type  = RWF_NO_DATA;

  if ( this->base.type_id == RWF_MAP )
    type = (RWF_type) this->map.container_type;
  else if ( this->base.type_id == RWF_FILTER_LIST ) {
    if ( iter != NULL )
      type = (RWF_type) ((RwfFieldIter *) iter)->u.flist.type;
  }
  else if ( this->base.type_id == RWF_SERIES )
    type = (RWF_type) this->series.container_type;
  else if ( this->base.type_id == RWF_VECTOR )
    type = (RWF_type) this->vector.container_type;
  else if ( this->base.type_id == RWF_MSG ) {
    type = (RWF_type) this->msg.container_type;
    if ( mref.fptr == (uint8_t *) this->msg.msg_key.data ||
         mref.fptr == (uint8_t *) this->msg.req_msg_key.data )
      type = RWF_MSG_KEY;
  }
  else if ( this->base.type_id == RWF_MSG_KEY )
    type =  (RWF_type) this->msg_key.attrib.container_type;

  switch ( type ) {
    case RWF_FIELD_LIST:
      msg = RwfMsg::unpack_field_list( bb, start, start + mref.fsize, 0,
                                       this->dict, *this->mem );
      break;
    case RWF_ELEMENT_LIST:
      msg = RwfMsg::unpack_element_list( bb, start, start + mref.fsize, 0,
                                         this->dict, *this->mem );
      break;
    case RWF_FILTER_LIST:
      msg = RwfMsg::unpack_filter_list( bb, start, start + mref.fsize, 0,
                                        this->dict, *this->mem );
      break;
    case RWF_VECTOR:
      msg = RwfMsg::unpack_vector( bb, start, start + mref.fsize, 0,
                                   this->dict, *this->mem );
      break;
    case RWF_MAP:
      msg = RwfMsg::unpack_map( bb, start, start + mref.fsize, 0,
                                this->dict, *this->mem );
      break;
    case RWF_SERIES:
      msg = RwfMsg::unpack_series( bb, start, start + mref.fsize, 0,
                                   this->dict, *this->mem );
      break;
    case RWF_MSG:
      msg = RwfMsg::unpack_message( bb, start, start + mref.fsize, 0,
                                    this->dict, *this->mem );
      break;
    case RWF_MSG_KEY:
      msg = RwfMsg::unpack_msg_key( bb, start, start + mref.fsize, 0,
                                    this->dict, *this->mem );
      break;
    default:
      msg = NULL;
      return Err::BAD_SUB_MSG;
  }
  return 0;
}

void
RwfFieldIter::lookup_fid( void ) noexcept
{
  if ( this->ftype == MD_NODATA ) {
    if ( this->iter_msg.dict != NULL ) {
      MDLookup by( this->u.field.fid );
      if ( this->iter_msg.dict->lookup( by ) ) {
        this->ftype = by.ftype;
        this->fsize = by.fsize;
        this->u.field.fname    = by.fname;
        this->u.field.fnamelen = by.fname_len;
      }
    }
    if ( this->ftype == MD_NODATA ) { /* undefined fid or no dictionary */
      this->ftype    = MD_OPAQUE;
      this->u.field.fname    = NULL;
      this->u.field.fnamelen = 0;
    }
  }
}

int
RwfFieldIter::get_name( MDName &name ) noexcept
{
  static const char nul_str[] = "-nul", upd_str[] = "-upd", set_str[] = "-set",
                    clr_str[] = "-clr", ins_str[] = "-ins", del_str[] = "-del",
                    add_str[] = "-add";
  RwfMsg & msg = (RwfMsg &) this->iter_msg;
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
      if ( this->field_idx == 0 && msg.vector.summary_size != 0 ) {
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
      if ( this->field_idx == 0 && msg.map.summary_size != 0 ) {
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
      if ( this->field_idx == 0 && msg.series.summary_size != 0 ) {
        name.fnamelen = sizeof( summary_str );
        name.fname    = summary_str;
      }
      else {
        size_t idx     = this->field_idx + ( msg.series.summary_size == 0 ? 1 : 0 );
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
    RwfMsg & msg = (RwfMsg &) this->iter_msg;
    if ( msg.dict != NULL && msg.base.type_id == RWF_FIELD_LIST ) {
      enu.val = get_uint<uint16_t>( mref );
      if ( this->iter_msg.dict->get_enum_text( this->u.field.fid, enu.val,
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
    MD_DEC_LOGn10_10-4,  /* ^10-14 */
    MD_DEC_LOGn10_10-3,
    MD_DEC_LOGn10_10-2,
    MD_DEC_LOGn10_10-1,
    MD_DEC_LOGn10_10,
    MD_DEC_LOGn10_9,
    MD_DEC_LOGn10_8,
    MD_DEC_LOGn10_7,
    MD_DEC_LOGn10_6,
    MD_DEC_LOGn10_5,
    MD_DEC_LOGn10_4,
    MD_DEC_LOGn10_3,
    MD_DEC_LOGn10_2,
    MD_DEC_LOGn10_1,
    MD_DEC_INTEGER,   /* 14 */
    MD_DEC_LOGp10_1,
    MD_DEC_LOGp10_2,
    MD_DEC_LOGp10_3,
    MD_DEC_LOGp10_4,
    MD_DEC_LOGp10_5,
    MD_DEC_LOGp10_6,
    MD_DEC_LOGp10_7,  /* ^10+7 */
    MD_DEC_INTEGER,   /* 22 */
    MD_DEC_FRAC_2,
    MD_DEC_FRAC_4,
    MD_DEC_FRAC_8,
    MD_DEC_FRAC_16,
    MD_DEC_FRAC_32,
    MD_DEC_FRAC_64,
    MD_DEC_FRAC_128,
    MD_DEC_FRAC_256,
    MD_DEC_INF,       /* 33 */
    MD_DEC_NINF,      /* 34 */
    MD_DEC_NAN        /* 35 */
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
  mref.fendian  = MD_BIG;
  mref.fentrysz = 0;
  mref.fentrytp = MD_NODATA;
  mref.fendian  = md_endian;

  if ( this->msg_fptr == NULL ) {
    RwfMsg & msg = (RwfMsg &) this->iter_msg;
    uint8_t * buf = &((uint8_t *) msg.msg_buf)[ this->data_start ];
    if ( this->ftype == MD_NODATA && msg.base.type_id == RWF_FIELD_LIST )
      this->lookup_fid();
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
RwfFieldIter::decode_ref( MDReference &mref ) noexcept
{
  uint8_t * buf = mref.fptr;
  switch ( mref.ftype ) {
    case MD_OPAQUE:
    case MD_MESSAGE:
      break;
    case MD_INT:
    case MD_UINT:
    case MD_ENUM:
    case MD_BOOLEAN:
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
  RwfMsg & msg = (RwfMsg &) this->iter_msg;
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
  RwfMsg & msg = (RwfMsg &) this->iter_msg;
  this->field_idx = 0;
  this->field_end = msg.msg_end;
  this->msg_fptr  = NULL;

  switch ( msg.base.type_id ) {
    case RWF_FIELD_LIST:
      this->field_start = msg.fields.data_start;
      if ( msg.fields.iter_cnt() == 0 )
        break;
      return this->unpack_field_list_entry();
    case RWF_MAP:
      if ( msg.map.iter_cnt() == 0 )
        break;
      return this->unpack_map_entry();
    case RWF_ELEMENT_LIST:
      this->field_start = msg.elist.data_start;
      if ( msg.elist.iter_cnt() == 0 )
        break;
      return this->unpack_element_list_entry();
    case RWF_FILTER_LIST:
      this->field_start = msg.flist.data_start;
      if ( msg.flist.iter_cnt() == 0 )
        break;
      return this->unpack_filter_list_entry();
    case RWF_SERIES:
      if ( msg.series.iter_cnt() == 0 )
        break;
      return this->unpack_series_entry();
    case RWF_VECTOR:
      if ( msg.vector.iter_cnt() == 0 )
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
  RwfMsg & msg = (RwfMsg &) this->iter_msg;

  switch ( msg.base.type_id ) {
    case RWF_FIELD_LIST:
      if ( ++this->field_idx >= msg.fields.iter_cnt() )
        break;
      this->field_start = this->field_end;
      this->field_end   = msg.msg_end;
      return this->unpack_field_list_entry();
    case RWF_MAP:
      if ( ++this->field_idx >= msg.map.iter_cnt() )
        break;
      return this->unpack_map_entry();
    case RWF_ELEMENT_LIST:
      if ( ++this->field_idx >= msg.elist.iter_cnt() )
        break;
      this->field_start = this->field_end;
      this->field_end   = msg.msg_end;
      return this->unpack_element_list_entry();
    case RWF_FILTER_LIST:
      if ( ++this->field_idx >= msg.flist.iter_cnt() )
        break;
      this->field_start = this->field_end;
      this->field_end   = msg.msg_end;
      return this->unpack_filter_list_entry();
    case RWF_SERIES:
      if ( ++this->field_idx >= msg.series.iter_cnt() )
        break;
      return this->unpack_series_entry();
    case RWF_VECTOR:
      if ( ++this->field_idx >= msg.vector.iter_cnt() )
        break;
      return this->unpack_vector_entry();
    case RWF_MSG:
      this->field_idx++;
      return this->unpack_message_entry();
    case RWF_MSG_KEY:
      this->field_idx++;
      return this->unpack_msg_key_entry();
    default:
      break;
  }

  return Err::NOT_FOUND;
}
