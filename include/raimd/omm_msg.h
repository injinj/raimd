#ifndef __rai_raimd__omm_msg_h__
#define __rai_raimd__omm_msg_h__

#include <raimd/omm_flags.h>

namespace rai {
namespace md {

static const uint8_t RWF_CONTAINER_BASE = 128;

template <class T>
struct DecodeT {
  const uint8_t * buf,
                * eob,
                * start;
  bool            ok;
  DecodeT( const uint8_t *b,  const uint8_t *e )
    : buf( b ), eob( e ), start( b ), ok( true ) {}
  DecodeT( const uint8_t *s,  const uint8_t *b,  const uint8_t *e,  bool o )
    : buf( b ), eob( e ), start( s ), ok( o ) {}

  size_t len( void ) const {
    if ( this->eob >= this->buf )
      return this->eob - this->buf;
    return 0;
  }
  size_t offset( size_t n ) const {
    return n + ( this->buf - this->start );
  }
  void incr( size_t sz ) {
    this->buf = &this->buf[ sz ];
  }
  uint32_t peek_u32( size_t i ) const {
    if ( &this->buf[ ( i + 1 ) * 4 ] <= this->eob )
      return get_u32<MD_BIG>( &this->buf[ i * 4 ] );
    return 0;
  }
  template <class I>
  T & u8( I &x ) {
    x = 0;
    if ( (this->ok &= ( &this->buf[ 1 ] <= this->eob )) == true )
      x = this->buf[ 0 ];
    this->buf = &this->buf[ 1 ];
    return (T &) *this;
  }
  template <class I>
  T & u16( I &x ) {
    x = 0;
    if ( (this->ok &= ( &this->buf[ 2 ] <= this->eob )) == true )
      x = get_u16<MD_BIG>( this->buf );
    this->buf = &this->buf[ 2 ];
    return (T &) *this;
  }
  template <class I>
  T & u15( I &x ) {
    x = 0;
    size_t sz = get_u15_prefix( this->buf, this->eob, x );
    this->ok &= ( sz != 0 );
    this->buf = &this->buf[ sz ];
    return (T &) *this;
  }
  T & u30( uint32_t &x ) {
    x = 0;
    size_t sz = get_u30_prefix( this->buf, this->eob, x );
    this->ok &= ( sz != 0 );
    this->buf = &this->buf[ sz ];
    return (T &) *this;
  }
  T & z16( uint16_t &x ) {
    x = 0;
    size_t sz = get_fe_prefix( this->buf, this->eob, x );
    this->ok &= ( sz != 0 );
    this->buf = &this->buf[ sz ];
    return (T &) *this;
  }
  T & u32( uint32_t &x ) {
    x = 0;
    if ( (this->ok &= ( &this->buf[ 4 ] <= this->eob )) == true )
      x = get_u32<MD_BIG>( this->buf );
    this->buf = &this->buf[ 4 ];
    return (T &) *this;
  }
  template <class P>
  T & p( P *&ptr,  size_t len ) {
    this->ok &= ( &this->buf[ len ] <= this->eob );
    ptr = (P *) this->buf;
    this->buf = &this->buf[ len ];
    return (T &) *this;
  }
};

struct DecodeHdr : public DecodeT<DecodeHdr> {
  DecodeHdr( const uint8_t *b,  const uint8_t *e ) :
    DecodeT( b, e ) {}
  DecodeHdr( const void *b,  size_t off,  size_t end ) :
    DecodeT( &((uint8_t *) b)[ off ], &((uint8_t *) b)[ end ] ) {}
};

struct RwfBase {
  uint32_t type_id; /* RWF_TYPE_ID */
  uint32_t parse_type( DecodeHdr &dec ) noexcept;
};

struct RwfPostInfo {
  uint32_t user_addr,
           user_id;
};

struct RwfGroup {
  void  * buf;
  uint8_t len;
};

struct RwfExtended {
  void  * buf;
  uint8_t len;
};

struct RwfPerm {
  void   * buf;
  uint16_t len;
};

struct RwfConfInfo {
  uint16_t count,
           time;
};

struct RwfText {
  const char * buf;
  uint16_t     len;
};

struct RwfState {
  uint8_t data_state,
          stream_state,
          code;
  RwfText text;
};

struct RwfQos {
  uint8_t  timeliness,
           rate,
           dynamic;
  uint16_t time_info,
           rate_info;
};

struct RwfAttrib {
  uint8_t  container_type;
  uint16_t len;
  void   * data;
};

struct RwfFieldIter;
struct RwfMsgKey : public RwfBase {
  enum {
    HAS_SERVICE_ID = 1,
    HAS_NAME       = 2,
    HAS_NAME_TYPE  = 4,
    HAS_FILTER     = 8,
    HAS_IDENTIFIER = 16,
    HAS_ATTRIB     = 32
  };
  uint64_t     flags;
  void       * data;
  uint16_t     data_size,
               key_flags,
               service_id,
               name_len;
  /* login name type: 1 = user , 2 = email, 3 = token, 4 = cookie, 5 = authn */
  /* other domains:   0 = unspec, 1 = RIC, 2 = contributor */
  uint8_t      name_type;
  const char * name;
  uint32_t     filter,
               identifier;
  RwfAttrib    attrib;
  bool test( RwfMsgSerial s ) const {
    return ( ( (uint64_t) 1 << s ) & this->flags ) != 0;
  }
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool ref_iter( size_t which,  RwfFieldIter &iter ) noexcept;
};

struct RwfPriority {
  uint8_t  clas;
  uint16_t count;
};

struct RwfFieldIter;
struct RwfMsgHdr : public RwfBase {
  uint64_t    flags;
  uint16_t    header_size;    /*Req|Rfr|Sta|Upd|Cls|Ack|Gen|Pos*/
  uint8_t     msg_class,      /* . | . | . | . | . | . | . | . */
              domain_type;    /* . | . | . | . | . | . | . | . */
  uint32_t    stream_id;      /* . | . | . | . | . | . | . | . */
  uint16_t    msg_flags;      /* . | . | . | . | . | . | . | . */
  uint8_t     container_type, /* . | . | . | . | . | . | . | . */
              update_type,    /*   |   |   | . |   |   |   |   */
              nak_code;       /*   |   |   |   |   | ? |   |   */
  RwfText     text;           /*   |   |   |   |   | ? |   |   */
  uint32_t    seq_num,        /*   | ? |   | ? |   | ? | ? | ? */
              second_seq_num, /*   |   |   |   |   |   | ? |   */
              post_id,        /*   |   |   |   |   |   |   | ? */
              ack_id;         /*   |   |   |   |   | . |   |   */
  RwfState    state;          /*   | . | ? |   |   |   |   |   */
  RwfGroup    group_id;       /*   | . | ? |   |   |   |   |   */
  RwfConfInfo conf_info;      /*   |   |   | ? |   |   |   |   */
  RwfPriority priority;       /* ? |   |   |   |   |   |   |   */
  RwfPerm     perm;           /*   | ? | ? | ? |   |   | ? | ? */
  RwfQos      qos,            /* ? | ? |   |   |   |   |   |   */
              worst_qos;      /* ? |   |   |   |   |   |   |   */
  RwfMsgKey   msg_key;        /* . | ? | ? | ? |   | ? | ? | ? */
  RwfExtended extended;       /* ? | ? | ? | ? | ? | ? | ? | ? */
  RwfPostInfo post_user;      /*   | ? | ? | ? |   |   |   | . */
  RwfMsgKey   req_msg_key;    /*   | ? | ? |   |   |   | ? |   */
  uint16_t    part_num,       /*   | ? |   |   |   |   | ? | ? */
              post_rights;    /*   |   |   |   |   |   |   | ? */
  size_t      data_start,
              data_end;
  int parse( const void *bb,  size_t off,  size_t end ) noexcept;
  bool test( RwfMsgSerial s ) const {
    return ( ( (uint64_t) 1 << s ) & this->flags ) != 0;
  }
  bool ref_iter( size_t which,  RwfFieldIter &iter ) noexcept;
};

extern const uint64_t rwf_msg_always_present[ RWF_MSG_CLASS_COUNT ],
                      rwf_msg_maybe_present[ RWF_MSG_CLASS_COUNT ],
                      rwf_msg_flag_only[ RWF_MSG_CLASS_COUNT ];
#ifdef DEFINE_RWF_MSG_DECODER
#define bit( x ) ( (uint64_t) 1 << x )
#define bit_2( a, b )                          bit( a ) | bit( b )
#define bit_3( a, b, c )                       bit( a ) | bit( b ) | bit( c )
#define bit_4( a, b, c, d )                    bit_3( a, b, c ) | bit( d )
#define bit_6( a, b, c, d, e, f )              bit_3( a, b, c ) | bit_3( d, e, f )
#define bit_7( a, b, c, d, e, f, g )           bit_6( a, b, c, d, e, f ) | bit( g )
#define bit_8( a, b, c, d, e, f, g, h )        bit_7( a, b, c, d, e, f, g ) | bit( h )
#define bit_9( a, b, c, d, e, f, g, h, i )     bit_8( a, b, c, d, e, f, g, h ) | bit( i )
#define bit_10( a, b, c, d, e, f, g, h, i, j ) bit_9( a, b, c, d, e, f, g, h, i ) | bit( j )
#define bit_13( a, b, c, d, e, f, g, h, i, j, k, l, m ) \
                                               bit_10( a, b, c, d, e, f, g, h, i, j ) | bit_3( k, l, m )
#define bit_14( a, b, c, d, e, f, g, h, i, j, k, l, m, n ) \
                                               bit_13( a, b, c, d, e, f, g, h, i, j, k, l, m ) | bit( n )

const uint64_t rwf_msg_always_present[ RWF_MSG_CLASS_COUNT ] = {
/*MSG_KEY_CLASS     */ 0,
/*REQUEST_MSG_CLASS */ bit( X_HAS_MSG_KEY ),
/*REFRESH_MSG_CLASS */ bit( X_HAS_GROUP_ID ) | bit( X_HAS_STATE ),
/*STATUS_MSG_CLASS  */ 0,
/*UPDATE_MSG_CLASS  */ bit( X_HAS_UPDATE_TYPE ),
/*CLOSE_MSG_CLASS   */ 0,
/*ACK_MSG_CLASS     */ bit( X_HAS_ACK_ID ),
/*GENERIC_MSG_CLASS */ 0,
/*POST_MSG_CLASS    */ bit( X_HAS_POST_USER_INFO )
};
const uint64_t rwf_msg_maybe_present[ RWF_MSG_CLASS_COUNT ] = {
/*MSG_KEY_CLASS */     bit_6 ( X_HAS_SERVICE_ID, X_HAS_NAME, X_HAS_NAME_TYPE, X_HAS_FILTER, X_HAS_IDENTIFIER, X_HAS_ATTRIB ),
/*REQUEST_MSG_CLASS */ bit_13( X_HAS_EXTENDED_HEADER, X_HAS_PRIORITY, X_STREAMING, X_MSG_KEY_IN_UPDATES, X_CONF_INFO_IN_UPDATES, X_NO_REFRESH, X_HAS_QOS, X_HAS_WORST_QOS, X_PRIVATE_STREAM, X_PAUSE_FLAG, X_HAS_VIEW, X_HAS_BATCH, X_QUALIFIED_STREAM ),
/*REFRESH_MSG_CLASS */ bit_14( X_HAS_EXTENDED_HEADER, X_HAS_PERM_DATA, X_HAS_MSG_KEY, X_HAS_SEQ_NUM, X_SOLICITED, X_REFRESH_COMPLETE, X_HAS_QOS, X_CLEAR_CACHE, X_DO_NOT_CACHE, X_PRIVATE_STREAM, X_HAS_POST_USER_INFO, X_HAS_PART_NUM, X_HAS_REQ_MSG_KEY, X_QUALIFIED_STREAM ),
/*STATUS_MSG_CLASS  */ bit_10( X_HAS_EXTENDED_HEADER, X_HAS_PERM_DATA, X_HAS_MSG_KEY, X_HAS_GROUP_ID, X_HAS_STATE, X_CLEAR_CACHE, X_PRIVATE_STREAM, X_HAS_POST_USER_INFO, X_HAS_REQ_MSG_KEY, X_QUALIFIED_STREAM ),
/*UPDATE_MSG_CLASS  */ bit_10( X_HAS_EXTENDED_HEADER, X_HAS_PERM_DATA, X_HAS_MSG_KEY, X_HAS_SEQ_NUM, X_HAS_CONF_INFO, X_DO_NOT_CACHE, X_DO_NOT_CONFLATE, X_DO_NOT_RIPPLE, X_HAS_POST_USER_INFO, X_DISCARDABLE ),
/*CLOSE_MSG_CLASS   */ bit_3 ( X_HAS_EXTENDED_HEADER, X_ACK_FLAG, X_HAS_BATCH ),
/*ACK_MSG_CLASS     */ bit_7 ( X_HAS_EXTENDED_HEADER, X_HAS_TEXT, X_PRIVATE_STREAM, X_HAS_SEQ_NUM, X_HAS_MSG_KEY, X_HAS_NAK_CODE, X_QUALIFIED_STREAM ),
/*GENERIC_MSG_CLASS */ bit_8 ( X_HAS_EXTENDED_HEADER, X_HAS_PERM_DATA, X_HAS_MSG_KEY, X_HAS_SEQ_NUM, X_MESSAGE_COMPLETE, X_HAS_SECONDARY_SEQ_NUM, X_HAS_PART_NUM, X_HAS_REQ_MSG_KEY ),
/*POST_MSG_CLASS    */ bit_9 ( X_HAS_EXTENDED_HEADER, X_HAS_POST_ID, X_HAS_MSG_KEY, X_HAS_SEQ_NUM, X_POST_COMPLETE, X_ACK_FLAG, X_HAS_PERM_DATA, X_HAS_PART_NUM, X_HAS_POST_USER_RIGHTS )
};
const uint64_t rwf_msg_flag_only[ RWF_MSG_CLASS_COUNT ] = {
/*MSG_KEY_CLASS */     0,
/*REQUEST_MSG_CLASS */ bit_7 ( X_STREAMING, X_MSG_KEY_IN_UPDATES, X_CONF_INFO_IN_UPDATES, X_NO_REFRESH, X_PRIVATE_STREAM, X_PAUSE_FLAG, X_QUALIFIED_STREAM ),
/*REFRESH_MSG_CLASS */ bit_6 ( X_SOLICITED, X_REFRESH_COMPLETE, X_CLEAR_CACHE, X_DO_NOT_CACHE, X_PRIVATE_STREAM, X_QUALIFIED_STREAM ),
/*STATUS_MSG_CLASS  */ bit_3 ( X_CLEAR_CACHE, X_PRIVATE_STREAM, X_QUALIFIED_STREAM ),
/*UPDATE_MSG_CLASS  */ bit_4 ( X_DO_NOT_CACHE, X_DO_NOT_CONFLATE, X_DO_NOT_RIPPLE, X_DISCARDABLE ),
/*CLOSE_MSG_CLASS   */ bit   ( X_ACK_FLAG ),
/*ACK_MSG_CLASS     */ bit_2 ( X_PRIVATE_STREAM, X_QUALIFIED_STREAM ),
/*GENERIC_MSG_CLASS */ bit   ( X_MESSAGE_COMPLETE ),
/*POST_MSG_CLASS    */ bit_2 ( X_POST_COMPLETE, X_ACK_FLAG )
};
#undef bit
#undef bit_3
#undef bit_6
#undef bit_7
#undef bit_8
#undef bit_9
#undef bit_10
#undef bit_13
#undef bit_14

struct RwfMsgDecode : public DecodeT<RwfMsgDecode> {
  RwfMsgHdr & h;

  RwfMsgDecode( RwfMsgHdr &x,  DecodeHdr &hdr )
    : DecodeT( hdr.start, hdr.buf, hdr.eob, hdr.ok ), h( x ) {
    x.flags  = rwf_flags_map[ x.msg_class ]->serial_map( x.msg_flags );
    x.flags |= rwf_msg_always_present[ x.msg_class ];
  }

  RwfMsgDecode & qos( RwfQos &q ) {
    uint8_t x = 0;
    this->u8( x );
    q.timeliness = x >> 5;
    q.rate       = ( x >> 1 ) & 0xf;
    q.dynamic    = x & 1;
    q.time_info  = 0;
    q.rate_info  = 0;
    if ( q.timeliness > 2 )
      this->u16( q.time_info );
    if ( q.rate > 2 )
      this->u16( q.rate_info );
    return *this;
  }
  template <class Buf>
  RwfMsgDecode & buffer15( Buf &b ) {
    this->u15( b.len );
    b.buf = (void *) this->buf;
    this->buf = &this->buf[ b.len ];
    return *this;
  }
  template <class Buf>
  RwfMsgDecode & text15( Buf &b ) {
    this->u15( b.len );
    b.buf = (const char *) this->buf;
    this->buf = &this->buf[ b.len ];
    return *this;
  }
  template <class Buf>
  RwfMsgDecode & buffer8( Buf &b ) {
    this->u8( b.len );
    b.buf = (void *) this->buf;
    this->buf = &this->buf[ b.len ];
    return *this;
  }
  RwfMsgDecode & msg_key( RwfMsgKey &k ) {
    this->u15( k.data_size );
    k.data = (void *) this->buf;
    this->u15( k.key_flags );

    k.flags = rwf_flags_map[ MSG_KEY_CLASS ]->serial_map( k.key_flags );
    if ( k.test( X_HAS_SERVICE_ID ) )
      this->z16( k.service_id );

    if ( k.test( X_HAS_NAME ) ) {
      uint8_t name_len;
      this->u8( name_len );
      k.name = (char *) this->buf;
      k.name_len = name_len;
      this->buf = &this->buf[ name_len ];
      if ( k.test( X_HAS_NAME_TYPE ) )
        this->u8( k.name_type );
    }
    if ( k.test( X_HAS_FILTER ) )
      this->u32( k.filter );
    if ( k.test( X_HAS_IDENTIFIER ) )
      this->u32( k.identifier );
    if ( k.test( X_HAS_ATTRIB ) ) {
      this->u8( k.attrib.container_type );
      k.attrib.container_type += RWF_CONTAINER_BASE;
      this->u15( k.attrib.len );
      k.attrib.data = (void *) this->buf;
    }
    this->buf = &((uint8_t *) k.data)[ k.data_size ];
    return *this;
  }
  bool is_set( RwfMsgSerial s ) const {
    return this->h.test( s );
  }
  RwfMsgDecode & get_update_type( void ) {
    return is_set( X_HAS_UPDATE_TYPE ) ? this->u8( this->h.update_type ) : *this;
  }
  RwfMsgDecode & get_seq_num( void ) {
    return is_set( X_HAS_SEQ_NUM ) ? this->u32( this->h.seq_num ) : *this;
  }
  RwfMsgDecode & get_conflate_info( void ) {
    if ( is_set( X_HAS_CONF_INFO ) ) {
      this->u15( this->h.conf_info.count );
      return this->u16( this->h.conf_info.time );
    }
    return *this;
  }
  RwfMsgDecode & get_msg_key( void ) {
    return is_set( X_HAS_MSG_KEY ) ? this->msg_key( this->h.msg_key ) : *this;
  }
  RwfMsgDecode & get_extended( void ) {
    return is_set( X_HAS_EXTENDED_HEADER ) ? this->buffer8( this->h.extended ) : *this;
  }
  RwfMsgDecode & get_post_user( void ) {
    if ( is_set( X_HAS_POST_USER_INFO ) ) {
      this->u32( this->h.post_user.user_addr );
      return this->u32( this->h.post_user.user_id );
    }
    return *this;
  }
  RwfMsgDecode & get_second_seq_num( void ) {
    return is_set( X_HAS_SECONDARY_SEQ_NUM ) ? this->u32( this->h.second_seq_num ) : *this;
  }
  RwfMsgDecode & get_perm( void ) {
    return is_set( X_HAS_PERM_DATA ) ? this->buffer15( this->h.perm ) : *this;
  }
  RwfMsgDecode & get_req_msg_key( void ) {
    return is_set( X_HAS_REQ_MSG_KEY ) ? this->msg_key( this->h.req_msg_key ) : *this;
  }
  RwfMsgDecode & get_state( void ) {
    if ( is_set( X_HAS_STATE ) ) {
      uint8_t x = 0;
      this->u8( x );
      this->h.state.data_state   = x & 0x7;
      this->h.state.stream_state = x >> 3;
      this->u8( this->h.state.code );
      return this->text15( this->h.state.text );
    }
    return *this;
  }
  RwfMsgDecode & get_group_id( void ) {
    return is_set( X_HAS_GROUP_ID ) ? this->buffer8( this->h.group_id ) : *this;
  }
  RwfMsgDecode & get_qos( void ) {
    return is_set( X_HAS_QOS ) ? this->qos( this->h.qos ) : *this;
  }
  RwfMsgDecode & get_part_num( void ) {
    return is_set( X_HAS_PART_NUM ) ? this->u15( this->h.part_num ) : *this;
  }
  RwfMsgDecode & get_post_id( void ) {
    return is_set( X_HAS_POST_ID ) ? this->u32( this->h.post_id ) : *this;
  }
  RwfMsgDecode & get_post_rights( void ) {
    return is_set( X_HAS_POST_USER_RIGHTS ) ? this->u15( this->h.post_rights ) : *this;
  }
  RwfMsgDecode & get_priority( void ) {
    if ( is_set( X_HAS_PRIORITY ) ) {
      this->u8( this->h.priority.clas );
      return this->z16( this->h.priority.count );
    }
    return *this;
  }
  RwfMsgDecode & get_worst_qos( void ) {
    return is_set( X_HAS_WORST_QOS ) ? this->qos( this->h.worst_qos ) : *this;
  }
  RwfMsgDecode & get_ack_id( void ) {
    return is_set( X_HAS_ACK_ID ) ? this->u32( this->h.ack_id ) : *this;
  }
  RwfMsgDecode & get_nak_code( void ) {
    return is_set( X_HAS_NAK_CODE ) ? this->u8( this->h.nak_code ) : *this;
  }
  RwfMsgDecode & get_text( void ) {
    return is_set( X_HAS_TEXT ) ? this->text15( this->h.text ) : *this;
  }
};
#endif

}
}
#endif
