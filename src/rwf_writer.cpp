#include <stdio.h>
#include <math.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

RwfMsgWriterBase::RwfMsgWriterBase( RwfWriterType t,  MDMsgMem &m,  MDDict *d,
                                    void *bb, size_t len ) noexcept
                : mem( m ), dict( 0 ), buf( (uint8_t *) bb ), off( 0 ),
                  buflen( len ), prefix_len( 0 ), len_bits( 0 ), err( 0 ),
                  size_ptr( 0 ), parent( 0 ), child( 0 ), type( t ),
                  is_complete( false )
{
  while ( d != NULL && d->dict_type[ 0 ] != 'a' ) /* look for app_a type */
    d = d->next;
  this->dict = d;
}

int
RwfMsgWriterBase::append_base( RwfMsgWriterBase &base,  int bits,
                               size_t *sz_ptr ) noexcept
{
  size_t len_bytes = ( bits == 0 ? 0 : bits == 15 ? 2 : 3 );
  if ( ! this->has_space( this->off + base.off + len_bytes ) )
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
    *base.size_ptr = len + len_bytes;
}

bool
RwfMsgWriterBase::resize( size_t len ) noexcept
{
  static const size_t max_size = 0x3fffffff; /* 1 << 30 - 1 == 1073741823 */
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
  this->mem.extend( old_len, new_len, &new_buf );
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
  if ( this->parent != NULL )
    return this->parent->error( e );
  return this->err = e;
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
  this->mem.alloc( sizeof( RwfMsgWriter ), &this->child );
  return this->child;
}

void
RwfMsgWriter::reset( void ) noexcept
{
  this->RwfMsgWriterBase::reset( 0, 64 );

  uint8_t *start = (uint8_t *) (void *) &this->msg_flags,
          *end   = (uint8_t *) (void *) &this[ 1 ];
  ::memset( start, 0, end - start );

  this->flags          = rwf_msg_always_present[ this->msg_class ];
  this->msg_key_off    = 0;
  this->msg_key_size   = 0;
  this->container_off  = 0;
  this->container_size = 0;
  this->header_size    = 0;
  this->container_type = RWF_CONTAINER_BASE;
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
    hdr.set( this->header_size - af );
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
      RwfMsgKeyWriter( this->mem, this->dict, NULL, 0 );
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
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
RwfMsgKeyWriter::service_id( uint16_t service_id ) noexcept
{
  if ( ! this->has_space( 2 ) )
    return this->set_error( Err::NO_SPACE );
  this->key_flags |= RwfMsgKey::HAS_SERVICE_ID;
  this->z16( service_id );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::name( const char *nm,  size_t nm_size ) noexcept
{
  if ( ! this->has_space( nm_size + 1 ) )
    return this->set_error( Err::NO_SPACE );
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
  this->key_flags |= RwfMsgKey::HAS_NAME_TYPE;
  this->u8( name_type );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::filter( uint32_t filter ) noexcept
{
  if ( ! this->has_space( 4 ) )
    return this->set_error( Err::NO_SPACE );
  this->key_flags |= RwfMsgKey::HAS_FILTER;
  this->u32( filter );
  return *this;
}

RwfMsgKeyWriter &
RwfMsgKeyWriter::identifier( uint32_t id ) noexcept
{
  if ( ! this->has_space( 4 ) )
    return this->set_error( Err::NO_SPACE );
  this->key_flags |= RwfMsgKey::HAS_IDENTIFIER;
  this->u32( id );
  return *this;
}

RwfElementListWriter &
RwfMsgKeyWriter::attrib( void ) noexcept
{
  RwfElementListWriter * elem_list =
    new ( this->make_child() )
      RwfElementListWriter( this->mem, this->dict, NULL, 0 );
    
  if ( this->has_space( 1 ) ) {
    this->key_flags |= RwfMsgKey::HAS_ATTRIB;
    this->u8( RWF_ELEMENT_LIST - RWF_CONTAINER_BASE );
    this->append_base( *elem_list, 15, NULL );
  }
  return *elem_list;
}

size_t
RwfFieldListWriter::update_hdr( void ) noexcept
{
  size_t hdr_size = 7;
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( RwfFieldListHdr::HAS_FIELD_LIST_INFO |
           RwfFieldListHdr::HAS_STANDARD_DATA )
     .u8 ( 3 )  /* info size */
     .u8 ( 1 )  /* dict id */
     .u16( this->flist )
     .u16( this->nflds );
  return this->off;
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( const char *fname,  size_t fname_len,
                                 const void *ival,  size_t ilen,
                                 MDType t ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_ival( by, ival, ilen, t );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( MDFid fid,  const void *ival, size_t ilen,
                                 MDType t ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_ival( by, ival, ilen, t );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ival( MDLookup &by,  const void *ival,  size_t ilen,
                                 MDType t ) noexcept
{
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
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_ref( fid, by.ftype, by.fsize, mref );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ref( const char *fname,  size_t fname_len,
                                MDReference &mref ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_ref( by.fid, by.ftype, by.fsize, mref );
}

RwfFieldListWriter &
RwfFieldListWriter::append_ref( MDFid fid,  MDType ftype,  uint32_t fsize,
                                MDReference &mref ) noexcept
{
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
      if ( slen < fsize )
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
  if ( fendian != MD_BIG && fsize > 1 &&
       ( ftype == MD_UINT || ftype == MD_INT || ftype == MD_REAL ) )
    this->flip( fptr, fsize );
  else
    this->b( fptr, fsize );
  return *this;
}

RwfFieldListWriter &
RwfFieldListWriter::append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                                    MDDecimal &dec ) noexcept
{
  if ( ftype == MD_DECIMAL ) {
    size_t ilen = int_size( dec.ival );

    if ( ! this->has_space( ilen + 4 ) )
      return this->set_error( Err::NO_SPACE );

    this->nflds++;
    this->u16( fid )
         .u8 ( ilen + 1 )
         .u8 ( md_to_rwf_decimal_hint( dec.hint ) )
         .i  ( dec.ival, ilen );
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

size_t
RwfMsgWriterBase::time_size( const MDTime &time ) noexcept
{
  switch ( time.resolution ) {
    case MD_RES_MILLISECS: return 5;
    case MD_RES_MICROSECS: return 7;
    case MD_RES_NANOSECS:  return 8;
    case MD_RES_MINUTES:   return 3;
    case MD_RES_SECONDS:   return 4;
    default:               return 1;
  }
}

void
RwfMsgWriterBase::pack_time( size_t sz,  const MDTime &time ) noexcept
{
  if ( sz == 1 ) {
    this->u8( 0 );
    return;
  }
  this->u8( sz - 1 )
       .u8( time.hour )
       .u8( time.minute );
  if ( sz > 3 ) /* second */
    this->u8( time.sec );
  if ( sz == 5 ) /* millisec */
    this->u16( time.fraction );
  else if ( sz == 7 ) { /* microsec */
    this->u16( time.fraction / 1000 )
         .u16( time.fraction % 1000 );
  }
  else if ( sz == 8 ) { /* nano encoding */
    uint16_t milli = (uint16_t) ( time.fraction / 1000000 ),
             nano  = (uint16_t) ( time.fraction % 1000 ),
             micro = (uint16_t) ( time.fraction % 1000000 ) / 1000;
    micro |= ( nano & 0xff00 ) << 3; /* shift 3 top bits into micro */
    nano  &= 0xff;
    this->u16( milli )
         .u16( micro )
         .u8 ( nano );
  }
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                                 MDTime &time ) noexcept
{
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
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_decimal( by.fid, by.ftype, by.fsize, dec );
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( MDFid fid,  MDTime &time ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_time( by.fid, by.ftype, by.fsize, time );
}

RwfFieldListWriter &
RwfFieldListWriter::append_date( MDFid fid,  MDDate &date ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_date( by.fid, by.ftype, by.fsize, date );
}

RwfFieldListWriter &
RwfFieldListWriter::append_decimal( const char *fname,  size_t fname_len,
                                    MDDecimal &dec ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_decimal( by.fid, by.ftype, by.fsize, dec );
}

RwfFieldListWriter &
RwfFieldListWriter::append_time( const char *fname,  size_t fname_len,
                                 MDTime &time ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_time( by.fid, by.ftype, by.fsize, time );
}

RwfFieldListWriter &
RwfFieldListWriter::append_date( const char *fname,  size_t fname_len,
                                 MDDate &date ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->set_error( Err::UNKNOWN_FID );
  return this->append_date( by.fid, by.ftype, by.fsize, date );
}

static RWF_type
md_type_to_rwf_primitive_type( MDType type ) noexcept
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

    default:
      return RWF_BUFFER;
  }
}

size_t
RwfElementListWriter::update_hdr( void ) noexcept
{
  size_t hdr_size = 3;
  if ( hdr_size > this->off )
    this->off = hdr_size;
  if ( ! this->check_offset() ) {
    this->error( Err::NO_SPACE );
    return 0;
  }
  RwfMsgWriterHdr hdr( *this );
  hdr.u8 ( RwfElementListHdr::HAS_STANDARD_DATA )
     .u16( this->nitems );
  return this->off;
}

RwfElementListWriter &
RwfElementListWriter::append_ref( const char *fname,  size_t fname_len,
                                  MDReference &mref ) noexcept
{
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

  if ( fendian != MD_BIG && fsize > 1 &&
       ( ftype == MD_UINT || ftype == MD_INT || ftype == MD_REAL ) )
    this->flip( fptr, fsize );
  else
    this->b( fptr, fsize );

  return *this;
}

RwfElementListWriter &
RwfElementListWriter::pack_uval( const char *fname,  size_t fname_len,
                                 uint64_t uval ) noexcept
{
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
  size_t ilen = int_size( dec.ival ),
         len  = fname_pack_size( fname_len, ilen );
  if ( ! this->has_space( len ) )
    return this->set_error( Err::NO_SPACE );

  this->nitems++;
  this->u15( fname_len )
       .b  ( fname, fname_len )
       .u8 ( ilen + 1 )
       .u8 ( md_to_rwf_decimal_hint( dec.hint ) )
       .i  ( dec.ival, ilen );
  return *this;
}

RwfElementListWriter &
RwfElementListWriter::append_time( const char *fname,  size_t fname_len,
                                   MDTime &time ) noexcept
{
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

size_t
RwfMapWriter::update_hdr( void ) noexcept
{
  size_t  hdr_size = 5;
  uint8_t flags = 0;

  if ( this->summary_size != 0 ) {
    flags |= RwfMapHdr::HAS_SUMMARY_DATA;
    hdr_size += this->summary_size;
  }
  if ( this->hint_cnt != 0 ) {
    flags |= RwfMapHdr::HAS_COUNT_HINT;
    hdr_size += 4;
  }
  if ( this->key_fid != 0 ) {
    flags |= RwfMapHdr::HAS_KEY_FID;
    hdr_size += 2;
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
  if ( this->summary_size != 0 )
    hdr.incr( this->summary_size );
  if ( this->hint_cnt != 0 )
    hdr.u32( this->hint_cnt | 0xc0000000U );
  hdr.u16( this->nitems );
  return this->off;
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
  if ( w.check_container( *container, true ) ) {
    w.off = 3;
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
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
      this->off = 3 + this->summary_size + 2;
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
  if ( fendian != MD_BIG && fsize > 1 &&
       ( ftype == MD_UINT || ftype == MD_INT || ftype == MD_REAL ) )
    this->flip( fptr, fsize );
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
  size_t ilen = int_size( dec.ival ),
         len  = map_pack_size( ilen );
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );

  this->u8 ( action )
       .u8 ( ilen + 1 )
       .u8 ( md_to_rwf_decimal_hint( dec.hint ) )
       .i  ( dec.ival, ilen );
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
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
  size_t  hdr_size = 4;
  uint8_t flags = 0;

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
  if ( this->summary_size != 0 )
    hdr.incr( this->summary_size );
  if ( this->hint_cnt != 0 )
    hdr.u32( this->hint_cnt | 0xc0000000U );
  hdr.u16( this->nitems );

  return this->off;
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
  if ( w.check_container( *container, true ) ) {
    w.off = 2;
    w.append_base( *container, 15, &w.summary_size );
  }
  return *container;
}

template<class T>
static T &
add_series_entry( RwfSeriesWriter &w ) noexcept
{
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
  if ( w.check_container( *container, false ) ) {
    if ( w.nitems++ == 0 ) {
      w.off = 4 + w.summary_size;
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
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
  T * container = new ( w.make_child() ) T( w.mem, w.dict, NULL, 0 );
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

