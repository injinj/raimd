#ifndef __rai_raimd__rwf_writer_h__
#define __rai_raimd__rwf_writer_h__

namespace rai {
namespace md {

struct RwfFieldListWriter;
struct RwfElementListWriter;
struct RwfMsgKeyWriter;
struct RwfMapWriter;
struct RwfFilterListWriter;
struct RwfSeriesWriter;
struct RwfVectorWriter;
struct RwfElementListSet;
struct RwfFieldListSet;
struct RwfFieldDefnWriter;
struct MDLookup;

enum RwfWriterType {
  W_NONE         = 128, /* RWF_NO_DATA      */
  W_MSG_KEY      = 129, /* RWF_MSG_KEY      */
  W_FIELD_LIST   = 132, /* RWF_FIELD_LIST   */
  W_ELEMENT_LIST = 133, /* RWF_ELEMENT_LIST */
  W_FILTER_LIST  = 135, /* RWF_FILTER_LIST  */
  W_VECTOR       = 136, /* RWF_VECTOR       */
  W_MAP          = 137, /* RWF_MAP          */
  W_SERIES       = 138, /* RWF_SERIES       */
  W_MSG          = 141  /* RWF_MSG          */
};

struct RwfMsgWriterBase {
  MDMsgMem & mem;
  MDDict   * dict;
  uint8_t  * buf;
  size_t     off,
             buflen,
             prefix_len;
  int        len_bits,
             err;
  size_t   * size_ptr;
  RwfMsgWriterBase  * parent,
                    * child;
  const RwfWriterType type;
  bool is_complete;

  RwfMsgWriterBase( RwfWriterType t,  MDMsgMem &mem,  MDDict *d,  void *bb,
                    size_t len ) noexcept;
  bool has_space( size_t len ) {
    bool b = ( this->off + len <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool check_offset( void ) {
    bool b = ( this->off <= this->buflen );
    if ( ! b ) b = this->resize( this->buflen - this->off );
    return b;
  }
  bool check_prefix( size_t len ) {
    if ( len > this->prefix_len ) {
      size_t need = this->prefix_len - len;
      if ( ! this->has_space( need ) )
        return false;
      ::memmove( &this->buf[ need ], this->buf, this->off );
      this->buf        += need;
      this->prefix_len += need;
      this->buflen     -= need;
    }
    return true;
  }
  bool resize( size_t len ) noexcept;
  void reset( size_t o,  size_t p = 0 ) noexcept;
  int error( int e ) noexcept;
  void * make_child( void ) noexcept;
  int append_base( RwfMsgWriterBase &base,  int bits,
                   size_t *sz_ptr ) noexcept;
  RwfMsgWriterBase & incr( size_t len ) {
    this->off += len;
    return *this;
  }
  RwfMsgWriterBase & seek( size_t new_off ) {
    this->off = new_off;
    return *this;
  }
  RwfMsgWriterBase & u16( uint16_t x ) {
    set_u16<MD_BIG>( &this->buf[ this->off ], x ); this->off += 2;
    return *this;
  }
  RwfMsgWriterBase & u32( uint32_t x ) {
    set_u32<MD_BIG>( &this->buf[ this->off ], x ); this->off += 4;
    return *this;
  }
  RwfMsgWriterBase & u64( uint64_t x ) {
    set_u64<MD_BIG>( &this->buf[ this->off ], x ); this->off += 8;
    return *this;
  }
  RwfMsgWriterBase & u8( uint8_t x ) {
    this->buf[ this->off++ ] = x;
    return *this;
  }
  RwfMsgWriterBase & z16( uint16_t x ) {
    this->off += set_fe_prefix( &this->buf[ this->off ], x );
    return *this;
  }
  RwfMsgWriterBase & u15( uint16_t x ) {
    this->off += set_u15_prefix( &this->buf[ this->off ], x );
    return *this;
  }
  RwfMsgWriterBase & b( const void *x,  size_t buflen ) {
    ::memcpy( &this->buf[ this->off ], x, buflen );
    this->off += buflen;
    return *this;
  }
  RwfMsgWriterBase & uN( const void *x,  size_t buflen ) {
    if ( buflen == 1 ) {
      return this->u8( *(uint8_t *) x );
    }
    if ( buflen == 2 ) {
      uint16_t u;
      ::memcpy( &u, x, 2 );
      return this->u16( u );
    }
    if ( buflen == 4 ) {
      uint32_t u;
      ::memcpy( &u, x, 4 );
      return this->u32( u );
    }
    uint64_t u;
    ::memcpy( &u, x, 8 );
    return this->u64( u );
    /*while ( buflen > 0 )
      this->buf[ this->off++ ] = ((uint8_t *) x)[ --buflen ];
    return *this;*/
  }
  RwfMsgWriterBase & uN2( const void *x,  size_t buflen,  void *y ) {
    if ( buflen == 2 ) {
      uint16_t u;
      ::memcpy( &u, x, 2 );
      set_u16<MD_BIG>( y, u );
    }
    else if ( buflen == 4 ) {
      uint32_t u;
      ::memcpy( &u, x, 4 );
      set_u32<MD_BIG>( y, u );
    }
    else {
      uint64_t u;
      ::memcpy( &u, x, 8 );
      set_u64<MD_BIG>( y, u );
    }
    return *this;
  }
  RwfMsgWriterBase & u( uint64_t x,  size_t ilen ) {
    uint8_t * ptr = &this->buf[ this->off ];
    this->off += ilen;
    do {
      ptr[ --ilen ] = (uint8_t) ( x & 0xffU );
      x >>= 8;
    } while ( ilen != 0 );

    return *this;
  }
  RwfMsgWriterBase & i( int64_t x,  size_t ilen ) {
    return this->u( (uint64_t) x, ilen );
  }
  RwfMsgWriterBase & f64( double x ) {
    uint8_t * f   = (uint8_t *) (void *) &x;
    uint8_t * ptr = &this->buf[ this->off ];
    this->off += 8;
    for ( int i = 0; i < 8; i++ )
      ptr[ i ] = f[ 7 - i ];
    return *this;
  }
  RwfMsgWriterBase & f32( float x ) {
    uint8_t * f   = (uint8_t *) (void *) &x;
    uint8_t * ptr = &this->buf[ this->off ];
    this->off += 4;
    for ( int i = 0; i < 4; i++ )
      ptr[ i ] = f[ 3 - i ];
    return *this;
  }
  int pack_mref( uint8_t type,  MDReference &mref ) noexcept;
  void pack_time( size_t sz,  const MDTime &time ) noexcept;
  static size_t time_size( const MDTime &time ) noexcept;

  static size_t uint_size( uint64_t uval ) {
    uint64_t bits = 0xff;
    size_t   ilen = 1;
    while ( ( uval & bits ) != uval ) {
      ilen++;
      bits = ( bits << 8 ) | (uint64_t) 0xffU;
    }
    return ilen;
  }
  static size_t int_size( int64_t ival ) {
    uint64_t uval = (uint64_t) ival;
    uint64_t bits = ( ~(uint64_t) 0 << 7 );
    size_t   ilen = 1;
    if ( ival >= 0 )
      uval = ~uval;
    while ( ( uval | bits ) != uval ) {
      ilen++;
      bits <<= 8;
    }
    return ilen;
  }
  static size_t fname_pack_size( size_t fname_len,  size_t fsize ) {
    if ( fname_len > 0x7fff )
      return (size_t) -1;
    return get_u15_prefix_len( fname_len ) + fname_len + 1 +
           get_fe_prefix_len( fsize ) + fsize;
  }
  static size_t fid_pack_size( size_t fsize ) {
    return get_fe_prefix_len( fsize ) + 2 /* fid */ + fsize;
  }
  static size_t map_pack_size( size_t fsize ) {
    return get_fe_prefix_len( fsize ) + 1 /* action */ + fsize;
  }
  size_t complete( void ) noexcept;
  /* update_hdr until t == parent, W_NONE ends all containers */
  size_t end( RwfWriterType t = W_NONE ) noexcept;
  size_t end_msg_key( void )      { return this->end( W_MSG_KEY ); }
  size_t end_field_list( void )   { return this->end( W_FIELD_LIST ); }
  size_t end_element_list( void ) { return this->end( W_ELEMENT_LIST ); }
  size_t end_filter_list( void )  { return this->end( W_FILTER_LIST ); }
  size_t end_vector( void )       { return this->end( W_VECTOR ); }
  size_t end_map( void )          { return this->end( W_MAP ); }
  size_t end_series( void )       { return this->end( W_SERIES ); }
  size_t end_msg( void )          { return this->end( W_MSG ); }
  size_t end_summary( void )      { return this->end( this->type ); }
  size_t end_entry( void )        { return this->end( this->type ); }
};

struct RwfMsgWriterHdr : public RwfMsgWriterBase {
  RwfMsgWriterBase & base;
  uint64_t           flags;
  RwfMsgWriterHdr( RwfMsgWriterBase &b,  uint64_t fl = 0 )
    : RwfMsgWriterBase( W_NONE, b.mem, NULL, b.buf, b.buflen ), base( b ),
      flags( fl ) {
    if ( b.len_bits != 0 || b.size_ptr != NULL )
      this->update_len( b );
  }
  void update_len( RwfMsgWriterBase &b ) noexcept;
  ~RwfMsgWriterHdr() {
    if ( this->base.parent != NULL )
      this->base.parent->off += this->base.off;
  }
  bool is_set( RwfMsgSerial s ) const {
    uint64_t mask = (uint64_t) 1U << s;
    return ( this->flags & mask ) != 0;
  }
  RwfMsgWriterHdr &set_update_type( uint8_t update_type )        { if ( this->is_set( X_HAS_UPDATE_TYPE ) )       this->u8( update_type );           return *this; }
  RwfMsgWriterHdr &set_nak_code( uint8_t nak_code )              { if ( this->is_set( X_HAS_NAK_CODE ) )          this->u8( nak_code );              return *this; }
  RwfMsgWriterHdr &set_seq_num( uint32_t seq_num )               { if ( this->is_set( X_HAS_SEQ_NUM ) )           this->u32( seq_num );              return *this; }
  RwfMsgWriterHdr &set_second_seq_num( uint32_t second_seq_num ) { if ( this->is_set( X_HAS_SECONDARY_SEQ_NUM ) ) this->u32( second_seq_num );       return *this; }
  RwfMsgWriterHdr &set_post_id( uint32_t post_id )               { if ( this->is_set( X_HAS_POST_ID ) )           this->u32( post_id );              return *this; }
  RwfMsgWriterHdr &set_ack_id( uint32_t ack_id )                 { if ( this->is_set( X_HAS_ACK_ID ) )            this->u32( ack_id );               return *this; }
  RwfMsgWriterHdr &set_part_num( uint16_t part_num )             { if ( this->is_set( X_HAS_PART_NUM ) )          this->u16( part_num | 0x8000 );    return *this; }
  RwfMsgWriterHdr &set_post_rights( uint16_t post_rights )       { if ( this->is_set( X_HAS_POST_USER_RIGHTS ) )  this->u16( post_rights | 0x8000 ); return *this; }
  RwfMsgWriterHdr &set_perm( RwfPerm &perm )                     { if ( this->is_set( X_HAS_POST_USER_RIGHTS ) )  this->u16( perm.len | 0x8000 ) 
                                                                                                                       .b  ( perm.buf, perm.len );   return *this; }
  /* msg_key  */
  /* extended */
  /* req_key  */
  RwfMsgWriterHdr &set_group_id( RwfGroup &group_id )            { if ( this->is_set( X_HAS_GROUP_ID ) )          this->u8 ( group_id.len )
                                                                                                                       .b  ( group_id.buf, group_id.len ); return *this; }
  RwfMsgWriterHdr &set_post_user( RwfPostInfo &post_user )       { if ( this->is_set( X_HAS_POST_USER_INFO ) )    this->u32( post_user.user_addr )
                                                                                                                       .u32( post_user.user_id );    return *this; }
  RwfMsgWriterHdr &set_conflate_info( RwfConfInfo &conf_info )   { if ( this->is_set( X_HAS_CONF_INFO ) )         this->u16( conf_info.count | 0x8000 )
                                                                                                                       .u16( conf_info.time );       return *this; }
  RwfMsgWriterHdr &set_state( RwfState &state )                  { if ( this->is_set( X_HAS_STATE ) )             this->u8 ( state.data_state | ( state.stream_state << 3 ) )
                                                                                                                       .u8 ( state.code )
                                                                                                                       .u16( state.text.len | 0x8000 )
                                                                                                                       .b  ( state.text.buf, state.text.len ); return *this; }
  RwfMsgWriterHdr &set_qos( RwfQos &qos )                        { if ( this->is_set( X_HAS_QOS ) )               this->add_qos( qos );              return *this; }
  RwfMsgWriterHdr &set_worst_qos( RwfQos &worst_qos )            { if ( this->is_set( X_HAS_WORST_QOS ) )         this->add_qos( worst_qos );        return *this; }
  RwfMsgWriterHdr &set_text( RwfText &text )                     { if ( this->is_set( X_HAS_TEXT ) )              this->u16( text.len | 0x8000 )
                                                                                                                       .b  ( text.buf, text.len );   return *this; }
  RwfMsgWriterHdr &set_priority( RwfPriority &priority )         { if ( this->is_set( X_HAS_PRIORITY ) )          this->u8 ( priority.clas )
                                                                                                                       .u8 ( 0xfe )
                                                                                                                       .u16( priority.count );       return *this; }
  void add_qos( RwfQos &qos ) {
    this->u8( ( qos.timeliness << 5 ) | ( qos.rate << 1 ) |
              qos.dynamic );
    if ( qos.timeliness > 2 )
      this->u16( qos.time_info );
    if ( qos.rate > 2 )
      this->u16( qos.rate_info );
  }
};

/*      +==== 
 hdr -> |
        +---- msg_key_off
        |
        +---- container_off
 */
struct RwfMsgWriter : RwfMsgWriterBase {
  const uint64_t all_flags;
  uint64_t    flags;
  size_t      msg_key_off,
              msg_key_size,
              container_off,
              container_size;
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
  /*          msg_key          * . | ? | ? | ? |   | ? | ? | ? */
  /*          extended         * ? | ? | ? | ? | ? | ? | ? | ? */
  RwfPostInfo post_user;      /*   | ? | ? | ? |   |   |   | . */
  /*          req_msg_key      *   | ? | ? |   |   |   | ? |   */
  uint16_t    part_num,       /*   | ? |   |   |   |   | ? | ? */
              post_rights;    /*   |   |   |   |   |   |   | ? */

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfMsgWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len,  RwfMsgClass cl,
                RdmDomainType dom,  uint32_t id )
    : RwfMsgWriterBase( W_MSG, m, d, bb, len ),
      all_flags( rwf_msg_always_present[ cl ] | rwf_msg_maybe_present[ cl ] |
                 rwf_msg_flag_only[ cl ] ),
      msg_class( cl ), domain_type( dom ), stream_id( id ) {
    this->reset();
  }
  bool check_container( RwfMsgWriterBase &base ) noexcept;
  /* children */
  RwfMsgKeyWriter      & add_msg_key     ( void ) noexcept;
  RwfFieldListWriter   & add_field_list  ( void ) noexcept;
  RwfElementListWriter & add_element_list( void ) noexcept;
  RwfMapWriter         & add_map         ( MDType key_type ) noexcept;
  RwfFilterListWriter  & add_filter_list ( void ) noexcept;
  RwfSeriesWriter      & add_series      ( void ) noexcept;
  RwfVectorWriter      & add_vector      ( void ) noexcept;
  /* msg_flags */
  bool is_set( RwfMsgSerial s ) {
    uint64_t mask = (uint64_t) 1U << s;
    return ( this->flags & mask ) != 0;
  }
  RwfMsgWriter & set( RwfMsgSerial s ) {
    uint64_t mask = (uint64_t) 1U << s;
    if ( ( this->all_flags & mask ) != 0 )
      this->flags |= mask;
    return *this;
  }
  RwfMsgWriter & set( RwfMsgSerial s,  RwfMsgSerial t )                                   { this->set( s ); return this->set( t ); }
  RwfMsgWriter & set( RwfMsgSerial s,  RwfMsgSerial t,  RwfMsgSerial u )                  { this->set( s, t ); return this->set( u ); }
  RwfMsgWriter & set( RwfMsgSerial s,  RwfMsgSerial t,  RwfMsgSerial u,  RwfMsgSerial v ) { this->set( s, t, u ); return this->set( v ); }

  void reset( void ) noexcept;
  size_t update_hdr( void ) noexcept;
  /* update_type */
  RwfMsgWriter & add_update( RdmUpdateType type ) {
    this->update_type = type; /* always included in UPDATE */
    return *this;
  }
  /* nak_code */
  RwfMsgWriter & add_nak_code( uint8_t cd ) {
    this->nak_code = cd;
    return this->set( X_HAS_NAK_CODE );
  }
  /* text */
  RwfMsgWriter & add_text( const char *s ) {
    this->text.buf = (char *) s;
    this->text.len = ::strlen( s );
    return this->set( X_HAS_TEXT );
  }
  /* seq_num */
  RwfMsgWriter & add_seq_num( uint32_t num ) {
    this->seq_num = num;
    return this->set( X_HAS_SEQ_NUM );
  }
  /* post_id */
  RwfMsgWriter & add_post_id( uint32_t id ) {
    this->post_id = id;
    return this->set( X_HAS_POST_ID );
  }
  /* ack_id */
  RwfMsgWriter & add_ack_id( uint32_t id ) {
    this->ack_id = id;
    return this->set( X_HAS_ACK_ID );
  }
  /* post_user */
  RwfMsgWriter & add_post_user( uint32_t addr,  uint32_t id ) {
    this->post_user.user_addr = addr;
    this->post_user.user_id   = id;
    return this->set( X_HAS_POST_USER_INFO );
  }
  /* state */
  RwfMsgWriter & add_state( RdmDataState data,  RdmStreamState stream,
                            const char *text = NULL,  uint8_t code = 0 ) {
    this->state.data_state   = data;
    this->state.stream_state = stream;
    this->state.code         = code;
    if ( text == NULL ) {
      this->state.text.len = 0;
      this->state.text.buf = 0;
    }
    else {
      this->state.text.len = ::strlen( text );
      this->state.text.buf = text;
    }
    return this->set( X_HAS_STATE );
  }
  RwfMsgWriter & add_state( RwfState &state ) {
    this->state.data_state   = state.data_state;
    this->state.stream_state = state.stream_state;
    this->state.code         = state.code;
    this->state.text.len     = state.text.len;
    this->state.text.buf     = state.text.buf;
    return this->set( X_HAS_STATE );
  }
  /* group_id */
  RwfMsgWriter & add_group_id( void *g,  size_t g_len ) {
    this->group_id.buf = g;
    this->group_id.len = g_len;
    return this->set( X_HAS_GROUP_ID );
  }
  RwfMsgWriter & add_group_id( RwfGroup &group ) {
    return this->add_group_id( group.buf, group.len );
  }
  /* conf_info */
  RwfMsgWriter & add_conf_info( uint32_t cnt,  uint32_t tm ) {
    this->conf_info.count = cnt;
    this->conf_info.time  = tm;
    return this->set( X_HAS_CONF_INFO );
  }
  RwfMsgWriter & add_conf_info( RwfConfInfo &conf ) {
    return this->add_conf_info( conf.count, conf.time );
  }
  /* conf_info */
  RwfMsgWriter & add_priority( uint8_t cl,  uint16_t cnt ) {
    this->priority.clas  = cl;
    this->priority.count = cnt;
    return this->set( X_HAS_PRIORITY );
  }
  RwfMsgWriter & add_pririty( RwfPriority &prio ) {
    return this->add_priority( prio.clas, prio.count );
  }
  /* qos */
  RwfMsgWriter & add_qos( RdmQosTime time,  RdmQosRate rate,  bool dynamic ) {
    this->qos.timeliness = time;
    this->qos.rate       = rate;
    this->qos.dynamic    = dynamic ? 1 : 0;
    return this->set( X_HAS_QOS );
  }
  RwfMsgWriter & add_qos( RwfQos &qos ) {
    return this->add_qos( (RdmQosTime) qos.timeliness, (RdmQosRate) qos.rate,
                          qos.dynamic );
  }

  bool is_set( RwfMsgSerial s ) const {
    uint64_t mask = (uint64_t) 1U << s;
    return ( this->flags & mask ) != 0;
  }
  size_t size_upto_msg_key( void ) const noexcept;
  size_t size_after_msg_key( void ) const noexcept;
  const RwfMsgWriter &size_update_type( size_t &sz )   const { if ( this->is_set( X_HAS_UPDATE_TYPE ) )       sz++;                              return *this; }
  const RwfMsgWriter &size_nak_code( size_t &sz )      const { if ( this->is_set( X_HAS_NAK_CODE ) )          sz++;                              return *this; }
  const RwfMsgWriter &size_seq_num( size_t &sz )       const { if ( this->is_set( X_HAS_SEQ_NUM ) )           sz += 4;                           return *this; }
  const RwfMsgWriter &size_second_seq_num( size_t &sz )const { if ( this->is_set( X_HAS_SECONDARY_SEQ_NUM ) ) sz += 4;                           return *this; }
  const RwfMsgWriter &size_post_id( size_t &sz )       const { if ( this->is_set( X_HAS_POST_ID ) )           sz += 4;                           return *this; }
  const RwfMsgWriter &size_ack_id( size_t &sz )        const { if ( this->is_set( X_HAS_ACK_ID ) )            sz += 4;                           return *this; }
  const RwfMsgWriter &size_part_num( size_t &sz )      const { if ( this->is_set( X_HAS_PART_NUM ) )          sz += 2;                           return *this; }
  const RwfMsgWriter &size_post_rights( size_t &sz )   const { if ( this->is_set( X_HAS_POST_USER_RIGHTS ) )  sz += 2;                           return *this; }
  /* msg_key  */
  /* extended */
  /* req_key  */
  const RwfMsgWriter &size_perm( size_t &sz )          const { if ( this->is_set( X_HAS_PERM_DATA ) )         sz += 2 + this->perm.len;          return *this; }
  const RwfMsgWriter &size_group_id( size_t &sz)       const { if ( this->is_set( X_HAS_GROUP_ID ) )          sz += 1 + this->group_id.len;      return *this; }
  const RwfMsgWriter &size_post_user( size_t &sz )     const { if ( this->is_set( X_HAS_POST_USER_INFO ) )    sz += 8;                           return *this; }
  const RwfMsgWriter &size_conflate_info( size_t &sz ) const { if ( this->is_set( X_HAS_CONF_INFO ) )         sz += 4;                           return *this; }
  const RwfMsgWriter &size_state( size_t &sz )         const { if ( this->is_set( X_HAS_STATE ) )             sz += 4 + this->state.text.len;    return *this; }
  const RwfMsgWriter &size_qos( size_t &sz )           const { if ( this->is_set( X_HAS_QOS ) )               sz += this->qos_size( this->qos );       return *this; }
  const RwfMsgWriter &size_worst_qos( size_t &sz)      const { if ( this->is_set( X_HAS_WORST_QOS ) )         sz += this->qos_size( this->worst_qos ); return *this; }
  const RwfMsgWriter &size_text( size_t &sz )          const { if ( this->is_set( X_HAS_TEXT ) )              sz += 2 + this->text.len;          return *this; }
  const RwfMsgWriter &size_priority( size_t &sz )      const { if ( this->is_set( X_HAS_PRIORITY ) )          sz += 4;                           return *this; }
  static size_t qos_size( const RwfQos &qos ) { return 1 + ( qos.timeliness > 2 ? 2 : 0 ) + ( qos.rate > 2 ? 2 : 0 ); }
};

struct RwfMsgKeyWriter : public RwfMsgWriterBase {
  uint8_t key_flags;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfMsgKeyWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len )
    : RwfMsgWriterBase( W_MSG_KEY, m, d, bb, len ) {
    this->reset();
  }
  void reset( void ) { this->RwfMsgWriterBase::reset( 1 );
                       this->key_flags = 0; }
  size_t update_hdr( void ) noexcept;
  RwfMsgKeyWriter & set_error( int e ) {
    this->error( e );
    return *this;
  }
  RwfMsgKeyWriter & order_error( int fl ) noexcept;
  /* in this order */
  RwfMsgKeyWriter & service_id( uint16_t service_id ) noexcept;
  RwfMsgKeyWriter & name( const char *nm,  size_t nm_size ) noexcept;
  RwfMsgKeyWriter & name( const char *nm ) {
    return this->name( nm, ::strlen( nm ) );
  }
  RwfMsgKeyWriter & name_type( uint8_t name_type ) noexcept;
  RwfMsgKeyWriter & filter( uint32_t filter ) noexcept;
  RwfMsgKeyWriter & identifier( uint32_t id ) noexcept;
  RwfElementListWriter & attrib( void ) noexcept;
};

struct RwfFieldListWriter : public RwfMsgWriterBase {
  uint16_t nflds,
           flist,
           set_nflds,
           set_id;
  uint32_t set_size;
  RwfFieldListSet * set;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfFieldListWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len )
    : RwfMsgWriterBase( W_FIELD_LIST, m, d, bb, len ), nflds( 0 ), flist( 0 ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 7 );
    this->nflds     = 0;
    this->flist     = 0;
    this->set_nflds = 0;
    this->set_id    = 0;
    this->set_size  = 0;
    this->set       = NULL;
  }
  size_t update_hdr( void ) noexcept;
  RwfFieldListWriter & set_error( int e ) {
    this->error( e );
    return *this;
  }
  bool match_set( MDFid fid ) noexcept;
  RwfFieldListWriter & use_field_set( uint16_t id ) noexcept;
  RwfFieldListWriter & append_ref( MDFid fid,  MDReference &mref ) noexcept;
  RwfFieldListWriter & append_ref( const char *fname,  size_t fname_len,
                                   MDReference &mref ) noexcept;
  RwfFieldListWriter & append_ref( const char *fname,
                                   MDReference &mref ) {
    return this->append_ref( fname, ::strlen( fname ), mref );
  }
  RwfFieldListWriter & append_ref( MDFid fid, MDType ftype, uint32_t fsize,
                                   MDReference &mref ) noexcept;
  template< class T >
  RwfFieldListWriter & append_type( MDFid fid,  T val,  MDType t ) {
    MDReference mref( (void *) &val, sizeof( val ), t, md_endian );
    return this->append_ref( fid, mref );
  }
  template< class T >
  RwfFieldListWriter & append_type( const char *fname,  size_t fname_len,
                                    T val,  MDType t ) {
    MDReference mref( (void *) &val, sizeof( val ), t, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  RwfFieldListWriter & append_ival( MDFid fid,  const void *ival,  size_t ilen,
                                    MDType t ) noexcept;
  RwfFieldListWriter & append_ival( const char *fname,  size_t fname_len,  const void *ival,
                                    size_t ilen,  MDType t ) noexcept;
  RwfFieldListWriter & append_ival( MDLookup &by,  const void *ival,
                                    size_t ilen,  MDType t ) noexcept;

  RwfFieldListWriter & pack_uval( MDFid fid,  uint64_t ival ) noexcept;

  RwfFieldListWriter & pack_ival( MDFid fid,  int64_t ival ) noexcept;

  RwfFieldListWriter & pack_partial( MDFid fid, const uint8_t *fptr,
                                     size_t fsize,  size_t foffset ) noexcept;
  template< class T >
  RwfFieldListWriter & append_int( MDFid fid,  T ival ) {
    return this->append_ival( fid, &ival, sizeof( ival ), MD_INT );
  }
  template< class T >
  RwfFieldListWriter & append_uint( MDFid fid,  T uval ) {
    return this->append_ival( fid, &uval, sizeof( uval ), MD_UINT );
  }
  template< class T >
  RwfFieldListWriter & append_real( MDFid fid,  T rval ) {
    return this->append_type( fid, rval, MD_REAL );
  }

  template< class T >
  RwfFieldListWriter & append_int( const char *fname,  size_t fname_len, T ival ) {
    return this->append_ival( fname, fname_len, &ival, sizeof( ival ), MD_INT );
  }
  template< class T >
  RwfFieldListWriter & append_uint( const char *fname,  size_t fname_len, T uval ) {
    return this->append_ival( fname, fname_len, &uval, sizeof( uval ), MD_UINT);
  }
  template< class T >
  RwfFieldListWriter & append_real( const char *fname,  size_t fname_len, T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }
  template< class T >
  RwfFieldListWriter & append_int( const char *fname,  T ival ) {
    return this->append_ival( fname, ::strlen( fname ), &ival, sizeof( ival ), MD_INT );
  }
  template< class T >
  RwfFieldListWriter & append_uint( const char *fname,  T uval ) {
    return this->append_ival( fname, ::strlen( fname ), &uval, sizeof( uval ), MD_UINT);
  }
  template< class T >
  RwfFieldListWriter & append_real( const char *fname,  T rval ) {
    return this->append_type( fname, ::strlen( fname ), rval, MD_REAL );
  }

  RwfFieldListWriter & append_string( MDFid fid,  const char *str,  size_t len ) {
    MDReference mref( (void *) str, len, MD_STRING, md_endian );
    return this->append_ref( fid, mref );
  }
  RwfFieldListWriter & append_string( MDFid fid,  const char *str ) {
    return this->append_string( fid, str, ::strlen( str ));
  }
  RwfFieldListWriter & append_string( const char *fname,  size_t fname_len,
                                      const char *str,  size_t len ) {
    MDReference mref( (void *) str, len, MD_STRING, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  RwfFieldListWriter & append_string( const char *fname, const char *str ) {
    return this->append_string( fname, ::strlen( fname ), str, ::strlen( str ));
  }

  RwfFieldListWriter & append_decimal( MDFid fid,  MDType ftype, uint32_t fsize,
                                       MDDecimal &dec ) noexcept;
  RwfFieldListWriter & append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                                    MDTime &time ) noexcept;
  RwfFieldListWriter & append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                                    MDDate &date ) noexcept;

  RwfFieldListWriter & append_decimal( MDFid fid,  MDDecimal &dec ) noexcept;
  RwfFieldListWriter & append_time( MDFid,  MDTime &time ) noexcept;
  RwfFieldListWriter & append_date( MDFid,  MDDate &date ) noexcept;

  RwfFieldListWriter & append_decimal( const char *fname,  size_t fname_len,
                                       MDDecimal &dec ) noexcept;
  RwfFieldListWriter & append_time( const char *fname,  size_t fname_len,
                                    MDTime &time ) noexcept;
  RwfFieldListWriter & append_date( const char *fname,  size_t fname_len,
                                    MDDate &date ) noexcept;
  RwfFieldListWriter & append_decimal( const char *fname,  MDDecimal &dec ) {
    return this->append_decimal( fname, ::strlen( fname ), dec );
  }
  RwfFieldListWriter & append_time( const char *fname,  MDTime &time ) {
    return this->append_time( fname, ::strlen( fname ), time );
  }
  RwfFieldListWriter & append_date( const char *fname,  MDDate &date ) {
    return this->append_date( fname, ::strlen( fname ), date );
  }
  RwfFieldListWriter & append_set_ref( MDReference &mref ) noexcept;
  RwfFieldListWriter & append_set_string( const char *str,  size_t len ) {
    MDReference mref( (void *) str, len, MD_STRING, md_endian );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_string( const char *str ) {
    return this->append_set_string( str, ::strlen( str ));
  }
  RwfFieldListWriter & append_set_uint( uint64_t ival ) {
    MDReference mref( &ival, sizeof( ival ), MD_UINT );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_int( int64_t ival ) {
    MDReference mref( &ival, sizeof( ival ), MD_INT );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_real( double rval ) {
    MDReference mref( &rval, sizeof( rval ), MD_REAL );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_decimal( MDDecimal &dec ) {
    MDReference mref( &dec, sizeof( MDDecimal ), MD_DECIMAL );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_time( MDTime &time ) {
    MDReference mref( &time, sizeof( MDTime ), MD_TIME );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_date( MDDate &date ) {
    MDReference mref( &date, sizeof( MDDate ), MD_DATE );
    return this->append_set_ref( mref );
  }
  RwfFieldListWriter & append_set_ival( const void *ival, size_t ilen,
                                        MDType t ) {
    MDReference mref( (void *) ival, ilen, t );
    return this->append_set_ref( mref );
  }
  template <class WRITER, class... Ts>
  RwfFieldListWriter &apply( WRITER &writer,
       RwfFieldListWriter &(WRITER::*cb)( RwfFieldListWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

enum RwfFieldSetKind {
  DEFN_IS_FIELD_LIST = 0,
  DEFN_IS_ELEM_LIST  = 1
};

struct RwfFieldSetList {
  RwfFieldSetList * next;
  RwfFieldSetKind   kind;
  union {
    RwfFieldListSet   fld_list_defn;
    RwfElementListSet elem_list_defn;
  };
  void init( uint16_t id,  RwfFieldSetKind k ) noexcept;
  void add( const char *fname,  uint8_t rwf_type ) noexcept;
  void add( uint16_t fid,  uint8_t rwf_type ) noexcept;
  size_t size( void ) noexcept;
  void encode( RwfMsgWriterBase &hdr ) noexcept;
};

struct RwfFieldDefnWriter {
  RwfMsgWriterBase & parent;
  RwfFieldSetList  * hd, * tl, * set;
  uint32_t           num_sets,
                     hdr_off;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfFieldDefnWriter( RwfMsgWriterBase & p,  uint32_t off )
    : parent( p ), hd( 0 ), tl( 0 ), set( 0 ), num_sets( 0 ), hdr_off( off ) {}

  RwfFieldDefnWriter & add_defn( uint16_t id,  RwfFieldSetKind k ) noexcept;
  RwfFieldDefnWriter & end_defn( void ) noexcept;
  void end_field_defn( void ) noexcept;
  RwfFieldDefnWriter & append_defn( const char *fname,
                                    uint8_t rwf_type ) noexcept;
};

struct RwfElementListWriter : public RwfMsgWriterBase {
  uint16_t nitems,
           set_nitems,
           set_id;
  uint32_t set_size;
  RwfElementListSet * set;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfElementListWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len )
    : RwfMsgWriterBase( W_ELEMENT_LIST, m, d, bb, len ), nitems( 0 ),
      set_nitems( 0 ), set_id( 0 ), set_size( 0 ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 3 );
    this->nitems     = 0;
    this->set_nitems = 0;
    this->set_id     = 0;
    this->set_size   = 0;
    this->set        = NULL;
  }
  size_t update_hdr( void ) noexcept;
  RwfElementListWriter & set_error( int e ) {
    this->error( e );
    return *this;
  }
  bool match_set( const char *fname,  size_t fname_len ) noexcept;
  RwfElementListWriter & use_field_set( uint16_t id ) noexcept;
  RwfElementListWriter & append_ref( const char *fname,  size_t fname_len,
                                     MDReference &mref ) noexcept;
  RwfElementListWriter & append_ref( const char *fname,
                                     MDReference &mref ) {
    return this->append_ref( fname, ::strlen( fname ), mref );
  }
  template< class T >
  RwfElementListWriter & append_type( const char *fname,  size_t fname_len,
                                      T val,  MDType t ) {
    MDReference mref( (void *) &val, sizeof( val ), t, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  RwfElementListWriter & pack_uval( const char *fname,  size_t fname_len,
                                    uint64_t ival ) noexcept;
  RwfElementListWriter & pack_ival( const char *fname,  size_t fname_len,
                                    int64_t ival ) noexcept;
  RwfElementListWriter & pack_real( const char *fname,  size_t fname_len,
                                    double rval ) noexcept;
  template< class T >
  RwfElementListWriter & append_int( const char *fname,  size_t fname_len,
                                     T ival ) {
    return this->pack_ival( fname, fname_len, (int64_t) ival );
  }
  template< class T >
  RwfElementListWriter & append_uint( const char *fname,  size_t fname_len,
                                      T uval ) {
    return this->pack_ival( fname, fname_len, (uint64_t) uval );
  }
  template< class T >
  RwfElementListWriter & append_real( const char *fname,  size_t fname_len,
                                      T rval ) {
    return this->pack_real( fname, fname_len, (double) rval );
  }

  template< class T >
  RwfElementListWriter & append_int( const char *fname,  T ival ) {
    return this->pack_ival( fname, ::strlen( fname ), (int64_t) ival );
  }
  template< class T >
  RwfElementListWriter & append_uint( const char *fname,  T uval ) {
    return this->pack_uval( fname, ::strlen( fname ), (uint64_t) uval );
  }
  template< class T >
  RwfElementListWriter & append_real( const char *fname,  T rval ) {
    return this->pack_real( fname, ::strlen( fname ), (double) rval );
  }
  template< class T >
  RwfElementListWriter & append_array( const char *fname,  T *ar,  size_t arsz,
                                       MDType el_t ) {
    return this->append_array( fname, ::strlen( fname ), ar, arsz, el_t );
  }
  template< class T >
  RwfElementListWriter & append_array( const char *fname,  size_t fname_len,  T *ar,
                                       size_t arsz,  MDType el_t ) {
    MDReference mref( ar, sizeof( T ) * arsz, MD_ARRAY, el_t, sizeof( ar[ 0 ] ) );
    return this->append_ref( fname, fname_len, mref );
  }

  RwfElementListWriter & append_string( const char *fname,  size_t fname_len,
                                        const char *str,  size_t len ) {
    MDReference mref( (void *) str, len, MD_STRING, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  RwfElementListWriter & append_string( const char *fname,  const char *str ) {
    return this->append_string( fname, ::strlen( fname ), str, ::strlen( str ));
  }
  RwfElementListWriter & append_array( const char *fname,  size_t fname_len,
                                       const char **str,  size_t arsz ) noexcept;
  RwfElementListWriter & append_array( const char *fname,  const char **str,
                                       size_t arsz ) {
    return this->append_array( fname, ::strlen( fname ), str, arsz );
  }
  RwfElementListWriter & append_array( const char *fname,  size_t fname_len,
                                       RwfQos *qos,  size_t arsz ) noexcept;
  RwfElementListWriter & append_array( const char *fname,  RwfQos *qos,
                                       size_t arsz ) {
    return this->append_array( fname, ::strlen( fname ), qos, arsz );
  }
  RwfElementListWriter & append_decimal( const char *fname,  size_t fname_len,
                                         MDDecimal &dec ) noexcept;
  RwfElementListWriter & append_time( const char *fname,  size_t fname_len,
                                      MDTime &time ) noexcept;
  RwfElementListWriter & append_date( const char *fname,  size_t fname_len,
                                      MDDate &date ) noexcept;
  RwfElementListWriter & append_decimal( const char *fname, MDDecimal &dec ) {
    return this->append_decimal( fname, ::strlen( fname ), dec );
  }
  RwfElementListWriter & append_time( const char *fname,  MDTime &time ) {
    return this->append_time( fname, ::strlen( fname ), time );
  }
  RwfElementListWriter & append_date( const char *fname,  MDDate &date ) {
    return this->append_date( fname, ::strlen( fname ), date );
  }
  RwfElementListWriter & append_state( const char *fname, RwfState &state ) {
    return this->append_state( fname, ::strlen( fname ), state );
  }
  RwfElementListWriter & append_state( const char *fname,  size_t fname_len,
                                       RwfState &state ) noexcept;
  RwfElementListWriter & append_set_ref( MDReference &mref ) noexcept;
  RwfElementListWriter & append_set_string( const char *str,  size_t len ) {
    MDReference mref( (void *) str, len, MD_STRING, md_endian );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_string( const char *str ) {
    return this->append_set_string( str, ::strlen( str ));
  }
  RwfElementListWriter & append_set_uint( uint64_t ival ) {
    MDReference mref( &ival, sizeof( ival ), MD_UINT );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_int( int64_t ival ) {
    MDReference mref( &ival, sizeof( ival ), MD_INT );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_real( double rval ) {
    MDReference mref( &rval, sizeof( rval ), MD_REAL );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_decimal( MDDecimal &dec ) {
    MDReference mref( &dec, sizeof( MDDecimal ), MD_DECIMAL );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_time( MDTime &time ) {
    MDReference mref( &time, sizeof( MDTime ), MD_TIME );
    return this->append_set_ref( mref );
  }
  RwfElementListWriter & append_set_date( MDDate &date ) {
    MDReference mref( &date, sizeof( MDDate ), MD_DATE );
    return this->append_set_ref( mref );
  }
  template <class WRITER, class... Ts>
  RwfElementListWriter &apply( WRITER &writer,
       RwfElementListWriter &(WRITER::*cb)( RwfElementListWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

struct RwfMapWriter : RwfMsgWriterBase {
  MDType   key_ftype;
  uint8_t  container_type;
  MDFid    key_fid;
  uint32_t nitems,
           hint_cnt;
  size_t   field_defn_size,
           summary_size;
  RwfFieldDefnWriter * field_defn;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfMapWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len,  MDType kt = MD_UINT,
                uint8_t ct = RWF_CONTAINER_BASE,  MDFid fid = 0,
                uint32_t hcnt = 0 )
    : RwfMsgWriterBase( W_MAP, m, d, bb, len ), key_ftype( kt ),
      container_type( ct ),  key_fid( fid ), hint_cnt( hcnt ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 5 + ( this->key_fid != 0 ? 1 : 0 ) );
    this->nitems          = 0;
    this->field_defn_size = 0;
    this->summary_size    = 0;
    this->field_defn      = NULL;
  }
  RwfMapWriter &set_key_type( MDType t ) { this->key_ftype = t; return *this; }
  RwfMapWriter &set_key_fid( MDFid f ) { this->key_fid = f; return *this; }
  size_t update_hdr( void ) noexcept;
  RwfFieldDefnWriter   & add_field_defn( void ) noexcept;
  bool check_container( RwfMsgWriterBase &base,  bool is_summary ) noexcept;
  void add_action_entry( RwfMapAction action,  MDReference &key,
                         RwfMsgWriterBase &base ) noexcept;
  RwfFieldListWriter   & add_summary_field_list  ( void ) noexcept;
  RwfElementListWriter & add_summary_element_list( void ) noexcept;
  RwfMapWriter         & add_summary_map         ( void ) noexcept;
  RwfFilterListWriter  & add_summary_filter_list ( void ) noexcept;
  RwfSeriesWriter      & add_summary_series      ( void ) noexcept;
  RwfVectorWriter      & add_summary_vector      ( void ) noexcept;

  RwfFieldListWriter   & add_field_list  ( RwfMapAction action, MDReference &mref ) noexcept;
  RwfElementListWriter & add_element_list( RwfMapAction action, MDReference &mref ) noexcept;
  RwfMapWriter         & add_map         ( RwfMapAction action, MDReference &mref ) noexcept;
  RwfFilterListWriter  & add_filter_list ( RwfMapAction action, MDReference &mref ) noexcept;
  RwfSeriesWriter      & add_series      ( RwfMapAction action, MDReference &mref ) noexcept;
  RwfVectorWriter      & add_vector      ( RwfMapAction action, MDReference &mref ) noexcept;

  template<class T>
  RwfFieldListWriter & add_field_list( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_field_list( action, key );
  }
  template<class T>
  RwfElementListWriter & add_element_list( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_element_list( action, key );
  }
  template<class T>
  RwfMapWriter & add_map( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_map( action, key );
  }
  template<class T>
  RwfFilterListWriter & add_filter_list( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_filter_list( action, key );
  }
  template<class T>
  RwfSeriesWriter & add_series( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_series( action, key );
  }
  template<class T>
  RwfVectorWriter & add_vector( RwfMapAction action,  T val,  MDType t ) {
    MDReference key( (void *) &val, sizeof( val ), t, md_endian );
    return this->add_vector( action, key );
  }
  RwfFieldListWriter & add_field_list( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_field_list( action, key );
  }
  RwfElementListWriter & add_element_list( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_element_list( action, key );
  }
  RwfMapWriter & add_map( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_map( action, key );
  }
  RwfFilterListWriter & add_filter_list( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_filter_list( action, key );
  }
  RwfSeriesWriter & add_series( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_series( action, key );
  }
  RwfVectorWriter & add_vector( RwfMapAction action,  const char *str ) {
    MDReference key( (char *) str, ::strlen( str ), MD_STRING, md_endian );
    return this->add_vector( action, key );
  }
  int append_key( RwfMapAction action,  MDReference &mref ) noexcept;
  int key_ival( RwfMapAction action, int64_t i64 ) noexcept;
  int key_uval( RwfMapAction action, uint64_t i64 ) noexcept;
  int key_time( RwfMapAction action, MDTime &time ) noexcept;
  int key_date( RwfMapAction action, MDDate &date ) noexcept;
  int key_decimal( RwfMapAction action, MDDecimal &dec ) noexcept;
  template <class WRITER, class... Ts>
  RwfMapWriter &apply( WRITER &writer,
       RwfMapWriter &(WRITER::*cb)( RwfMapWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

struct RwfFilterListWriter : RwfMsgWriterBase {
  uint8_t  container_type;
  uint32_t nitems;
  uint32_t hint_cnt;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfFilterListWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len,
                       uint8_t ct = RWF_CONTAINER_BASE,  uint32_t hcnt = 0 )
    : RwfMsgWriterBase( W_FILTER_LIST, m, d, bb, len ),
      container_type( ct ), hint_cnt( hcnt ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 3 + ( this->hint_cnt != 0 ? 1 : 0 ) );
    this->nitems = 0;
  }
  size_t update_hdr( void ) noexcept;
  uint8_t check_container( RwfMsgWriterBase &base ) noexcept;
  void add_action_entry( RwfFilterAction action,  uint8_t id,
                         RwfMsgWriterBase &base ) noexcept;
  RwfFieldListWriter   & add_field_list  ( RwfFilterAction action, uint8_t id ) noexcept;
  RwfElementListWriter & add_element_list( RwfFilterAction action, uint8_t id ) noexcept;
  RwfMapWriter         & add_map         ( RwfFilterAction action, uint8_t id ) noexcept;
  RwfFilterListWriter  & add_filter_list ( RwfFilterAction action, uint8_t id ) noexcept;
  RwfSeriesWriter      & add_series      ( RwfFilterAction action, uint8_t id ) noexcept;
  RwfVectorWriter      & add_vector      ( RwfFilterAction action, uint8_t id ) noexcept;
  template <class WRITER, class... Ts>
  RwfFilterListWriter &apply( WRITER &writer,
       RwfFilterListWriter &(WRITER::*cb)( RwfFilterListWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

static const uint8_t DICT_VERB_NORMAL  = 0x7,
                     DICT_VERB_MINIMAL = 0x3,
                     DICT_VERB_MAXIMUM = 0xf;

struct RwfSeriesWriter : RwfMsgWriterBase {
  uint8_t  container_type;
  uint32_t nitems,
           hint_cnt;
  size_t   field_defn_size,
           summary_size;
  RwfFieldDefnWriter * field_defn;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfSeriesWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len,
                   uint8_t ct = RWF_CONTAINER_BASE,  uint32_t hcnt = 0 )
    : RwfMsgWriterBase( W_SERIES, m, d, bb, len ),
      container_type( ct ), hint_cnt( hcnt ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 4 + ( this->hint_cnt != 0 ? 4 : 0 ) );
    this->nitems          = 0;
    this->field_defn_size = 0;
    this->summary_size    = 0;
    this->field_defn      = NULL;
  }
  size_t update_hdr( void ) noexcept;
  RwfSeriesWriter & set_error( int e ) {
    this->error( e );
    return *this;
  }
  bool check_container( RwfMsgWriterBase &base,  bool is_summary ) noexcept;
  RwfFieldDefnWriter   & add_field_defn( void ) noexcept;

  RwfFieldListWriter   & add_summary_field_list  ( void ) noexcept;
  RwfElementListWriter & add_summary_element_list( void ) noexcept;
  RwfMapWriter         & add_summary_map         ( void ) noexcept;
  RwfFilterListWriter  & add_summary_filter_list ( void ) noexcept;
  RwfSeriesWriter      & add_summary_series      ( void ) noexcept;
  RwfVectorWriter      & add_summary_vector      ( void ) noexcept;

  RwfFieldListWriter   & add_field_list  ( void ) noexcept;
  RwfElementListWriter & add_element_list( void ) noexcept;
  RwfMapWriter         & add_map         ( void ) noexcept;
  RwfFilterListWriter  & add_filter_list ( void ) noexcept;
  RwfSeriesWriter      & add_series      ( void ) noexcept;
  RwfVectorWriter      & add_vector      ( void ) noexcept;

  RwfSeriesWriter      & encode_field_dictionary( MDFid start_fid,
                                                  MDFid &end_fid,
                                                  uint8_t verbosity,
                                                  bool add_summary,
                                                  size_t buflen ) noexcept;
  RwfSeriesWriter      & encode_enum_dictionary( uint32_t start_mapnum,
                                                 uint32_t &end_mapnum,
                                                 uint8_t verbosity,
                                                 bool add_summary,
                                                 size_t buflen ) noexcept;
  template <class WRITER, class... Ts>
  RwfSeriesWriter &apply( WRITER &writer,
       RwfSeriesWriter &(WRITER::*cb)( RwfSeriesWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

struct RwfVectorWriter : RwfMsgWriterBase {
  uint8_t  container_type;
  uint32_t nitems;
  uint32_t hint_cnt;
  size_t   field_defn_size,
           summary_size;
  RwfFieldDefnWriter * field_defn;

  void * operator new( size_t, void *ptr ) { return ptr; }
  RwfVectorWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len,
                   uint8_t ct = RWF_CONTAINER_BASE,  uint32_t hcnt = 0 )
    : RwfMsgWriterBase( W_VECTOR, m, d, bb, len ),
      container_type( ct ), hint_cnt( hcnt ) {
    this->reset();
  }
  void reset( void ) {
    this->RwfMsgWriterBase::reset( 4 + ( this->hint_cnt != 0 ? 4 : 0 ) );
    this->nitems          = 0;
    this->field_defn_size = 0;
    this->summary_size    = 0;
    this->field_defn      = NULL;
  }
  size_t update_hdr( void ) noexcept;
  bool check_container( RwfMsgWriterBase &base,  bool is_summary ) noexcept;
  void add_action_entry( RwfVectorAction action,  uint32_t index,
                         RwfMsgWriterBase &base ) noexcept;
  RwfFieldListWriter   & add_summary_field_list  ( void ) noexcept;
  RwfElementListWriter & add_summary_element_list( void ) noexcept;
  RwfMapWriter         & add_summary_map         ( void ) noexcept;
  RwfFilterListWriter  & add_summary_filter_list ( void ) noexcept;
  RwfSeriesWriter      & add_summary_series      ( void ) noexcept;
  RwfVectorWriter      & add_summary_vector      ( void ) noexcept;

  RwfFieldListWriter   & add_field_list  ( RwfVectorAction action, uint32_t index ) noexcept;
  RwfElementListWriter & add_element_list( RwfVectorAction action, uint32_t index ) noexcept;
  RwfMapWriter         & add_map         ( RwfVectorAction action, uint32_t index ) noexcept;
  RwfFilterListWriter  & add_filter_list ( RwfVectorAction action, uint32_t index ) noexcept;
  RwfSeriesWriter      & add_series      ( RwfVectorAction action, uint32_t index ) noexcept;
  RwfVectorWriter      & add_vector      ( RwfVectorAction action, uint32_t index ) noexcept;
  template <class WRITER, class... Ts>
  RwfVectorWriter &apply( WRITER &writer,
       RwfVectorWriter &(WRITER::*cb)( RwfVectorWriter &me, Ts... ),
       Ts... args ) {
    return (writer.*cb)( *this, args... );
  }
};

}
} // namespace rai

#endif
