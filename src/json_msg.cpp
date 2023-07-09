#include <raimd/json_msg.h>
#include <raimd/json.h>

using namespace rai;
using namespace md;

const char *
JsonMsg::get_proto_string( void ) noexcept
{
  return "JSON";
}

uint32_t
JsonMsg::get_type_id( void ) noexcept
{
  return JSON_TYPE_ID;
}

static MDMatch json_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 1, /* cnt of hint[] */
  .ftype       = (uint8_t) JSON_TYPE_ID,
  .buf         = { '{', 0, 0, 0 },
  .hint        = { JSON_TYPE_ID, 0 },
  .is_msg_type = JsonMsg::is_jsonmsg,
  .unpack      = (md_msg_unpack_f) JsonMsg::unpack
};

bool
JsonMsg::is_jsonmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  if ( end-off >= 2 ) {
    const uint8_t *buf = (const uint8_t *) bb;
    if ( buf[ off ] == '{' ) {
      for ( size_t i = end - 1; i > 0; i-- ) {
        if ( buf[ off + i ] == '}' )
          return true;
        if ( buf[ off + i ] > ' ' )
          return false;
      }
    }
  }
  return false;
}

JsonMsg *
JsonMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                 MDDict *d,  MDMsgMem &m ) noexcept
{
  if ( ! JsonMsg::is_jsonmsg( bb, off, end, h ) )
    return NULL;
  return JsonMsg::unpack_any( bb, off, end, h, d, m );
}

JsonMsg *
JsonMsg::unpack_any( void *bb,  size_t off,  size_t end,  uint32_t,
                     MDDict *d,  MDMsgMem &m ) noexcept
{
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( JsonMsg ), &ptr );

  JsonMsg * msg = new ( ptr ) JsonMsg( bb, off, end, d, m );
  JsonParser parser( m );

  const uint8_t * buf = (const uint8_t *) bb;
  JsonBufInput bin( &((const char *) buf)[ off ], 0, (uint32_t) ( end - off ) );
  if ( parser.parse( bin ) == 0 && parser.value != NULL ) {
    msg->js = parser.value;
    msg->msg_end = msg->msg_off + bin.offset;
    return msg;
  }
  return NULL;
}

void
JsonMsgCtx::release( void ) noexcept
{
#ifdef MD_REF_COUNT
  if ( this->mem != NULL ) {
    if ( this->mem->ref_cnt != MDMsgMem::NO_REF_COUNT )
      if ( --this->mem->ref_cnt == 0 )
          delete this->mem;
  }
#endif
  this->msg    = NULL;
  this->parser = NULL;
  this->input  = NULL;
  this->mem    = NULL;
}

JsonMsgCtx::~JsonMsgCtx() noexcept
{
  this->release();
}

int
JsonMsgCtx::parse( void *bb,  size_t off,  size_t end,  MDDict *d,
                   MDMsgMem &m,  bool is_yaml ) noexcept
{
  void * ptr;
  int    status;

  m.incr_ref();
  this->release();
  this->mem = &m;

  m.alloc( sizeof( JsonMsg ), &ptr );
  this->msg = new ( ptr ) JsonMsg( bb, off, end, d, m );
  m.alloc( sizeof( JsonParser ), &ptr );
  this->parser = new ( ptr ) JsonParser( m );

  const uint8_t * buf = (const uint8_t *) bb;
  m.alloc( sizeof( JsonBufInput ), &ptr );
  this->input = new ( ptr )
    JsonBufInput( &((const char *) buf)[ off ], 0, end - off );
  if ( is_yaml )
    status = this->parser->parse_yaml( *this->input );
  else
    status = this->parser->parse( *this->input );
  if ( status == 0 ) {
    if ( this->parser->value != NULL ) {
      this->msg->js = this->parser->value;
      this->msg->msg_end = this->msg->msg_off + this->input->offset;
      return 0;
    }
    return Err::NOT_FOUND;
  }
  return status;
}

int
JsonMsgCtx::parse_fd( int fd,  MDDict *d,  MDMsgMem &m,  bool is_yaml ) noexcept
{
  void * ptr;
  int    status;

  m.incr_ref();
  this->release();
  this->mem = &m;

  m.alloc( sizeof( JsonMsg ), &ptr );
  this->msg = new ( ptr ) JsonMsg( NULL, 0, 0, d, m );
  m.alloc( sizeof( JsonParser ), &ptr );
  this->parser = new ( ptr ) JsonParser( m );

  m.alloc( sizeof( JsonStreamInput ), &ptr );
  this->stream = new ( ptr ) JsonStreamInput( fd );
  if ( is_yaml )
    status = this->parser->parse_yaml( *this->stream );
  else
    status = this->parser->parse( *this->stream );
  if ( status == 0 ) {
    if ( this->parser->value != NULL ) {
      this->msg->js = this->parser->value;
      return 0;
    }
    return Err::NOT_FOUND;
  }
  return status;
}

void
JsonMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( json_match );
}

int
JsonMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  if ( this->js == NULL || this->js->type != JSON_OBJECT ) {
    iter = NULL;
    return Err::BAD_SUB_MSG;
  }
  this->mem->alloc( sizeof( JsonFieldIter ), &ptr );
  iter = new ( ptr ) JsonFieldIter( *this, *(JsonObject *) this->js );
  return 0;
}

int
JsonMsg::get_reference( MDReference &mref ) noexcept
{
  return JsonMsg::value_to_ref( mref, *this->js );
}

int
JsonMsg::get_sub_msg( MDReference &mref,  MDMsg *&msg,
                      MDFieldIter * ) noexcept
{
  JsonMsg * jmsg;
  void    * ptr;
  if ( mref.ftype != MD_MESSAGE ) {
    msg = NULL;
    return Err::BAD_SUB_MSG;
  }
  this->mem->alloc( sizeof( JsonMsg ), &ptr );
  jmsg = new ( ptr ) JsonMsg( NULL, 0, 0, this->dict, *this->mem );
  jmsg->js = (JsonObject *) (void *) mref.fptr;
  msg = jmsg;
  return 0;
}

int
JsonFieldIter::get_name( MDName &name ) noexcept
{
  JsonObject::Pair &pair = this->obj.val[ this->field_start ];
  if ( pair.name.val[ pair.name.length ] != '\0' ) {
    char * tmp;
    this->me.mem->alloc( pair.name.length + 1, &tmp );
    ::memcpy( tmp, pair.name.val, pair.name.length );
    tmp[ pair.name.length ] = '\0';
    pair.name.val = tmp;
  }
  name.fname    = pair.name.val;
  name.fnamelen = pair.name.length + 1;
  name.fid      = 0;
  return 0;
}

int
JsonFieldIter::copy_name( char *name,  size_t &name_len,  MDFid &fid ) noexcept
{
  JsonObject::Pair &pair = this->obj.val[ this->field_start ];
  size_t off = ( name_len < pair.name.length ? name_len : pair.name.length );
  ::memcpy( name, pair.name.val, off );
  if ( off < name_len )
    name[ off++ ] = '\0';
  name_len = off;
  fid      = 0;
  return 0;
}

int
JsonMsg::value_to_ref( MDReference &mref,  JsonValue &x ) noexcept
{
  mref.fendian  = md_endian;
  mref.fentrysz = 0;
  mref.fentrytp = MD_NODATA;
  switch ( x.type ) {
    default:
    case JSON_NULL:
      mref.fptr  = NULL;
      mref.ftype = MD_NODATA;
      mref.fsize = 0;
      break;

    case JSON_OBJECT: {
      JsonObject *obj = x.to_obj();
      mref.fptr  = (uint8_t *) (void *) obj;
      mref.ftype = MD_MESSAGE;
      mref.fsize = sizeof( JsonObject );
      break;
    }
    case JSON_ARRAY: {
      JsonArray *arr = x.to_arr();
      mref.fptr  = (uint8_t *) (void *) arr;
      mref.ftype = MD_ARRAY;
      mref.fsize = arr->length;
      break;
    }
    case JSON_NUMBER: {
      JsonNumber *num = x.to_num();
      mref.fptr  = (uint8_t *) (void *) &num->val;
      mref.ftype = MD_DECIMAL;
      mref.fsize = sizeof( MDDecimal );
      break;
    }
    case JSON_STRING: {
      JsonString *str = x.to_str();
      mref.fptr  = (uint8_t *) (void *) str->val;
      mref.ftype = MD_STRING;
      mref.fsize = str->length;
      break;
    }
    case JSON_BOOLEAN: {
      JsonBoolean *boo = x.to_bool();
      mref.fptr  = (uint8_t *) (void *) &boo->val;
      mref.ftype = MD_BOOLEAN;
      mref.fsize = sizeof( bool );
      break;
    }
  }
  return 0;
}

int
JsonFieldIter::get_reference( MDReference &mref ) noexcept
{
  JsonObject::Pair &pair = this->obj.val[ this->field_start ];
  return JsonMsg::value_to_ref( mref, *pair.val );
}

int
JsonMsg::get_array_ref( MDReference &mref,  size_t i,
                        MDReference &aref ) noexcept
{
  JsonArray *arr = (JsonArray *) (void *) mref.fptr;
  if ( i >= arr->length )
    return Err::NOT_FOUND;
  JsonValue *val = arr->val[ i ];
  return this->value_to_ref( aref, *val );
}

int
JsonFieldIter::find( const char *name,  size_t name_len,
                     MDReference &mref ) noexcept
{
  if ( name != NULL ) {
    for ( size_t i = 0; i < this->obj.length; i++ ) {
      JsonObject::Pair &pair = this->obj.val[ i ];
      if ( pair.name.length + 1 == name_len &&
           ::memcmp( pair.name.val, name, name_len ) == 0 ) {
        this->field_start = i;
        this->field_end   = i + 1;
        return this->get_reference( mref );
      }
    }
  }
  return Err::NOT_FOUND;
}

int
JsonFieldIter::first( void ) noexcept
{
  this->field_start = 0;
  this->field_end   = 0;
  if ( this->obj.length == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
JsonFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  if ( this->field_start >= this->obj.length )
    return Err::NOT_FOUND;
  this->field_end = this->field_start + 1;
  return 0;
}

int
JsonMsgWriter::append_field_name( const char *fname, size_t fname_len ) noexcept
{
  if ( ( this->flags & FIRST_FIELD ) == 0 ) {
    if ( ! this->start() )
      return Err::NO_SPACE;
    this->flags |= FIRST_FIELD;
  }
  else {
    if ( ! this->has_space( 1 ) )
      return Err::NO_SPACE;
    this->buf[ this->off++ ] = ',';
  }
  if ( ! this->has_space( fname_len + 3 ) )
    return Err::NO_SPACE;

  this->buf[ this->off++ ] = '\"';
  if ( fname_len > 0 )
    this->s( fname, fname_len - 1 );
  this->buf[ this->off++ ] = '\"';
  this->buf[ this->off++ ] = ':';
  return 0;
}

int
JsonMsgWriter::append_field( const char *fname,  size_t fname_len,
                             MDReference &mref ) noexcept
{
  int status = this->append_field_name( fname, fname_len );
  if ( status == 0 )
    status = this->append_ref( mref );
  return status;
}

int
JsonMsgWriter::append_ref( MDReference &mref ) noexcept
{
  char * ptr;
  size_t avail;
  switch ( mref.ftype ) {
    case MD_STRING:
    case MD_OPAQUE:
    case MD_PARTIAL: {
      ptr = (char *) &this->buf[ this->off ];
      if ( ! this->has_space( MDMsg::get_escaped_string_len( mref, "\"" )+1 ) )
        return Err::NO_SPACE;
      this->off += MDMsg::get_escaped_string_output( mref, "\"", ptr );
      break;
    }
    case MD_INT: {
      int64_t ival = get_int<uint64_t>( mref );
      size_t  len  = int_digs( ival );
      ptr = (char *) &this->buf[ this->off ];
      if ( ! this->has_space( len ) )
        return Err::NO_SPACE;
      this->off += int_str( ival, ptr, len );
      break;
    }
    case MD_IPDATA:
      if ( mref.fsize == 4 ) {
        const uint8_t * q = (const uint8_t *) mref.fptr;
        bool b = this->s( "\"", 1 );
        if ( b ) {
          for ( size_t i = 0; i < 4; i++ ) {
            size_t len = uint_digs( q[ i ] );
            b = this->has_space( len + 1 );
            if ( ! b ) break;
            ptr = (char *) &this->buf[ this->off ];
            uint_str( q[ i ], ptr, len );
            if ( i != 3 )
              ptr[ len++ ] = '.';
            this->off += len;
          }
          if ( b )
            b = this->s( "\"", 1 );
        }
        if ( ! b )
          return Err::NO_SPACE;
        break;
      }
    /* FALLTHRU */
    case MD_ENUM:
    case MD_UINT: {
      uint64_t ival = get_uint<uint64_t>( mref );
      size_t   len  = uint_digs( ival );
      ptr = (char *) &this->buf[ this->off ];
      if ( ! this->has_space( len ) )
        return Err::NO_SPACE;
      this->off += uint_str( ival, ptr, len );
      break;
    }
    case MD_DECIMAL:
    case MD_REAL: {
      MDDecimal dec;
      bool b;
      ptr = (char *) &this->buf[ this->off ];
      dec.get_decimal( mref );
      switch ( dec.hint ) {
        case MD_DEC_NULL: b = this->s( "null", 4 ); break;
        case MD_DEC_NNAN: b = this->s( "\"-NaN\"", 6 ); break;
        case MD_DEC_NAN:  b = this->s( "\"NaN\"", 5 ); break;
        case MD_DEC_INF:  b = this->s( "\"Inf\"", 5 ); break;
        default:
          avail = this->buflen - this->off;
          avail = dec.get_string( ptr, avail, true );
          this->off += avail;
          b = ( avail != 0 );
          break;
      }
      if ( ! b )
        return Err::NO_SPACE;
      break;
    }
    case MD_BOOLEAN: {
      bool b = ( *mref.fptr != 0 ) ?
               this->s( "true", 4 ) : this->s( "false", 5 );
      if ( ! b )
        return Err::NO_SPACE;
      break;
    }
    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      bool b = this->s( "\"", 1 );
      if ( b ) {
        ptr   = (char *) &this->buf[ this->off ];
        avail = this->buflen - this->off;
        this->off += time.get_string( ptr, avail );
        b = this->s( "\"", 1 );
      }
      if ( ! b )
        return Err::NO_SPACE;
      break;
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      bool b = this->s( "\"", 1 );
      if ( b ) {
        ptr   = (char *) &this->buf[ this->off ];
        avail = this->buflen - this->off;
        this->off += date.get_string( ptr, avail );
        b = this->s( "\"", 1 );
      }
      if ( ! b )
        return Err::NO_SPACE;
      break;
    }
    default:
      if ( ! this->s( "null", 4 ) )
        return Err::NO_SPACE;
      break;
  }
  return 0;
}

int
JsonMsgWriter::append_msg( const char *fname,  size_t fname_len,
                           JsonMsgWriter &submsg ) noexcept
{
  if ( ! this->has_space( fname_len + 3 ) )
    return Err::NO_SPACE;

  this->buf[ this->off++ ] = '\"';
  if ( fname_len > 0 )
    this->s( fname, fname_len - 1 );
  this->buf[ this->off++ ] = '\"';
  this->buf[ this->off++ ] = ':';

  submsg.buf    = &this->buf[ this->off ];
  submsg.off    = 0;
  submsg.buflen = this->buflen - this->off;
  submsg.flags  = 0;
  return 0;
}

int
JsonMsgWriter::convert_msg( MDMsg &msg ) noexcept
{
  MDFieldIter * iter;
  int status = msg.get_field_iter( iter );
  if ( status == 0 )
    status = iter->first();
  while ( status == 0 ) {
    MDName      name;
    MDReference mref;
    if ( iter->get_name( name ) == 0 && iter->get_reference( mref ) == 0 ) {
      if ( name.fnamelen > 0 ) {
        switch ( mref.ftype ) {
          case MD_MESSAGE: {
            JsonMsgWriter submsg( NULL, 0 );
            MDMsg * msg2 = NULL;
            status = this->append_msg( name.fname, name.fnamelen, submsg );
            if ( status == 0 )
              status = msg.get_sub_msg( mref, msg2, iter );
            if ( status == 0 ) {
              status = submsg.convert_msg( *msg2 );
              if ( status == 0 && ! submsg.finish() )
                status = Err::NO_SPACE;
              else
                this->off += submsg.off;
            }
            if ( status != 0 )
              return status;
            break;
          }
          case MD_ARRAY: {
            MDReference aref;
            size_t      num_entries = mref.fsize;
            if ( mref.fentrysz > 0 )
              num_entries /= mref.fentrysz;
            status = this->append_field_name( name.fname, name.fnamelen );
            if ( status == 0 && ! this->s( "[", 1 ) )
              status = Err::NO_SPACE;
            if ( status != 0 )
              return status;
            for ( size_t i = 0; i < num_entries; i++ ) {
              status = msg.get_array_ref( mref, i, aref );
              if ( status == 0 )
                status = this->append_ref( aref );
              if ( status == 0 && i + 1 < num_entries && ! this->s( ",", 1 ) )
                status = Err::NO_SPACE;
              if ( status != 0 )
                return status;
            }
            if ( ! this->s( "]", 1 ) )
              return Err::NO_SPACE;
            break;
          }
          default:
            status = this->append_field( name.fname, name.fnamelen, mref );
            if ( status != 0 )
              return status;
        }
      }
    }
    status = iter->next();
  }
  if ( status == Err::NOT_FOUND )
    return 0;
  return status;
}
