#include <stdio.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>
#include <raimd/sass.h>

using namespace rai;
using namespace md;

extern "C" {
MDMsgWriter_t * rwf_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,
                                       void *buf_ptr, size_t buf_sz,
                                       RwfMsgClass cl, RdmDomainType dom,
                                       uint32_t id )
{
  void * p = ((MDMsgMem *) mem)->make( sizeof( RwfMsgWriter ) );
  return new ( p ) RwfMsgWriter( *(MDMsgMem *) mem, (MDDict *) d, buf_ptr, buf_sz, cl, dom, id );
}

MDMsgWriter_t * rwf_msg_writer_field_list_create( MDMsgMem_t *mem,  MDDict_t *d,
                                                  void *buf_ptr, size_t buf_sz )
{
  void * p = ((MDMsgMem *) mem)->make( sizeof( RwfFieldListWriter ) );
  return new ( p ) RwfFieldListWriter( *(MDMsgMem *) mem, (MDDict *) d, buf_ptr, buf_sz );
}

int
md_msg_writer_rwf_add_seq_num( MDMsgWriter_t *w, uint32_t seqno )
{
  RwfMsgWriter *x = (RwfMsgWriter *) w;
  if ( x->wr_type != RWF_MSG_TYPE_ID )
    return -1;
  x->add_seq_num( seqno );
  return 0;
}
int
md_msg_writer_rwf_add_msg_key( MDMsgWriter_t *w, const char *subj, size_t slen )
{
  RwfMsgWriter *x = (RwfMsgWriter *) w;
  if ( x->wr_type != RWF_MSG_TYPE_ID )
    return -1;
  x->add_msg_key()
    .name( subj, slen )
    .name_type( NAME_TYPE_RIC )
    .end_msg_key();
  return 0;
}

MDMsgWriter_t *
md_msg_writer_rwf_add_field_list( MDMsgWriter_t *w )
{
  RwfMsgWriter *x = (RwfMsgWriter *) w;
  if ( x->wr_type != RWF_MSG_TYPE_ID )
    return NULL;
  RwfFieldListWriter &y = x->add_field_list();
  return (MDMsgWriter_t *) &y;
}

int
md_msg_writer_rwf_add_flist( MDMsgWriter_t *fl, uint16_t flist )
{
  RwfFieldListWriter *x = (RwfFieldListWriter *) fl;
  if ( x->wr_type != RWF_FIELD_LIST_TYPE_ID )
    return -1;
  x->add_flist( flist );
  return 0;
}

int
md_msg_writer_rwf_end_msg( MDMsgWriter_t *w )
{
  RwfMsgWriter *x = (RwfMsgWriter *) w;
  if ( (uint32_t) x->wr_type != RWF_MSG_TYPE_ID )
    return -1;
  x->end_msg();
  return 0;
}

int
md_msg_writer_rwf_set( MDMsgWriter_t *w, RwfMsgSerial ser )
{
  RwfMsgWriter *x = (RwfMsgWriter *) w;
  if ( (uint32_t) x->wr_type != RWF_MSG_TYPE_ID )
    return -1;
  x->set( ser );
  return 0;
}

}

RwfMsgWriterBase::RwfMsgWriterBase( RwfWriterType t,  MDMsgMem &m,  MDDict *d,
                                    void *bb, size_t len ) noexcept : type( t )
{
  this->msg_mem     = &m;
  this->buf         = (uint8_t *) bb;
  this->off         = 0;
  this->buflen      = len;
  this->wr_type     = RWF_MSG_TYPE_ID;
  this->err         = 0;
  this->prefix_len  = 0;
  this->len_bits    = 0;
  this->size_ptr    = NULL;
  this->parent      = NULL;
  this->child       = NULL;
  this->is_complete = false;
  while ( d != NULL && d->dict_type[ 0 ] != 'a' ) /* look for app_a type */
    d = d->get_next();
  this->dict = d;
}

int
RwfMsgWriterBase::append_base( RwfMsgWriterBase &base,  int bits,
                               size_t *sz_ptr ) noexcept
{
  size_t len_bytes = ( bits == 0 ? 0 : bits == 15 ? 2 : 3 );
  if ( ! this->has_space( len_bytes ) )
    this->error( Err::NO_SPACE );
  if ( this->err != 0 ) {
    base.is_complete = true;
    return this->err;
  }
  base.parent   = this;
  base.dict     = this->dict;
  base.buf      = &this->buf[ this->off + len_bytes ];
  base.buflen   = this->buflen - ( this->off + len_bytes );
  base.len_bits = bits;
  base.size_ptr = sz_ptr;
  this->off    += len_bytes;
  return 0;
}

void
RwfMsgWriterHdr::update_len( RwfMsgWriterBase &base ) noexcept
{
  size_t len_bytes = 0,
         len       = base.off;
  if ( base.len_bits >= 15 ) {
    if ( base.len_bits == 15 ) {
      len_bytes = 2;
      set_u16<MD_BIG>( this->buf - 2, len | 0x8000 );
    }
    else {
      len_bytes = 3;
      this->buf[ -3 ] = 0xfe;
      set_u16<MD_BIG>( this->buf - 2, len );
    }
  }
  if ( base.size_ptr != NULL )
    base.size_ptr[ 0 ] += len + len_bytes;
}

bool
RwfMsgWriterBase::resize( size_t len ) noexcept
{
  static const size_t max_size = 0x3fffffff; /* 1 << 30 - 1 == 1073741823 */
  if ( this->is_complete || this->err != 0 )
    return false;
  RwfMsgWriterBase *p = this;
  for ( ; p->parent != NULL; p = p->parent )
    ;
  if ( len > max_size )
    return false;
  size_t old_len = p->prefix_len + p->buflen,
         new_len = old_len + ( len - this->off );
  if ( new_len > max_size )
    return false;
  if ( new_len < old_len * 2 )
    new_len = old_len * 2;
  else
    new_len += 1024;
  if ( new_len > max_size )
    new_len = max_size;
  uint8_t * old_buf = p->buf - p->prefix_len,
          * new_buf = old_buf;
  this->mem().extend( old_len, new_len, &new_buf );
  uint8_t * end = &new_buf[ new_len ];

  p->buf = &new_buf[ p->prefix_len ];
  p->buflen = end - p->buf;

  for ( RwfMsgWriterBase *c = p->child; c != NULL && ! c->is_complete; c = c->child ) {
    if ( c->buf >= old_buf && c->buf < &old_buf[ old_len ] ) {
      size_t off = c->buf - old_buf;
      c->buf = &new_buf[ off ];
      c->buflen = end - c->buf;
    }
  }
  return true;
}

int
RwfMsgWriterBase::error( int e ) noexcept
{
  if ( this->err == 0 )
    this->err = e;
  if ( this->parent != NULL )
    return this->parent->error( e );
  return e;
}

size_t
RwfMsgWriterBase::complete( void ) noexcept
{
  size_t sz = this->off;
  switch ( this->type ) {
    default:
    case W_NONE:         break;
    case W_MSG_KEY:      sz = ((RwfMsgKeyWriter *) this)->update_hdr(); break;
    case W_FIELD_LIST:   sz = ((RwfFieldListWriter *) this)->update_hdr(); break;
    case W_ELEMENT_LIST: sz = ((RwfElementListWriter *) this)->update_hdr(); break;
    case W_FILTER_LIST:  sz = ((RwfFilterListWriter *) this)->update_hdr(); break;
    case W_VECTOR:       sz = ((RwfVectorWriter *) this)->update_hdr(); break;
    case W_MAP:          sz = ((RwfMapWriter *) this)->update_hdr(); break;
    case W_SERIES:       sz = ((RwfSeriesWriter *) this)->update_hdr(); break;
    case W_MSG:          sz = ((RwfMsgWriter *) this)->update_hdr(); break;
  }
  this->is_complete = true;
  return sz;
}

size_t
RwfMsgWriterBase::end( RwfWriterType t ) noexcept
{
  RwfMsgWriterBase *p = this;
  size_t sz = 0;
  while ( p->child != NULL && ! p->child->is_complete )
    p = p->child;
  for ( ; p != this; p = p->parent )
    p->complete();
  for ( p = this; p != NULL; p = p->parent ) {
    sz = p->off;
    if ( ! p->is_complete )
      sz = p->complete();
    if ( p->type == t )
      break;
  }
  return sz;
}

void
RwfMsgWriterBase::reset( size_t o,  size_t p ) noexcept
{
  this->off    = o;
  this->err    = 0;
  this->parent = NULL;
  this->is_complete = false;
  if ( p != this->prefix_len ) {
    if ( p > this->buflen + this->prefix_len )
      p = ( this->buflen + this->prefix_len ) / 2;
    this->buf    -= this->prefix_len;
    this->buf    += p;
    this->buflen += this->prefix_len;
    this->buflen -= p;
    this->prefix_len = p;
  }
}

void *
RwfMsgWriterBase::make_child( void ) noexcept
{
  if ( this->child != NULL ) {
    if ( ! this->child->is_complete )
      this->child->end( this->child->type );
    return this->child;
  }
  this->mem().alloc( sizeof( RwfMsgWriter ), &this->child );
  return this->child;
}

void
RwfMsgWriter::reset( void ) noexcept
{
  static uint16_t zero = 0;
  this->RwfMsgWriterBase::reset( 0, 64 );

  uint8_t *start = (uint8_t *) (void *) &this->msg_flags,
          *end   = (uint8_t *) (void *) &this[ 1 ];
  ::memset( start, 0, end - start );

  this->flags              = rwf_msg_always_present[ this->msg_class ];
  this->msg_key_off        = 0;
  this->msg_key_size       = 0;
  this->container_off      = 0;
  this->container_size     = 0;
  this->header_size        = 0;
  this->container_type     = RWF_CONTAINER_BASE;
  this->state.data_state   = DATA_STATE_OK;    /* always in refresh */
  this->state.stream_state = STREAM_STATE_OPEN;
  this->group_id.buf       = &zero;            /* always in refresh */
  this->group_id.len       = sizeof( zero );
}

size_t
RwfMsgWriter::size_upto_msg_key( void ) const noexcept
{
  size_t sz = 2 + /* header_size:u16 */
              1 + /* msg_class:u8 */
              1 + /* domain_type:u8 */
              4 + /* stream_id:u32 */
              2 + /* msg_flags:u16 */
              1;  /* container_type:u8 */
  switch ( this->msg_class ) {
    case REQUEST_MSG_CLASS:
      this->size_priority( sz )
           .size_qos( sz )
           .size_worst_qos( sz );
      break;
    case REFRESH_MSG_CLASS:
      this->size_seq_num( sz )
           .size_state( sz )
           .size_group_id( sz )
           .size_perm( sz )
           .size_qos( sz );
      break;
    case STATUS_MSG_CLASS:
      this->size_state( sz )
           .size_group_id( sz )
           .size_perm( sz );
      break;
    case UPDATE_MSG_CLASS:
      this->size_update_type( sz )
           .size_seq_num( sz )
           .size_conflate_info( sz )
           .size_perm( sz );
      break;
    case CLOSE_MSG_CLASS:
      break;
    case ACK_MSG_CLASS:
      this->size_ack_id( sz )
           .size_nak_code( sz )
           .size_text( sz )
           .size_seq_num( sz );
      break;
    case GENERIC_MSG_CLASS:
      this->size_seq_num( sz )
           .size_second_seq_num( sz )
           .size_perm( sz );
      break;
    case POST_MSG_CLASS:
      this->size_post_user( sz )
           .size_seq_num( sz )
           .size_post_id( sz )
           .size_perm( sz );
      break;
    default: break;
  }
  return sz;
}

size_t
RwfMsgWriter::size_after_msg_key( void ) const noexcept
{
  size_t sz = 0;
  switch ( this->msg_class ) {
    case REQUEST_MSG_CLASS:
      /*this->size_extended( sz )*/
      break;
    case REFRESH_MSG_CLASS:
      /*this->size_extended( sz )*/
      this->size_post_user( sz )
           .size_part_num( sz );
        /* .req_msg_key( sz ) */
      break;
    case STATUS_MSG_CLASS:
      /*this->size_extended( sz )*/
      this->size_post_user( sz );
        /* .req_msg_key( sz ) */
      break;
    case UPDATE_MSG_CLASS:
      /*this->size_extended( sz )*/
      this->size_post_user( sz );
      break;
    case CLOSE_MSG_CLASS:
      /*this->size_extended( sz )*/
      break;
    case ACK_MSG_CLASS:
      /*this->size_extended( sz )*/
      break;
    case GENERIC_MSG_CLASS:
      /*this->size_extended( sz )*/
      this->size_part_num( sz );
        /* .req_msg_key( sz ) */
      break;
    case POST_MSG_CLASS:
      /*this->size_extended( sz )*/
      this->size_part_num( sz )
           .size_post_rights( sz );
      break;
    default: break;
  }
  return sz;
}

size_t
RwfMsgWriter::update_hdr( void ) noexcept
{
  this->msg_flags = rwf_flags_map[ this->msg_class ]->to_flags( this->flags );

  size_t sz = this->size_upto_msg_key(),
         af = this->size_after_msg_key();
  if ( this->off > 0 ) {
    if ( ! this->check_prefix( sz ) ) {
      this->error( Err::NO_SPACE );
      return 0;
    }
    this->buf        -= sz;
    this->prefix_len -= sz;
    this->off        += sz;
    this->buflen     += sz;
    if ( this->msg_key_size != 0 )
      this->msg_key_off += sz;
    if ( this->container_size != 0 )
      this->container_off += sz;
  }
  else {
    this->off = sz + af;
  }
  this->header_size = this->msg_key_off + this->msg_key_size + af;
  if ( this->container_off > this->header_size )
    this->header_size = this->container_off;
  if ( sz + af > this->header_size )
    this->header_size = sz + af;

  if ( this->header_size > this->off )
    this->off = this->header_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this, this->flags );
  hdr.u16( this->header_size - 2 ) /* not including the size field */
     .u8 ( this->msg_class )
     .u8 ( this->domain_type )
     .u32( this->stream_id )
     .u16( this->msg_flags | 0x8000 )
     .u8 ( this->container_type - RWF_CONTAINER_BASE );

  switch ( this->msg_class ) {
    case REQUEST_MSG_CLASS:
      hdr.set_priority( this->priority )
         .set_qos( this->qos )
         .set_worst_qos( this->worst_qos );
      break;
    case REFRESH_MSG_CLASS:
      hdr.set_seq_num( this->seq_num )
         .set_state( this->state )
         .set_group_id( this->group_id )
         .set_perm( this->perm )
         .set_qos( this->qos );
      break;
    case STATUS_MSG_CLASS:
      hdr.set_state( this->state )
         .set_group_id( this->group_id )
         .set_perm( this->perm );
      break;
    case UPDATE_MSG_CLASS:
      hdr.set_update_type( this->update_type )
         .set_seq_num( this->seq_num )
         .set_conflate_info( this->conf_info )
         .set_perm( this->perm );
      break;
    case CLOSE_MSG_CLASS:
      /*hdr.set_extended();*/
      break;
    case ACK_MSG_CLASS:
      hdr.set_ack_id( this->ack_id )
         .set_nak_code( this->nak_code )
         .set_text( this->text )
         .set_seq_num( this->seq_num );
      break;
    case GENERIC_MSG_CLASS:
      hdr.set_seq_num( this->seq_num )
         .set_second_seq_num( this->second_seq_num )
         .set_perm( this->perm );
      break;
    case POST_MSG_CLASS:
      hdr.set_post_user( this->post_user )
         .set_seq_num( this->seq_num )
         .set_post_id( this->post_id )
         .set_perm( this->perm );
      break;
    default: break;
  }
  if ( af != 0 ) {
    hdr.seek( this->header_size - af );
    switch ( this->msg_class ) {
      case REQUEST_MSG_CLASS:
        /*this->size_extended( sz )*/
        break;
      case REFRESH_MSG_CLASS:
        /*this->size_extended( sz )*/
        hdr.set_post_user( this->post_user )
          /* .req_msg_key( sz ) */
           .set_part_num( this->part_num );
        break;
      case STATUS_MSG_CLASS:
        /*this->size_extended( sz )*/
        hdr.set_post_user( this->post_user );
          /* .req_msg_key( sz ) */
        break;
      case UPDATE_MSG_CLASS:
        /*this->size_extended( sz )*/
        hdr.set_post_user( this->post_user );
        break;
      case CLOSE_MSG_CLASS:
        /*this->size_extended( sz )*/
        break;
      case ACK_MSG_CLASS:
        /*this->size_extended( sz )*/
        break;
      case GENERIC_MSG_CLASS:
        /*this->size_extended( sz )*/
        hdr.set_part_num( this->part_num );
          /* .req_msg_key( sz ) */
        break;
      case POST_MSG_CLASS:
        /*this->size_extended( sz )*/
        hdr.set_part_num( this->part_num )
           .set_post_rights( this->post_rights );
        break;
      default: break;
    }
  }
  return this->off;
}

bool
RwfMsgWriter::check_container( RwfMsgWriterBase &base ) noexcept
{
  if ( base.type == W_MSG_KEY ) {
    if ( this->msg_key_size != 0 ) {
      this->error( Err::INVALID_MSG );
      base.is_complete = true;
      return false;
    }
  }
  else if ( this->container_type != RWF_CONTAINER_BASE ) {
    this->error( Err::INVALID_MSG );
    base.is_complete = true;
    return false;
  }
  return true;
}

RwfMsgKeyWriter &
RwfMsgWriter::add_msg_key( void ) noexcept
{
  RwfMsgKeyWriter * msg_key =
    new ( this->make_child() )
      RwfMsgKeyWriter( this->mem(), this->dict, NULL, 0 );
  if ( this->check_container( *msg_key ) ) {
    this->set( X_HAS_MSG_KEY );
    this->msg_key_off = this->off;
    this->append_base( *msg_key, 15, &this->msg_key_size );
  }
  return *msg_key;
}

template<class T>
static T &
add_msg_container( RwfMsgWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container ) ) {
    w.container_type = container->type;
    w.container_off  = ( w.off += w.size_after_msg_key() );
    w.append_base( *container, 0, &w.container_size );
  }
  return *container;
}

RwfFieldListWriter   & RwfMsgWriter::add_field_list  ( void ) noexcept { return add_msg_container<RwfFieldListWriter>( *this ); }
RwfElementListWriter & RwfMsgWriter::add_element_list( void ) noexcept { return add_msg_container<RwfElementListWriter>( *this ); }
RwfMapWriter         & RwfMsgWriter::add_map( MDType key_ty ) noexcept { return add_msg_container<RwfMapWriter>( *this ).set_key_type( key_ty ); }
RwfFilterListWriter  & RwfMsgWriter::add_filter_list ( void ) noexcept { return add_msg_container<RwfFilterListWriter>( *this ); }
RwfSeriesWriter      & RwfMsgWriter::add_series      ( void ) noexcept { return add_msg_container<RwfSeriesWriter>( *this ); }
RwfVectorWriter      & RwfMsgWriter::add_vector      ( void ) noexcept { return add_msg_container<RwfVectorWriter>( *this ); }

size_t
RwfMsgKeyWriter::update_hdr( void ) noexcept
{
  size_t hdr_size = 1;
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8( this->key_flags );
  return this->off;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::order_error( int fl ) noexcept
{
  fprintf( stderr, "msg_key field %s added out of order\n",
           rwf_msg_serial_str[ fl ] );
  return this->set_error( Err::BAD_FIELD );
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::service_id( uint16_t service_id ) noexcept
{
  if ( ! this->has_space( 2 ) )
    return this->set_error( Err::NO_SPACE );
  if ( this->key_flags != 0 )
    return this->order_error( X_HAS_SERVICE_ID );
  this->key_flags |= RwfMsgKey::HAS_SERVICE_ID;
  this->z16( service_id );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::name( const char *nm,  size_t nm_size ) noexcept
{
  if ( ! this->has_space( nm_size + 1 ) )
    return this->set_error( Err::NO_SPACE );
  if ( ( this->key_flags & ~( RwfMsgKey::HAS_NAME - 1 ) ) != 0 )
    return this->order_error( X_HAS_NAME );
  this->key_flags |= RwfMsgKey::HAS_NAME;
  this->u8( nm_size )
       .b ( nm, nm_size );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::name_type( uint8_t name_type ) noexcept
{
  if ( ! this->has_space( 1 ) )
    return this->set_error( Err::NO_SPACE );
  if ( ( this->key_flags & ~( RwfMsgKey::HAS_NAME_TYPE - 1 ) ) != 0 )
    return this->order_error( X_HAS_NAME_TYPE );
  this->key_flags |= RwfMsgKey::HAS_NAME_TYPE;
  this->u8( name_type );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::filter( uint32_t filter ) noexcept
{
  if ( ! this->has_space( 4 ) )
    return this->set_error( Err::NO_SPACE );
  if ( ( this->key_flags & ~( RwfMsgKey::HAS_FILTER - 1 ) ) != 0 )
    return this->order_error( X_HAS_FILTER );
  this->key_flags |= RwfMsgKey::HAS_FILTER;
  this->u32( filter );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::identifier( uint32_t id ) noexcept
{
  if ( ! this->has_space( 4 ) )
    return this->set_error( Err::NO_SPACE );
  if ( ( this->key_flags & ~( RwfMsgKey::HAS_IDENTIFIER - 1 ) ) != 0 )
    return this->order_error( X_HAS_IDENTIFIER );
  this->key_flags |= RwfMsgKey::HAS_IDENTIFIER;
  this->u32( id );
  return *this;
}

RwfElementListWriter &
RwfMsgKeyWriter::attrib( void ) noexcept
{
  RwfElementListWriter * elem_list =
    new ( this->make_child() )
      RwfElementListWriter( this->mem(), this->dict, NULL, 0 );
    
  if ( ( this->key_flags & ~( RwfMsgKey::HAS_ATTRIB - 1 )) != 0 )
    this->order_error( X_HAS_ATTRIB );
  if ( this->has_space( 1 ) ) {
    this->key_flags |= RwfMsgKey::HAS_ATTRIB;
    this->u8( RWF_ELEMENT_LIST - RWF_CONTAINER_BASE );
    this->append_base( *elem_list, 15, NULL );
  }
  return *elem_list;
}

RwfFieldDefnWriter &
RwfFieldDefnWriter::add_defn( uint16_t id,  RwfFieldSetKind k ) noexcept
{
  if ( this->set != NULL )
    this->end_defn();
  this->parent.mem().alloc( sizeof( RwfFieldSetList ), &this->set );
  this->set->init( id, k );
  return *this;
}

RwfFieldDefnWriter &
RwfFieldDefnWriter::end_defn( void ) noexcept
{
  if ( this->tl != NULL )
    this->tl->next = this->set;
  else
    this->hd = this->set;
  this->tl  = this->set;
  this->set->next = NULL;
  this->set = NULL;
  this->num_sets++;
  return *this;
}

void
RwfFieldDefnWriter::end_field_defn( void ) noexcept
{
  if ( this->set != NULL )
    this->end_defn();

  size_t sz = 2; /* flags:u8 + num_sets:u8 */
  for ( RwfFieldSetList *p = this->hd; p != NULL; p = p->next )
    sz += p->size(); /* set_id:u15 + set_arity:u8 <defn> */

  RwfMsgWriterBase & hdr = this->parent;
  hdr.off = this->hdr_off;
  hdr.u16( 0x8000 | sz )
     .u8( 0 )
     .u8( this->num_sets );
  for ( RwfFieldSetList *p = this->hd; p != NULL; p = p->next )
    p->encode( hdr );

  if ( hdr.type == W_SERIES )
    ((RwfSeriesWriter &) hdr).field_defn_size = sz + 2; /* 2 for size */
  else if ( hdr.type == W_MAP )
    ((RwfMapWriter &) hdr).field_defn_size = sz + 2; /* 2 for size */
  else if ( hdr.type == W_VECTOR )
    ((RwfVectorWriter &) hdr).field_defn_size = sz + 2; /* 2 for size */
}

RwfFieldDefnWriter &
RwfFieldDefnWriter::append_defn( const char *fname,  uint8_t rwf_type ) noexcept
{
  RwfFieldSetList * p = this->set;
  size_t oldsz, newsz;

  if ( p->kind != DEFN_IS_ELEM_LIST ) {
    MDLookup by( fname, ::strlen( fname ) );
    if ( this->parent.dict == NULL || ! this->parent.dict->get( by ) )
      this->parent.error( Err::UNKNOWN_FID );
    else {
      oldsz = sizeof( RwfFieldSetList ) +
              sizeof( RwfFieldListSet::FieldEntry ) * p->fld_list_defn.count,
      newsz = oldsz + sizeof( RwfFieldListSet::FieldEntry );
      this->parent.mem().extend( oldsz, newsz, &p );
      p->add( by.fid, rwf_type );
    }
  }
  else {
    oldsz = sizeof( RwfFieldSetList ) +
            sizeof( RwfElementListSet::ElemEntry ) * p->elem_list_defn.count,
    newsz = oldsz + sizeof( RwfElementListSet::ElemEntry );
    this->parent.mem().extend( oldsz, newsz, &p );
    p->add( fname, rwf_type );
  }
  return *this;
}

void
RwfFieldSetList::init( uint16_t id,  RwfFieldSetKind k ) noexcept
{
  this->next = NULL;
  this->kind = k;
  if ( k == DEFN_IS_FIELD_LIST ) {
    this->fld_list_defn.count  = 0;
    this->fld_list_defn.set_id = id;
  }
  else {
    this->elem_list_defn.count  = 0;
    this->elem_list_defn.set_id = id;
  }
}

void
RwfFieldSetList::add( const char *fname,  uint8_t rwf_type ) noexcept
{
  uint32_t i = this->elem_list_defn.count++;
  this->elem_list_defn.entry[ i ].name     = (char *) fname;
  this->elem_list_defn.entry[ i ].name_len = ::strlen( fname );
  this->elem_list_defn.entry[ i ].type     = rwf_type;
}

void
RwfFieldSetList::add( uint16_t fid,  uint8_t rwf_type ) noexcept
{
  uint32_t i = this->fld_list_defn.count++;
  this->fld_list_defn.entry[ i ].fid  = fid;
  this->fld_list_defn.entry[ i ].type = rwf_type;
}

size_t
RwfFieldSetList::size( void ) noexcept
{
  size_t sz = 3; /* set_id:u15 + set_arity:u8 <defn> */
  if ( this->kind == DEFN_IS_FIELD_LIST ) {
    sz += this->fld_list_defn.count * ( 2 + 1 ); /* fid:u16 type:u8 */
  }
  else {
    for ( uint32_t i = 0; i < this->elem_list_defn.count; i++ ) {
      uint16_t len = this->elem_list_defn.entry[ i ].name_len;
      sz += len + get_u15_prefix_len( len ) + 1; /* name:u15 type:u8 */
    }
  }
  return sz;
}

void
RwfFieldSetList::encode( RwfMsgWriterBase &hdr ) noexcept
{
  uint32_t i;
  if ( this->kind == DEFN_IS_FIELD_LIST ) {
    hdr.u16( 0x8000 | this->fld_list_defn.set_id )
       .u8( this->fld_list_defn.count );
    for ( i = 0; i < this->fld_list_defn.count; i++ ) {
      hdr.u16( this->fld_list_defn.entry[ i ].fid )
         .u8 ( this->fld_list_defn.entry[ i ].type );
    }
  }
  else {
    hdr.u16( 0x8000 | this->elem_list_defn.set_id )
       .u8( this->elem_list_defn.count );
    for ( i = 0; i < this->elem_list_defn.count; i++ ) {
      uint16_t len = this->elem_list_defn.entry[ i ].name_len;
      hdr.u15( len )
         .b  ( this->elem_list_defn.entry[ i ].name, len )
         .u8 ( this->elem_list_defn.entry[ i ].type );
    }
  }
}

int
RwfMsgWriterBase::pack_mref( uint8_t type,  MDReference &mref ) noexcept
{
  MDValue val;
  char    tmp_buf[ 64 ];
  size_t  tmp_size = rwf_fixed_size( (RWF_type) type );
  int     status = 0;

  if ( ! this->has_space( tmp_size ) )
    return Err::NO_SPACE;

  switch ( type ) {
    case RWF_ASCII_STRING:
    case RWF_UTF8_STRING:
    case RWF_RMTES_STRING:
      if ( mref.ftype != MD_STRING && mref.ftype != MD_OPAQUE ) {
        tmp_size = sizeof( tmp_buf );
        val.str  = tmp_buf;
        status   = to_string( mref, tmp_buf, tmp_size );
      }
      else {
        tmp_size = mref.fsize;
        val.str  = (char *) mref.fptr;
      }
      if ( status == 0 ) {
        if ( ! this->has_space( get_u15_prefix_len( tmp_size ) + tmp_size ) )
          status = Err::NO_SPACE;
        else {
          this->u15( tmp_size )
               .b( val.str, tmp_size );
        }
      }
      break;
    case RWF_INT_1:
      if ( (status = cvt_number( mref, val.i8 )) == 0 )
        this->u8( val.i8 );
      break;
    case RWF_UINT_1:
      if ( (status = cvt_number( mref, val.u8 )) == 0 )
        this->u8( val.u8 );
      break;
    case RWF_INT_2:
      if ( (status = cvt_number( mref, val.i16 )) == 0 )
        this->u16( val.i16 );
      break;
    case RWF_UINT_2:
      if ( (status = cvt_number( mref, val.u16 )) == 0 )
        this->u16( val.u16 );
      break;
    case RWF_INT_4:
      if ( (status = cvt_number( mref, val.i32 )) == 0 )
        this->u32( val.i32 );
      break;
    case RWF_UINT_4:
      if ( (status = cvt_number( mref, val.u32 )) == 0 )
        this->u32( val.u32 );
      break;
    case RWF_INT_8:
      if ( (status = cvt_number( mref, val.i64 )) == 0 )
        this->u64( val.i64 );
      break;
    case RWF_UINT_8:
      if ( (status = cvt_number( mref, val.u64 )) == 0 )
        this->u64( val.u64 );
      break;
    case RWF_FLOAT_4:
      if ( (status = cvt_number( mref, val.f32 )) == 0 )
        this->f32( val.f32 );
      break;
    case RWF_DOUBLE_8:
      if ( (status = cvt_number( mref, val.f64 )) == 0 )
        this->f64( val.f64 );
      break;
    case RWF_REAL:
    case RWF_REAL_4RB:
    case RWF_REAL_8RB: {
      MDDecimal dec;
      if ( (status = dec.get_decimal( mref )) != 0 )
        break;
      tmp_size = 1;
      if ( dec.hint > MD_DEC_NULL || dec.hint < MD_DEC_NNAN ) {
        tmp_size += int_size( dec.ival );

        if ( type == RWF_REAL_4RB ) { /* 1, 2, 3, 4, 5 */
          if ( tmp_size > 5 ) {
            while ( dec.ival > (int64_t) (uint64_t) 0xffffffffU )
              dec.degrade();
            tmp_size = 5;
          }
        }
        else if ( type == RWF_REAL_8RB ) {
          if ( ( tmp_size & 1 ) == 0 ) /* 1, 3, 5, 7, 9 */
            tmp_size++;
        }
      }
      if ( ! this->has_space( tmp_size ) ) {
        status = Err::NO_SPACE;
        break;
      }
      uint8_t rwf_hint = md_to_rwf_decimal_hint( dec.hint );
      if ( type == RWF_REAL ) {
        this->u8( tmp_size )
             .u8( rwf_hint );
        if ( tmp_size > 1 )
          this->i( dec.ival, tmp_size - 1 );
        break;
      }
      if ( --tmp_size == 0 )
        this->u8( 0x20 );
      else {
        static const uint8_t n32_prefix[] = { 0, 0, 0x40, 0x80, 0xC0 };
        uint8_t prefix = type == RWF_REAL_4RB ?
                         n32_prefix[ tmp_size ] : n32_prefix[ tmp_size / 2 ];
        this->u8( prefix | rwf_hint ).i( dec.ival, tmp_size );
      }
      break;
    }
    case RWF_ARRAY: {
      size_t len = mref.fsize + 4 + get_fe_prefix_len( mref.fsize + 4 );
      if ( ! this->has_space( len ) )
        return Err::NO_SPACE;
      this->z16( mref.fsize + 4 )
           .u8 ( md_type_to_rwf_primitive_type( mref.fentrytp ) )
           .u8 ( mref.fentrysz )
           .u16( mref.fsize / mref.fentrysz )
           .b  ( mref.fptr, mref.fsize );

      if ( mref.fendian != MD_BIG && mref.fentrysz > 1 &&
           is_endian_type( mref.fentrytp ) ) {
        size_t end = this->off,
               i   = end - mref.fsize;
        for ( ; i < end; i += mref.fentrysz )
          this->uN2( &this->buf[ i ], mref.fentrysz, &this->buf[ i ] );
      }
      break;
    }
    case RWF_DATE_4: {
      MDDate date;
      if ( (status = date.get_date( mref )) != 0 )
        break;
      if ( ! this->has_space( 4 ) )
        return Err::NO_SPACE;
      this->u8( date.day )
           .u8( date.mon )
           .u16( date.year );
      break;
    }
    case RWF_TIME_3:
    case RWF_TIME_5:
    case RWF_TIME_7:
    case RWF_TIME_8: {
      MDTime time;
      if ( (status = time.get_time( mref )) != 0 )
        break;
      if ( time.is_null() ) {
        time.hour     = 0xff;
        time.minute   = 0xff;
        time.sec      = 0xff;
        time.fraction = 0;
      }
      this->u8( time.hour )
           .u8( time.minute )
           .u8( time.sec );

      if ( type != RWF_TIME_3 ) {
        uint16_t ms = 0, us = 0, ns = 0;
        if ( time.resolution == MD_RES_MILLISECS )
          ms = (uint16_t) time.fraction;
        else if ( time.resolution == MD_RES_MICROSECS ) {
          us = (uint16_t) ( time.fraction % 1000 );
          ms = (uint16_t) ( time.fraction / 1000 );
        }
        else if ( time.resolution == MD_RES_NANOSECS ) {
          ns = (uint16_t) ( time.fraction % 1000 );
          us = (uint16_t) ( ( time.fraction / 1000 ) % 1000 );
          ms = (uint16_t) ( time.fraction / 1000000 );
        }
        if ( type == RWF_TIME_5 )
          this->u16( ms );
        else if ( type == RWF_TIME_7 )
          this->u16( ms )
               .u16( us );
        else if ( type == RWF_TIME_8 ) {
          us |= ( ns & 0xff00 ) << 3; /* shift 3 top bits into micro */
          ns  &= 0xff;
          this->u16( ms )
               .u16( us )
               .u8 ( ns );
        }
      }
      break;
    }
    case RWF_DATETIME_7:
    case RWF_DATETIME_9:
    case RWF_DATETIME_11:
    case RWF_DATETIME_12:
    default:
      status = Err::BAD_CVT_STRING;
      break;
  }
  return status;
}

size_t
RwfMsgWriterBase::time_size( const MDTime &time ) noexcept
{
  if ( ( time.resolution & MD_RES_NULL ) != 0 )
    return 0+1;
  switch ( time.resolution ) {
    case MD_RES_MINUTES:   return 1+2; /* 2 h:m */
    case MD_RES_SECONDS:   return 1+3; /* 3 h:m:s */
    case MD_RES_MILLISECS: return 1+5; /* 5 h:m:s.ms */
    case MD_RES_MICROSECS: return 1+7; /* 7 h:m:s.ms.us */
    case MD_RES_NANOSECS:  return 1+8; /* 8 h:m:s.ms.us.n */
    default:               return 0+1; /* 0 */
  }
}

void
RwfMsgWriterBase::pack_time( size_t sz,  const MDTime &time ) noexcept
{
  uint8_t  hr   = time.hour,
           min  = time.minute,
           sec  = time.sec;
  uint32_t frac = time.fraction;
  if ( --sz == 0 ) {
    this->u8( 0 );
    return;
  }
  if ( ( time.resolution & MD_RES_NULL ) != 0 ) {
    hr   = 0;
    min  = 0;
    sec  = 0;
    frac = 0;
  }
  this->u8( sz )
       .u8( hr )
       .u8( min );
  switch ( sz ) {
    case 3: /* second h:m:s */
      this->u8 ( sec );
      break;
    case 5: /* millisec h:m:s.ms */
      this->u8 ( sec  )
           .u16( frac );
      break;
    case 7: /* microsec h:m:s.ms.us */
      this->u8 ( sec  )
           .u16( frac / 1000 )
           .u16( frac % 1000 );
      break;
    case 8: { /* nanosec h:m:s.ms.us.n */
      uint16_t milli = (uint16_t) ( frac / 1000000 ),
               nano  = (uint16_t) ( frac % 1000 ),
               micro = (uint16_t) ( frac % 1000000 ) / 1000;
      micro |= ( nano & 0xff00 ) << 3; /* shift 3 top bits into micro */
      nano  &= 0xff;
      this->u16( milli )
           .u16( micro )
           .u8 ( nano );
      break;
    }
    case 2: /* minutes */
    default: break;
  }
}

size_t
RwfFieldListWriter::update_hdr( void ) noexcept
{
/*
 * three cases:
 *
 * flags:u8  HAS_STANDARD_DATA | HAS_FIELD_LIST_INFO
 * info_size:u8 dict_id:u8 flist:u16
 * nflds:u16
 * data
 *
 * flags:u8  HAS_SET_ID | HAS_SET_DATA | HAS_FIELD_LIST_INFO
 * info_size:u8 dict_id:u8 flist:u16
 * set_id:u16
 * data
 *
 * flags:u8  HAS_SET_ID | HAS_SET_DATA | HAS_STANDARD_DATA | HAS_FIELD_LIST_INFO
 * info_size:u8 dict_id:u8 flist:u16
 * set_id:u16
 * set_size:u16
 * set_data
 * nflds:u16
 * data
*/
  size_t hdr_size = 7;
  if ( this->set_nflds < this->nflds )
    hdr_size += 2 + this->set_size + 2; /* set_id + set_size + set_data */

  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  if ( this->set_nflds != 0 ) {
    uint8_t flags = RwfFieldListHdr::HAS_FIELD_LIST_INFO |
                    RwfFieldListHdr::HAS_SET_ID |
                    RwfFieldListHdr::HAS_SET_DATA;
    if ( this->set_nflds < this->nflds )
      flags |= RwfFieldListHdr::HAS_STANDARD_DATA;

    hdr.u8( flags )
       .u8 ( 3 )  /* info size */
       .u8 ( this->dict_id )  /* dict id */
       .u16( this->flist )
       .u16( 0x8000 | this->set_id );
    if ( this->set_nflds < this->nflds )
      hdr.incr( this->set_size + 2 )
         .u16( this->nflds - this->set_nflds );
  }
  else {
    hdr.u8 ( RwfFieldListHdr::HAS_FIELD_LIST_INFO |
             RwfFieldListHdr::HAS_STANDARD_DATA )
       .u8 ( 3 )  /* info size */
       .u8 ( this->dict_id )  /* dict id */
       .u16( this->flist )
       .u16( this->nflds );
  }
  return this->off;
}

RwfFieldListWriter &
RwfFieldListWriter::use_field_set( uint16_t id ) noexcept
{
  if ( this->off == 7 ) {
    for ( RwfMsgWriterBase *p = this->parent; p != NULL; p = p->parent ) {
      RwfFieldDefnWriter * defn;
      switch ( p->type ) {
        case W_SERIES: defn = ((RwfSeriesWriter *) p )->field_defn; break;
        case W_MAP:    defn = ((RwfMapWriter *) p )->field_defn;    break;
        case W_VECTOR: defn = ((RwfVectorWriter *) p )->field_defn;  break;
        default:       defn = NULL; break;
      }
      if ( defn != NULL ) {
        for ( RwfFieldSetList *set = defn->hd; set != NULL; set = set->next ) {
          if ( set->kind == DEFN_IS_FIELD_LIST &&
               set->fld_list_defn.set_id == id ) {
            this->set_id = id;
            this->set    = &set->fld_list_defn;
            return *this;
          }
        }
      }
    }
  }
  return this->set_error( Err::BAD_FIELD );
}

bool
RwfFieldListWriter::match_set( MDFid fid ) noexcept
{
  if ( this->set_size == 0 && this->nflds < this->set->count ) {
    if ( fid == this->set->entry[ this->nflds ].fid )
      return true;
  }
  if ( this->set_size == 0 && this->set_nflds > 0 ) {
    if ( ! this->has_space( 4 ) ) {
      this->error( Err::NO_SPACE );
      return false;
    }
    size_t start = 7;
    this->set_size = this->off - start;
    ::memmove( &this->buf[ start + 2 ], &this->buf[ start ], this->set_size );
    this->seek( start )
         .u16( 0x8000 | this->set_size )
         .seek( start + this->set_size + 4 ); /* room for nflds */
  }
  return false;
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( const char *fname,  size_t fname_len,
                                 const void *ival,  size_t ilen,
                                 MDType t ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->unknown_fid();
  return this->append_ival( by, ival, ilen, t );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( MDFid fid,  const void *ival, size_t ilen,
                                 MDType t ) noexcept
{
  if ( this->set != NULL && this->match_set( fid ) )
    return this->append_set_ival( ival, ilen, t );
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->unknown_fid();
  return this->append_ival( by, ival, ilen, t );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( MDLookup &by,  const void *ival,  size_t ilen,
                                 MDType t ) noexcept
{
  if ( this->set != NULL && this->match_set( by.fid ) )
    return this->append_set_ival( ival, ilen, t );
  MDValue val;

  if ( by.ftype == MD_UINT || by.ftype == MD_ENUM || by.ftype == MD_BOOLEAN ) {
    val.u64 = 0;
    ::memcpy( &val.u64, ival, ilen );
    return this->pack_uval( by.fid, val.u64 );
  }
  if ( by.ftype == MD_INT ) {
    val.u64 = 0;
    ::memcpy( &val.u64, ival, ilen );
    if ( ilen == 1 )      val.i64 = (int64_t) val.i8; /* sign extend */
    else if ( ilen == 2 ) val.i64 = (int64_t) val.i16;
    else if ( ilen == 4 ) val.i64 = (int64_t) val.i32;
    return this->pack_ival( by.fid, val.i64 );
  }
  MDReference mref( (void *) ival, ilen, t, md_endian );
  return this->append_ref( by.fid, by.ftype, by.fsize, mref );
}

RwfFieldListWriter &
RwfFieldListWriter::pack_uval( MDFid fid,  uint64_t uval ) noexcept
{
  size_t ilen = uint_size( uval );
  if ( ! this->has_space( ilen + 3 ) )
    return this->set_error( Err::NO_SPACE );

  this->nflds++;
  this->u16( fid )
       .u8 ( ilen )
       .u  ( uval, ilen );
  return *this;
}

RwfFieldListWriter &
RwfFieldListWriter::pack_ival( MDFid fid,  int64_t ival ) noexcept
{
  size_t ilen = int_size( ival );
  if ( ! this->has_space( ilen + 3 ) )
    return this->set_error( Err::NO_SPACE );

  this->nflds++;
  this->u16( fid )
       .u8 ( ilen )
       .i  ( ival, ilen );
  return *this;
}

RwfFieldListWriter &
RwfFieldListWriter::pack_partial( MDFid fid,  const uint8_t *fptr,  size_t fsize,
                                  size_t foffset ) noexcept
{
  size_t partial_len = ( foffset > 100 ? 3 : foffset > 10 ? 2 : 1 );
  size_t len = fid_pack_size( fsize + partial_len + 3 );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nflds++;
  this->u16( fid )
       .z16( fsize + partial_len + 3 )
       .u8 ( 0x1b )
       .u8 ( '[' );
  if ( partial_len == 3 )
    this->u8( ( ( foffset / 100 ) % 10 ) + '0' );
  if ( partial_len >= 2 )
    this->u8( ( ( foffset / 10 ) % 10 ) + '0' );
  this->u8( ( foffset % 10 ) + '0' )
       .u8( '`' )
       .b ( fptr, fsize );
  return *this;
}

RwfFieldListWriter &
RwfFieldListWriter::append_ref( MDFid fid,  MDReference &mref ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->unknown_fid();
  return this->append_ref( fid, by.ftype, by.fsize, mref );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ref( const char *fname,  size_t fname_len,
                                MDReference &mref ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->unknown_fid();
  return this->append_ref( by.fid, by.ftype, by.fsize, mref );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ref( MDFid fid,  MDType ftype,  uint32_t fsize,
                                MDReference &mref ) noexcept
{
  if ( this->set != NULL && this->match_set( fid ) )
    return this->append_set_ref( mref );
  char      str_buf[ 64 ];
  uint8_t * fptr = mref.fptr;
  size_t    len, slen;
  MDValue   val;
  MDEndian  fendian = mref.fendian;

  switch ( ftype ) {
    case MD_BOOLEAN:
    case MD_ENUM:
    case MD_UINT:
      if ( cvt_number<uint64_t>( mref, val.u64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      return this->pack_uval( fid, val.u64 );

    case MD_INT:
      if ( cvt_number<int64_t>( mref, val.i64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      return this->pack_ival( fid, val.i64 );

    case MD_REAL:
      if ( cvt_number<double>( mref, val.f64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      if ( fsize == 4 )
        val.f32 = (float) val.f64;
      fptr = (uint8_t *) (void *) &val;
      fendian = md_endian;
      break;

    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      return this->append_time( fid, ftype, fsize, time );
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      return this->append_date( fid, ftype, fsize, date );
    }
    case MD_DECIMAL: {
      MDDecimal dec;
      dec.get_decimal( mref );
      return this->append_decimal( fid, ftype, fsize, dec );
    }
    case MD_ARRAY: {
      size_t len = fid_pack_size( fsize + 4 );
      if ( ! this->has_space( len ) )
        return this->set_error( Err::NO_SPACE );

      this->nflds++;
      this->u16( fid )
           .u8 ( md_type_to_rwf_primitive_type( ftype ) )
           .z16( fsize + 4 )
           .u8 ( md_type_to_rwf_primitive_type( mref.fentrytp ) )
           .u8 ( mref.fentrysz )
           .u16( fsize / mref.fentrysz )
           .b  ( mref.fptr, mref.fsize );

      if ( fendian != MD_BIG && mref.fentrysz > 1 &&
           is_endian_type( mref.fentrytp ) ) {
        size_t end = this->off,
               i   = end - mref.fsize;
        for ( ; i < end; i += mref.fentrysz )
          this->uN2( &this->buf[ i ], mref.fentrysz, &this->buf[ i ] );
      }
      return *this;
    }
    case MD_PARTIAL:
      slen = mref.fsize;
      if ( slen > fsize )
        slen = fsize;
      if ( mref.ftype == MD_PARTIAL && mref.fentrysz != 0 )
        return this->pack_partial( fid, fptr, slen, mref.fentrysz );
      /* FALLTHRU */
    case MD_OPAQUE:
    case MD_STRING: {
      if ( mref.ftype == MD_STRING || mref.ftype == MD_PARTIAL ||
           mref.ftype == MD_OPAQUE )
        slen = mref.fsize;
      else {
        slen = sizeof( str_buf );
        if ( to_string( mref, str_buf, slen ) != 0 )
          return this->set_error( Err::BAD_CVT_STRING );
        fptr = (uint8_t *) (void *) str_buf;
      }
      /*if ( slen < fsize )*/
        fsize = (uint32_t) slen;
      break;
    }
    default:
      return this->set_error( Err::BAD_CVT_NUMBER );
  }

  len = fid_pack_size( fsize );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nflds++;
  this->u16( fid )
       .z16( fsize );
  /* invert endian, for little -> big */
  if ( fendian != MD_BIG && is_endian_type( ftype ) )
    this->uN( fptr, fsize );
  else
    this->b( fptr, fsize );
  return *this;
}

RwfFieldListWriter &
RwfFieldListWriter::append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                                    MDDecimal &dec ) noexcept
{
  if ( this->set != NULL && this->match_set( fid ) )
    return this->append_set_decimal( dec );
  if ( ftype == MD_DECIMAL ) {
    size_t ilen = 1;
    if ( dec.hint > MD_DEC_NULL || dec.hint < MD_DEC_NNAN )
      ilen += int_size( dec.ival );

    if ( ! this->has_space( ilen + 3 ) )
      return this->set_error( Err::NO_SPACE );

    this->nflds++;
    this->u16( fid )
         .u8 ( ilen )
         .u8 ( md_to_rwf_decimal_hint( dec.hint ) );
    if ( ilen > 1 )
      this->i( dec.ival, ilen - 1 );
    return *this;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    MDReference mref( sbuf, dec.get_string( sbuf, sizeof( sbuf ) ),
                      MD_STRING, MD_BIG );
    return this->append_ref( fid, ftype, fsize, mref );
  }
  if ( ftype == MD_REAL ) {
    double fval;
    if ( dec.get_real( fval ) == 0 ) {
      MDReference mref( &fval, sizeof( fval ), MD_REAL, md_endian );
      return this->append_ref( fid, ftype, fsize, mref );
    }
  }
  return this->set_error( Err::BAD_CVT_NUMBER );
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                                 MDTime &time ) noexcept
{
  if ( this->set != NULL && this->match_set( fid ) )
    return this->append_set_time( time );
  if ( ftype == MD_TIME ) {
    size_t sz = this->time_size( time );
    if ( ! this->has_space( sz + 2 ) )
      return this->set_error( Err::NO_SPACE );
    this->nflds++;
    this->u16( fid )
         .pack_time( sz, time );
    return *this;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    MDReference mref( sbuf, time.get_string( sbuf, sizeof( sbuf ) ),
                      MD_STRING, MD_BIG );
    return this->append_ref( fid, ftype, fsize, mref );
  }
  return this->set_error( Err::BAD_TIME );
}

RwfFieldListWriter &
RwfFieldListWriter::append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                                 MDDate &date ) noexcept
{
  if ( this->set != NULL && this->match_set( fid ) )
    return this->append_set_date( date );
  if ( ftype == MD_DATE ) {
    if ( ! this->has_space( 7 ) )
      return this->set_error( Err::NO_SPACE );
    this->nflds++;
    this->u16( fid )
         .u8( 4 )
         .u8( date.day )
         .u8( date.mon )
         .u16( date.year );
    return *this;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    MDReference mref( sbuf, date.get_string( sbuf, sizeof( sbuf ) ),
                      MD_STRING, MD_BIG );
    return this->append_ref( fid, ftype, fsize, mref );
  }
  return this->set_error( Err::BAD_DATE );
}

RwfFieldListWriter &
RwfFieldListWriter::append_decimal( MDFid fid,  MDDecimal &dec ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->unknown_fid();
  return this->append_decimal( by.fid, by.ftype, by.fsize, dec );
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( MDFid fid,  MDTime &time ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->unknown_fid();
  return this->append_time( by.fid, by.ftype, by.fsize, time );
}

RwfFieldListWriter &
RwfFieldListWriter::append_date( MDFid fid,  MDDate &date ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->unknown_fid();
  return this->append_date( by.fid, by.ftype, by.fsize, date );
}

RwfFieldListWriter &
RwfFieldListWriter::append_decimal( const char *fname,  size_t fname_len,
                                    MDDecimal &dec ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->unknown_fid();
  return this->append_decimal( by.fid, by.ftype, by.fsize, dec );
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( const char *fname,  size_t fname_len,
                                 MDTime &time ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->unknown_fid();
  return this->append_time( by.fid, by.ftype, by.fsize, time );
}

RwfFieldListWriter &
RwfFieldListWriter::append_date( const char *fname,  size_t fname_len,
                                 MDDate &date ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->unknown_fid();
  return this->append_date( by.fid, by.ftype, by.fsize, date );
}

RwfFieldListWriter &
RwfFieldListWriter::append_set_ref( MDReference &mref ) noexcept
{
  uint32_t n = this->nflds;

  if ( this->set == NULL || n >= this->set->count )
    return this->set_error( Err::BAD_FIELD );

  uint8_t type   = this->set->entry[ n ].type;
  int     status = this->pack_mref( type, mref );

  if ( status != 0 )
    this->error( status );
  else {
    this->set_nflds++;
    this->nflds++;
  }
  return *this;
}

int
RwfFieldListWriter::convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept
{
  MDFieldIter *iter;
  int status;
  if ( (status = msg.get_field_iter( iter )) == 0 ) {
    if ( (status = iter->first()) == 0 ) {
      do {
        MDName      n;
        MDReference mref, href;
        MDEnum      enu;
        if ( (status = iter->get_name( n )) == 0 &&
             (status = iter->get_reference( mref )) == 0 ) {
          if ( skip_hdr && is_sass_hdr( n ) )
            continue;
          this->append_ref( n.fname, n.fnamelen, mref );
          status = this->err;
        }
        if ( status != 0 )
          break;
      } while ( (status = iter->next()) == 0 );
    }
  }
  if ( status != Err::NOT_FOUND )
    return status;
  return 0;
}

size_t
RwfElementListWriter::update_hdr( void ) noexcept
{
/*
 * three cases: ( minus list info )
 *
 * flags:u8   HAS_STANDARD_DATA
 * nitems:u16
 * data
 *
 * flags:u8   HAS_SET_ID | HAS_SET_DATA
 * set_id:u16
 * data
 *
 * flags:u8   HAS_SET_ID | HAS_SET_DATA | HAS_STANDARD_DATA
 * set_id:u16
 * set_size:u16
 * set_data
 * nitems:u16
 * data
*/
  size_t hdr_size = 3;
  if ( this->set_nitems < this->nitems )
    hdr_size += 2 + this->set_size + 2; /* set_id + set_size + set_data */

  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  if ( this->set_nitems != 0 ) {
    uint8_t flags = RwfElementListHdr::HAS_SET_ID |
                    RwfElementListHdr::HAS_SET_DATA;
    if ( this->set_nitems < this->nitems  )
      flags |= RwfElementListHdr::HAS_STANDARD_DATA;

    hdr.u8( flags )
       .u16( 0x8000 | this->set_id );
    if ( this->set_nitems < this->nitems )
      hdr.incr( this->set_size + 2 )
         .u16( this->nitems - this->set_nitems );
  }
  else {
    hdr.u8 ( RwfElementListHdr::HAS_STANDARD_DATA )
       .u16( this->nitems );
  }
  return this->off;
}

RwfElementListWriter &
RwfElementListWriter::use_field_set( uint16_t id ) noexcept
{
  if ( this->off == 3 ) {
    for ( RwfMsgWriterBase *p = this->parent; p != NULL; p = p->parent ) {
      RwfFieldDefnWriter * defn;
      switch ( p->type ) {
        case W_SERIES: defn = ((RwfSeriesWriter *) p )->field_defn; break;
        case W_MAP:    defn = ((RwfMapWriter *) p )->field_defn;    break;
        case W_VECTOR: defn = ((RwfVectorWriter *) p )->field_defn;  break;
        default:       defn = NULL; break;
      }
      if ( defn != NULL ) {
        for ( RwfFieldSetList *set = defn->hd; set != NULL; set = set->next ) {
          if ( set->kind == DEFN_IS_ELEM_LIST &&
               set->elem_list_defn.set_id == id ) {
            this->set_id = id;
            this->set    = &set->elem_list_defn;
            return *this;
          }
        }
      }
    }
  }
  return this->set_error( Err::BAD_FIELD );
}

bool
RwfElementListWriter::match_set( const char *fname,  size_t fname_len ) noexcept
{
  if ( this->set_size == 0 && this->nitems < this->set->count ) {
    if ( fname_len == this->set->entry[ this->nitems ].name_len &&
         ::memcmp( fname, this->set->entry[ this->nitems ].name,
                   fname_len ) == 0 ) {
      return true;
    }
  }
  if ( this->set_size == 0 && this->set_nitems > 0 ) {
    if ( ! this->has_space( 4 ) ) {
      this->error( Err::NO_SPACE );
      return false;
    }
    size_t start = 3;
    this->set_size = this->off - start;
    ::memmove( &this->buf[ start + 2 ], &this->buf[ start ], this->set_size );
    this->seek( start )
         .u16( 0x8000 | this->set_size )
         .seek( start + this->set_size + 4 ); /* reoom for nitems */
  }
  return false;
}

uint8_t
rai::md::md_type_to_rwf_primitive_type( MDType type ) noexcept
{
  switch ( type ) {
    case MD_INT:      return RWF_INT;
    case MD_BOOLEAN:
    case MD_UINT:     return RWF_UINT;
    case MD_REAL:     return RWF_DOUBLE;
    case MD_DECIMAL:  return RWF_REAL;
    case MD_DATE:     return RWF_DATE;
    case MD_TIME:     return RWF_TIME;
    case MD_DATETIME: return RWF_DATETIME;
    case MD_ENUM:     return RWF_ENUM;
    case MD_STRING:   return RWF_ASCII_STRING;
    case MD_ARRAY:    return RWF_ARRAY;
    case MD_OPAQUE:   return RWF_BUFFER;

    default:
      return RWF_BUFFER;
  }
}

RwfElementListWriter &
RwfElementListWriter::append_ref( const char *fname,  size_t fname_len,
                                  MDReference &mref ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_ref( mref );
  uint8_t * fptr    = mref.fptr;
  MDType    ftype   = mref.ftype;
  size_t    fsize   = mref.fsize;
  MDEndian  fendian = mref.fendian;
  MDValue   val;

  switch ( ftype ) {
    case MD_BOOLEAN:
    case MD_ENUM:
    case MD_UINT:
      if ( cvt_number<uint64_t>( mref, val.u64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      return this->pack_uval( fname, fname_len, val.u64 );

    case MD_INT:
      if ( cvt_number<int64_t>( mref, val.i64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      return this->pack_ival( fname, fname_len, val.i64 );

    case MD_REAL:
      if ( cvt_number<double>( mref, val.f64 ) != 0 )
        return this->set_error( Err::BAD_CVT_NUMBER );
      fptr    = (uint8_t *) (void *) &val.f64;
      fsize   = sizeof( double );
      fendian = md_endian;
      break;

    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      return this->append_time( fname, fname_len, time );
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      return this->append_date( fname, fname_len, date );
    }
    case MD_DECIMAL: {
      MDDecimal dec;
      dec.get_decimal( mref );
      return this->append_decimal( fname, fname_len, dec );
    }
    case MD_ARRAY: {
      size_t len = fname_pack_size( fname_len, fsize + 4 );
      if ( ! this->has_space( len ) )
        return this->set_error( Err::NO_SPACE );

      this->nitems++;
      this->u15( fname_len )
           .b  ( fname, fname_len )
           .u8 ( RWF_ARRAY )
           .z16( fsize + 4 )
           .u8 ( md_type_to_rwf_primitive_type( mref.fentrytp ) )
           .u8 ( mref.fentrysz )
           .u16( fsize / mref.fentrysz )
           .b  ( mref.fptr, mref.fsize );

      if ( fendian != MD_BIG && mref.fentrysz > 1 &&
           is_endian_type( mref.fentrytp ) ) {
        size_t end = this->off,
               i   = end - mref.fsize;
        for ( ; i < end; i += mref.fentrysz )
          this->uN2( &this->buf[ i ], mref.fentrysz, &this->buf[ i ] );
      }
      return *this;
    }
    case MD_PARTIAL: /* no partials in elem list */
    case MD_OPAQUE:
    case MD_STRING:
      break;

    default:
      return this->set_error( Err::BAD_CVT_NUMBER );
  }

  size_t len = fname_pack_size( fname_len, fsize );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( md_type_to_rwf_primitive_type( ftype ) )
       .z16( fsize );

  if ( fendian != MD_BIG && is_endian_type( ftype ) )
    this->uN( fptr, fsize );
  else
    this->b( fptr, fsize );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_array( const char *fname,  size_t fname_len,
                                    const char **str,  size_t arsz ) noexcept
{
  size_t len,
         sz  = 0, i;
  for ( i = 0; i < arsz; i++ ) {
    sz += 1 + ( str[ i ] != NULL ? ::strlen( str[ i ] ) : 0 );
  }
  len = fname_pack_size( fname_len, sz + 4 );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_ARRAY )
       .z16( sz + 4 )
       .u8 ( RWF_ASCII_STRING )
       .u8 ( 0 )
       .u16( arsz );

  for ( i = 0; i < arsz; i++ ) {
    sz = ( str[ i ] != NULL ? ::strlen( str[ i ] ) : 0 );
    this->u8( sz );
    if ( sz > 0 )
      this->b( str[ i ], sz );
  }
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_array( const char *fname,  size_t fname_len,
                                    RwfQos *qos,  size_t arsz ) noexcept
{
  size_t len,
         sz  = 0, i;
  for ( i = 0; i < arsz; i++ ) {
    sz += 1 + 1 + ( qos[ i ].timeliness > 2 ? 2 : 0 ) +
                  ( qos[ i ].rate       > 2 ? 2 : 0 );
  }
  len = fname_pack_size( fname_len, sz + 4 );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_ARRAY )
       .z16( sz + 4 )
       .u8 ( RWF_QOS )
       .u8 ( 0 )
       .u16( arsz );

  for ( i = 0; i < arsz; i++ ) {
    sz = 1 + ( qos[ i ].timeliness > 2 ? 2 : 0 ) +
             ( qos[ i ].rate       > 2 ? 2 : 0 );
    uint8_t q = ( qos[ i ].timeliness << 5 ) |
                ( qos[ i ].rate << 1 ) |
                  qos[ i ].dynamic;
    this->u8( sz )
         .u8( q );
    if ( qos[ i ].timeliness > 2 )
      this->u16( qos[ i ].time_info );
    if ( qos[ i ].rate > 2 )
      this->u16( qos[ i ].rate_info );
  }
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_state( const char *fname,  size_t fname_len,
                                    RwfState &state ) noexcept
{
  size_t len,
         sz = 1 + 1 + get_u15_prefix_len( state.text.len ) + state.text.len;
  len = fname_pack_size( fname_len, sz );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_STATE )
       .u15( sz )
       .u8 ( state.data_state | state.stream_state << 3 )
       .u8 ( state.code )
       .u15( state.text.len );
  if ( state.text.len > 0 )
    this->b( state.text.buf, state.text.len );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::pack_uval( const char *fname,  size_t fname_len,
                                 uint64_t uval ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_uint( uval );
  size_t ilen = uint_size( uval ),
         len  = fname_pack_size( fname_len, ilen );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_UINT )
       .u8 ( ilen )
       .u  ( uval, ilen );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::pack_ival( const char *fname,  size_t fname_len,
                                 int64_t ival ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_int( ival );
  size_t ilen = int_size( ival ),
         len  = fname_pack_size( fname_len, ilen );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_INT )
       .u8 ( ilen )
       .i  ( ival, ilen );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::pack_real( const char *fname,  size_t fname_len,
                                 double fval ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_real( fval );
  size_t len = fname_pack_size( fname_len, 8 );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( RWF_DOUBLE )
       .u8 ( 8 )
       .f64( fval );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_decimal( const char *fname,  size_t fname_len,
                                      MDDecimal &dec ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_decimal( dec );
  size_t ilen = 1;
  if ( dec.hint > MD_DEC_NULL || dec.hint < MD_DEC_NNAN )
    ilen += int_size( dec.ival );
  size_t len  = fname_pack_size( fname_len, ilen );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( ilen )
       .u8 ( md_to_rwf_decimal_hint( dec.hint ) );
  if ( ilen > 1 )
    this->i( dec.ival, ilen - 1 );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_time( const char *fname,  size_t fname_len,
                                   MDTime &time ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_time( time );
  size_t sz  = this->time_size( time ),
         len = fname_pack_size( fname_len, sz );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .pack_time( sz, time );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_date( const char *fname,  size_t fname_len,
                                   MDDate &date ) noexcept
{
  if ( this->set != NULL && this->match_set( fname, fname_len ) )
    return this->append_set_date( date );
  size_t len = fname_pack_size( fname_len, 4 );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( 4 )
       .u8 ( date.day )
       .u8 ( date.mon )
       .u16( date.year );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_set_ref( MDReference &mref ) noexcept
{
  uint32_t n = this->nitems;

  if ( this->set == NULL || n >= this->set->count )
    return this->set_error( Err::BAD_FIELD );

  uint8_t type   = this->set->entry[ n ].type;
  int     status = this->pack_mref( type, mref );

  if ( status != 0 )
    this->error( status );
  else {
    this->set_nitems++;
    this->nitems++;
  }
  return *this;
}

size_t
RwfMapWriter::update_hdr( void ) noexcept
{
  size_t  hdr_size = 5;/*flags:u8 key-type:u8 container-type:u8 item-count:u16*/
  uint8_t flags = 0;

  if ( this->key_fid != 0 ) {
    flags |= RwfMapHdr::HAS_KEY_FID;
    hdr_size += 2;
  }
  if ( this->field_defn_size != 0 ) {
    flags |= RwfMapHdr::HAS_SET_DEFS;
    hdr_size += this->field_defn_size;
  }
  if ( this->summary_size != 0 ) {
    flags |= RwfMapHdr::HAS_SUMMARY_DATA;
    hdr_size += this->summary_size;
  }
  if ( this->hint_cnt != 0 ) {
    flags |= RwfMapHdr::HAS_COUNT_HINT;
    hdr_size += 4;
  }
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( flags )
     .u8 ( md_type_to_rwf_primitive_type( this->key_ftype ) )
     .u8 ( this->container_type - RWF_CONTAINER_BASE );
  if ( this->key_fid != 0 )
    hdr.u16( this->key_fid );
  if ( this->field_defn_size != 0 )
    hdr.incr( this->field_defn_size );
  if ( this->summary_size != 0 )
    hdr.incr( this->summary_size );
  if ( this->hint_cnt != 0 )
    hdr.u32( this->hint_cnt | 0xc0000000U );
  hdr.u16( this->nitems );
  return this->off;
}

RwfFieldDefnWriter &
RwfMapWriter::add_field_defn( void ) noexcept
{
  void * m = NULL;
  uint32_t hdr_off = 3;
  if ( this->key_fid != 0 )
    hdr_off += 2;
  this->mem().alloc( sizeof( RwfFieldDefnWriter ), &m );
  this->field_defn = new ( m ) RwfFieldDefnWriter( *this, hdr_off );
  if ( this->summary_size != 0 || this->nitems != 0 )
    this->error( Err::INVALID_MSG );
  return *this->field_defn;
}

bool
RwfMapWriter::check_container( RwfMsgWriterBase &base,
                               bool is_summary ) noexcept
{
  if ( is_summary && this->summary_size != 0 ) {
      this->error( Err::INVALID_MSG );
      base.is_complete = true;
      return false;
  }
  if ( this->container_type != base.type ) {
    if ( this->container_type != RWF_CONTAINER_BASE ) {
      this->error( Err::INVALID_MSG );
      base.is_complete = true;
      return false;
    }
    this->container_type = base.type;
  }
  return true;
}

template<class T>
static T &
add_map_summary( RwfMapWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, true ) ) {
    w.off = 3 + w.field_defn_size;
    if ( w.key_fid != 0 )
      w.off += 2;
    w.append_base( *container, 15, &w.summary_size );
  }
  return *container;
}

template<class T>
static T &
add_map_entry( RwfMapWriter &w,  RwfMapAction action,
               MDReference &key ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, false ) )
    w.add_action_entry( action, key, *container );
  return *container;
}

void
RwfMapWriter::add_action_entry( RwfMapAction action,  MDReference &key,
                                RwfMsgWriterBase &base ) noexcept
{
  if ( this->check_container( base, false ) ) {
    if ( this->nitems++ == 0 ) {
      this->off = 3 + this->summary_size + this->field_defn_size + 2;
      if ( this->key_fid != 0 )
        this->off += 2;
      if ( this->hint_cnt != 0 )
        this->off += 4;
    }
    this->append_key( action, key );
    this->append_base( base, 16, NULL );
  }
}

RwfFieldListWriter   & RwfMapWriter::add_summary_field_list  ( void ) noexcept { return add_map_summary<RwfFieldListWriter>( *this ); }
RwfElementListWriter & RwfMapWriter::add_summary_element_list( void ) noexcept { return add_map_summary<RwfElementListWriter>( *this ); }
RwfMapWriter         & RwfMapWriter::add_summary_map         ( void ) noexcept { return add_map_summary<RwfMapWriter>( *this ); }
RwfFilterListWriter  & RwfMapWriter::add_summary_filter_list ( void ) noexcept { return add_map_summary<RwfFilterListWriter>( *this ); }
RwfSeriesWriter      & RwfMapWriter::add_summary_series      ( void ) noexcept { return add_map_summary<RwfSeriesWriter>( *this ); }
RwfVectorWriter      & RwfMapWriter::add_summary_vector      ( void ) noexcept { return add_map_summary<RwfVectorWriter>( *this ); }

RwfFieldListWriter   & RwfMapWriter::add_field_list  ( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfFieldListWriter>( *this, action, mref ); }
RwfElementListWriter & RwfMapWriter::add_element_list( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfElementListWriter>( *this, action, mref ); }
RwfMapWriter         & RwfMapWriter::add_map         ( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfMapWriter>( *this, action, mref ); }
RwfFilterListWriter  & RwfMapWriter::add_filter_list ( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfFilterListWriter>( *this, action, mref ); }
RwfSeriesWriter      & RwfMapWriter::add_series      ( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfSeriesWriter>( *this, action, mref ); }
RwfVectorWriter      & RwfMapWriter::add_vector      ( RwfMapAction action, MDReference &mref ) noexcept { return add_map_entry<RwfVectorWriter>( *this, action, mref ); }

int
RwfMapWriter::append_key( RwfMapAction action,  MDReference &mref ) noexcept
{
  char      str_buf[ 64 ];
  uint8_t * fptr = mref.fptr;
  size_t    len;
  MDValue   val;
  MDEndian  fendian = mref.fendian;
  MDType    ftype   = this->key_ftype;
  size_t    fsize   = 0;

  switch ( ftype ) {
    case MD_BOOLEAN:
    case MD_ENUM:
    case MD_UINT:
      if ( cvt_number<uint64_t>( mref, val.u64 ) != 0 )
        return this->error( Err::BAD_CVT_NUMBER );
      return this->key_uval( action, val.u64 );

    case MD_INT:
      if ( cvt_number<int64_t>( mref, val.i64 ) != 0 )
        return this->error( Err::BAD_CVT_NUMBER );
      return this->key_ival( action, val.i64 );

    case MD_REAL:
      if ( cvt_number<double>( mref, val.f64 ) != 0 )
        return this->error( Err::BAD_CVT_NUMBER );
      fsize   = 8;
      fptr    = (uint8_t *) (void *) &val;
      fendian = md_endian;
      break;

    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      return this->key_time( action, time );
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      return this->key_date( action, date );
    }
    case MD_DECIMAL: {
      MDDecimal dec;
      dec.get_decimal( mref );
      return this->key_decimal( action, dec );
    }

    case MD_OPAQUE:
    case MD_STRING: {
      if ( mref.ftype == MD_STRING || mref.ftype == MD_PARTIAL ||
           mref.ftype == MD_OPAQUE )
        fsize = mref.fsize;
      else {
        fsize = sizeof( str_buf );
        if ( to_string( mref, str_buf, fsize ) != 0 )
          return this->error( Err::BAD_CVT_STRING );
        fptr = (uint8_t *) (void *) str_buf;
      }
      break;
    }
    default:
      return this->error( Err::BAD_CVT_NUMBER );
  }
  len = map_pack_size( fsize );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8( action )
       .z16( fsize );
  /* invert endian, for little -> big */
  if ( fendian != MD_BIG && is_endian_type( ftype ) )
    this->uN( fptr, fsize );
  else
    this->b( fptr, fsize );
  return 0;
}

int
RwfMapWriter::key_uval( RwfMapAction action,  uint64_t uval ) noexcept
{
  size_t ilen = uint_size( uval ),
         len  = map_pack_size( ilen );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .u8 ( ilen )
       .u  ( uval, ilen );
  return 0;
}

int
RwfMapWriter::key_ival( RwfMapAction action,  int64_t ival ) noexcept
{
  size_t ilen = int_size( ival ),
         len  = map_pack_size( ilen );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .u8 ( ilen )
       .i  ( ival, ilen );
  return 0;
}

int
RwfMapWriter::key_decimal( RwfMapAction action,  MDDecimal &dec ) noexcept
{
  size_t ilen = 1;
  if ( dec.hint > MD_DEC_NULL || dec.hint < MD_DEC_NNAN )
    ilen += int_size( dec.ival );
  size_t len = map_pack_size( ilen );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .u8 ( ilen )
       .u8 ( md_to_rwf_decimal_hint( dec.hint ) );
  if ( ilen > 1 )
    this->i( dec.ival, ilen - 1 );
  return 0;
}

int
RwfMapWriter::key_time( RwfMapAction action,  MDTime &time ) noexcept
{
  size_t sz  = this->time_size( time ),
         len = map_pack_size( sz );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .pack_time( sz, time );
  return 0;
}

int
RwfMapWriter::key_date( RwfMapAction action,  MDDate &date ) noexcept
{
  size_t len = map_pack_size( 4 );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .u8 ( 4 )
       .u8 ( date.day )
       .u8 ( date.mon )
       .u16( date.year );
  return 0;
}

size_t
RwfFilterListWriter::update_hdr( void ) noexcept
{
  size_t  hdr_size = 3 + ( this->hint_cnt != 0 ? 1 : 0 );
  uint8_t flags = 0;

  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  if ( this->hint_cnt != 0 )
    flags |= RwfFilterListHdr::HAS_COUNT_HINT;

  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( flags )
     .u8 ( this->container_type - RWF_CONTAINER_BASE );
  if ( this->hint_cnt != 0 )
    hdr.u8( this->hint_cnt );
  hdr.u8 ( this->nitems );
  return this->off;
}

uint8_t
RwfFilterListWriter::check_container( RwfMsgWriterBase &base ) noexcept
{
  if ( this->container_type != base.type ) {
    if ( this->container_type != RWF_CONTAINER_BASE ) 
      return RwfFilterListHdr::ENTRY_HAS_CONTAINER_TYPE;
    this->container_type = base.type;
  }
  return 0;
}

void
RwfFilterListWriter::add_action_entry( RwfFilterAction action,  uint8_t id,
                                       RwfMsgWriterBase &base ) noexcept
{
  uint8_t flags = this->check_container( base );
  if ( ! this->has_space( 2 + ( flags != 0 ? 1 : 0 ) ) ) {
    base.is_complete = true;
    this->error( Err::NO_SPACE );
    return;
  }
  if ( this->nitems++ == 0 )
    this->off = 3 + ( this->hint_cnt != 0 ? 1 : 0 );
  this->u8( action | ( flags << 4 ) )
       .u8( id );
  if ( flags != 0 )
    this->u8( base.type - RWF_CONTAINER_BASE );
  this->append_base( base, 16, NULL );
}

template<class T>
static T &
add_filter_list_entry( RwfFilterListWriter &w,  RwfFilterAction action,
                       uint8_t id ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  w.add_action_entry( action, id, *container );
  return *container;
}

RwfFieldListWriter   & RwfFilterListWriter::add_field_list  ( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfFieldListWriter>( *this, action, id ); }
RwfElementListWriter & RwfFilterListWriter::add_element_list( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfElementListWriter>( *this, action, id ); }
RwfMapWriter         & RwfFilterListWriter::add_map         ( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfMapWriter>( *this, action, id ); }
RwfFilterListWriter  & RwfFilterListWriter::add_filter_list ( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfFilterListWriter>( *this, action, id ); }
RwfSeriesWriter      & RwfFilterListWriter::add_series      ( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfSeriesWriter>( *this, action, id ); }
RwfVectorWriter      & RwfFilterListWriter::add_vector      ( RwfFilterAction action, uint8_t id ) noexcept { return add_filter_list_entry<RwfVectorWriter>( *this, action, id ); }

size_t
RwfSeriesWriter::update_hdr( void ) noexcept
{
  size_t  hdr_size = 4; /* flags:u8 container-type:u8 item-count:u16 */
  uint8_t flags = 0;

  if ( this->field_defn_size != 0 ) {
    flags |= RwfSeriesHdr::HAS_SET_DEFS;
    hdr_size += this->field_defn_size;
  }
  if ( this->summary_size != 0 ) {
    flags |= RwfSeriesHdr::HAS_SUMMARY_DATA;
    hdr_size += this->summary_size;
  }
  if ( this->hint_cnt != 0 ) {
    flags |= RwfSeriesHdr::HAS_COUNT_HINT;
    hdr_size += 4;
  }
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( flags )
     .u8 ( this->container_type - RWF_CONTAINER_BASE );
  if ( this->field_defn_size != 0 )
    hdr.incr( this->field_defn_size );
  if ( this->summary_size != 0 )
    hdr.incr( this->summary_size );
  if ( this->hint_cnt != 0 )
    hdr.u32( this->hint_cnt | 0xc0000000U );
  hdr.u16( this->nitems );

  return this->off;
}

RwfFieldDefnWriter &
RwfSeriesWriter::add_field_defn( void ) noexcept
{
  void * m = NULL;
  this->mem().alloc( sizeof( RwfFieldDefnWriter ), &m );
  this->field_defn = new ( m ) RwfFieldDefnWriter( *this, 2 );
  if ( this->summary_size != 0 || this->nitems != 0 )
    this->error( Err::INVALID_MSG );
  return *this->field_defn;
}

bool
RwfSeriesWriter::check_container( RwfMsgWriterBase &base,
                                  bool is_summary ) noexcept
{
  if ( is_summary && this->summary_size != 0 ) {
    this->error( Err::INVALID_MSG );
    base.is_complete = true;
    return false;
  }
  if ( this->container_type != base.type ) {
    if ( this->container_type != RWF_CONTAINER_BASE ) {
      this->error( Err::INVALID_MSG );
      base.is_complete = true;
      return false;
    }
    this->container_type = base.type;
  }
  return true;
}

template<class T>
static T &
add_series_summary( RwfSeriesWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, true ) ) {
    w.off = 2 + w.field_defn_size;
    w.append_base( *container, 15, &w.summary_size );
  }
  return *container;
}

template<class T>
static T &
add_series_entry( RwfSeriesWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, false ) ) {
    if ( w.nitems++ == 0 ) {
      w.off = 4 + w.summary_size + w.field_defn_size;
      if ( w.hint_cnt != 0 )
        w.off += 4;
    }
    w.append_base( *container, 16, NULL );
  }
  return *container;
}

RwfFieldListWriter   & RwfSeriesWriter::add_summary_field_list  ( void ) noexcept { return add_series_summary<RwfFieldListWriter>( *this ); }
RwfElementListWriter & RwfSeriesWriter::add_summary_element_list( void ) noexcept { return add_series_summary<RwfElementListWriter>( *this ); }
RwfMapWriter         & RwfSeriesWriter::add_summary_map         ( void ) noexcept { return add_series_summary<RwfMapWriter>( *this ); }
RwfFilterListWriter  & RwfSeriesWriter::add_summary_filter_list ( void ) noexcept { return add_series_summary<RwfFilterListWriter>( *this ); }
RwfSeriesWriter      & RwfSeriesWriter::add_summary_series      ( void ) noexcept { return add_series_summary<RwfSeriesWriter>( *this ); }
RwfVectorWriter      & RwfSeriesWriter::add_summary_vector      ( void ) noexcept { return add_series_summary<RwfVectorWriter>( *this ); }

RwfFieldListWriter   & RwfSeriesWriter::add_field_list  ( void ) noexcept { return add_series_entry<RwfFieldListWriter>( *this ); }
RwfElementListWriter & RwfSeriesWriter::add_element_list( void ) noexcept { return add_series_entry<RwfElementListWriter>( *this ); }
RwfMapWriter         & RwfSeriesWriter::add_map         ( void ) noexcept { return add_series_entry<RwfMapWriter>( *this ); }
RwfFilterListWriter  & RwfSeriesWriter::add_filter_list ( void ) noexcept { return add_series_entry<RwfFilterListWriter>( *this ); }
RwfSeriesWriter      & RwfSeriesWriter::add_series      ( void ) noexcept { return add_series_entry<RwfSeriesWriter>( *this ); }
RwfVectorWriter      & RwfSeriesWriter::add_vector      ( void ) noexcept { return add_series_entry<RwfVectorWriter>( *this ); }

size_t
RwfVectorWriter::update_hdr( void ) noexcept
{
  size_t  hdr_size = 4;
  uint8_t flags = 0;

  if ( this->summary_size != 0 ) {
    flags |= RwfVectorHdr::HAS_SUMMARY_DATA;
    hdr_size += this->summary_size;
  }
  if ( this->hint_cnt != 0 ) {
    flags |= RwfVectorHdr::HAS_COUNT_HINT;
    hdr_size += 4;
  }
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( flags )
     .u8 ( this->container_type - RWF_CONTAINER_BASE );
  if ( this->summary_size != 0 )
    hdr.incr( this->summary_size );
  if ( this->hint_cnt != 0 )
    hdr.u32( this->hint_cnt | 0xc0000000U );
  hdr.u16( this->nitems );

  return this->off;
}

bool
RwfVectorWriter::check_container( RwfMsgWriterBase &base,
                                  bool is_summary ) noexcept
{
  if ( is_summary && this->summary_size != 0 ) {
    this->error( Err::INVALID_MSG );
    base.is_complete = true;
    return false;
  }
  if ( this->container_type != base.type ) {
    if ( this->container_type != RWF_CONTAINER_BASE ) {
      this->error( Err::INVALID_MSG );
      base.is_complete = true;
      return false;
    }
    this->container_type = base.type;
  }
  return true;
}

template<class T>
static T &
add_vector_summary( RwfVectorWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, true ) ) {
    w.off = 2;
    w.append_base( *container, 15, &w.summary_size );
  }
  return *container;
}

template<class T>
static T &
add_vector_entry( RwfVectorWriter &w,  RwfVectorAction action,
                  uint32_t index ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem(), w.dict, NULL, 0 );
  if ( w.check_container( *container, false ) )
    w.add_action_entry( action, index, *container );
  return *container;
}

void
RwfVectorWriter::add_action_entry( RwfVectorAction action,  uint32_t index,
                                   RwfMsgWriterBase &base ) noexcept
{
  if ( this->nitems == 0 ) {
    this->off = 4 + this->summary_size;
    if ( this->hint_cnt != 0 )
      this->off += 4;
  }
  if ( ! this->has_space( 5 ) ) {
    this->error( Err::NO_SPACE );
    base.is_complete = true;
    return;
  }
  this->nitems++;
  this->u8 ( action )
       .u32( index | 0xc0000000U );
  this->append_base( base, 16, NULL );
}

RwfFieldListWriter   & RwfVectorWriter::add_summary_field_list  ( void ) noexcept { return add_vector_summary<RwfFieldListWriter>( *this ); }
RwfElementListWriter & RwfVectorWriter::add_summary_element_list( void ) noexcept { return add_vector_summary<RwfElementListWriter>( *this ); }
RwfMapWriter         & RwfVectorWriter::add_summary_map         ( void ) noexcept { return add_vector_summary<RwfMapWriter>( *this ); }
RwfFilterListWriter  & RwfVectorWriter::add_summary_filter_list ( void ) noexcept { return add_vector_summary<RwfFilterListWriter>( *this ); }
RwfSeriesWriter      & RwfVectorWriter::add_summary_series      ( void ) noexcept { return add_vector_summary<RwfSeriesWriter>( *this ); }
RwfVectorWriter      & RwfVectorWriter::add_summary_vector      ( void ) noexcept { return add_vector_summary<RwfVectorWriter>( *this ); }

RwfFieldListWriter   & RwfVectorWriter::add_field_list  ( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfFieldListWriter>( *this, action, index ); }
RwfElementListWriter & RwfVectorWriter::add_element_list( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfElementListWriter>( *this, action, index ); }
RwfMapWriter         & RwfVectorWriter::add_map         ( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfMapWriter>( *this, action, index ); }
RwfFilterListWriter  & RwfVectorWriter::add_filter_list ( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfFilterListWriter>( *this, action, index ); }
RwfSeriesWriter      & RwfVectorWriter::add_series      ( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfSeriesWriter>( *this, action, index ); }
RwfVectorWriter      & RwfVectorWriter::add_vector      ( RwfVectorAction action, uint32_t index ) noexcept { return add_vector_entry<RwfVectorWriter>( *this, action, index ); }

