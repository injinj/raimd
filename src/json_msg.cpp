#include <stdio.h>
#include <raimd/json_msg.h>
#include <raimd/json.h>
#include <raimd/md_dict.h>

using namespace rai;
using namespace md;

static const char JsonMsg_proto_string[] = "JSON";
const char *
JsonMsg::get_proto_string( void ) noexcept
{
  return JsonMsg_proto_string;
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
  .unpack      = (md_msg_unpack_f) JsonMsg::unpack,
  .name        = JsonMsg_proto_string
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
      if ( MDDict::dict_equals( name, name_len, pair.name.val,
                                pair.name.length ) ) {
        this->field_start = i;
        this->field_end   = i + 1;
        this->field_index = i;
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
  this->field_index = 0;
  if ( this->obj.length == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
JsonFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  this->field_index++;
  if ( this->field_start >= this->obj.length )
    return Err::NOT_FOUND;
  this->field_end = this->field_start + 1;
  return 0;
}

bool
JsonMsgWriter::resize( size_t len ) noexcept
{
  static const size_t max_size = 0x3fffffff; /* 1 << 30 - 1 == 1073741823 */
  if ( this->err != 0 )
    return false;
  JsonMsgWriter *p = this;
  for ( ; p->parent != NULL; p = p->parent )
    ;
  if ( len > max_size )
    return false;
  size_t old_len = p->buflen,
         new_len = old_len + ( len - this->off );
  if ( new_len > max_size )
    return false;
  if ( new_len < old_len * 2 )
    new_len = old_len * 2;
  else
    new_len += 1024;
  if ( new_len > max_size )
    new_len = max_size;
  uint8_t * old_buf = p->buf,
          * new_buf = old_buf;
  this->mem.extend( old_len, new_len, &new_buf );
  uint8_t * end = &new_buf[ new_len ];

  p->buf    = new_buf;
  p->buflen = new_len;

  for ( JsonMsgWriter *c = this; c != p; c = c->parent ) {
    if ( c->buf >= old_buf && c->buf < &old_buf[ old_len ] ) {
      size_t off = c->buf - old_buf;
      c->buf = &new_buf[ off ];
      c->buflen = end - c->buf;
    }
  }
  return this->off + len <= this->buflen;
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

JsonMsgWriter &
JsonMsgWriter::append_field( const char *fname,  size_t fname_len,
                             MDReference &mref ) noexcept
{
  int status = this->append_field_name( fname, fname_len );
  if ( status == 0 )
    return this->append_ref( mref );
  return this->error( status );
}

JsonMsgWriter &
JsonMsgWriter::append_ref( MDReference &mref ) noexcept
{
  char * ptr;
  size_t avail;
  switch ( mref.ftype ) {
    case MD_STRING:
    case MD_OPAQUE:
    case MD_PARTIAL: {
      if ( ! this->has_space( MDMsg::get_escaped_string_len( mref, "\"" )+1 ) )
        return this->error( Err::NO_SPACE );
      ptr = (char *) &this->buf[ this->off ];
      this->off += MDMsg::get_escaped_string_output( mref, "\"", ptr );
      break;
    }
    case MD_INT: {
      int64_t ival = get_int<uint64_t>( mref );
      size_t  len  = int_digs( ival );
      if ( ! this->has_space( len ) )
        return this->error( Err::NO_SPACE );
      ptr = (char *) &this->buf[ this->off ];
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
          return this->error( Err::NO_SPACE );
        break;
      }
    /* FALLTHRU */
    case MD_ENUM:
    case MD_UINT: {
      uint64_t ival = get_uint<uint64_t>( mref );
      size_t   len  = uint_digs( ival );
      if ( ! this->has_space( len ) )
        return this->error( Err::NO_SPACE );
      ptr = (char *) &this->buf[ this->off ];
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
        return this->error( Err::NO_SPACE );
      break;
    }
    case MD_BOOLEAN: {
      bool b = ( *mref.fptr != 0 ) ?
               this->s( "true", 4 ) : this->s( "false", 5 );
      if ( ! b )
        return this->error( Err::NO_SPACE );
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
        return this->error( Err::NO_SPACE );
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
        return this->error( Err::NO_SPACE );
      break;
    }
    default:
      if ( ! this->s( "null", 4 ) )
        return this->error( Err::NO_SPACE );
      break;
  }
  return *this;
}

JsonMsgWriter &
JsonMsgWriter::append_msg( const char *fname,  size_t fname_len,
                           JsonMsgWriter &submsg ) noexcept
{
  if ( ! this->has_space( fname_len + 3 ) )
    return this->error( Err::NO_SPACE );

  this->buf[ this->off++ ] = '\"';
  if ( fname_len > 0 )
    this->s( fname, fname_len - 1 );
  this->buf[ this->off++ ] = '\"';
  this->buf[ this->off++ ] = ':';

  submsg.buf    = &this->buf[ this->off ];
  submsg.off    = 0;
  submsg.buflen = this->buflen - this->off;
  submsg.flags  = 0;
  submsg.err    = 0;
  submsg.parent = this;
  return submsg;
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
            JsonMsgWriter submsg( this->mem, NULL, 0 );
            MDMsg * msg2 = NULL;
            this->append_msg( name.fname, name.fnamelen, submsg );
            status = this->err;
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
              if ( status == 0 ) {
                this->append_ref( aref );
                status = this->err;
              }
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
            this->append_field( name.fname, name.fnamelen, mref );
            status = this->err;
            if ( status != 0 )
              return status;
            break;
        }
      }
    }
    status = iter->next();
  }
  if ( status != Err::NOT_FOUND )
    return status;
  return 0;
}
