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
                 MDDict *d,  MDMsgMem *m ) noexcept
{
  if ( ! JsonMsg::is_jsonmsg( bb, off, end, h ) )
    return NULL;
  return JsonMsg::unpack_any( bb, off, end, h, d, m );
}

JsonMsg *
JsonMsg::unpack_any( void *bb,  size_t off,  size_t end,  uint32_t,
                     MDDict *d,  MDMsgMem *m ) noexcept
{
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;

  void * ptr;
  m->alloc( sizeof( JsonMsg ), &ptr );

  JsonMsg * msg = new ( ptr ) JsonMsg( bb, off, end, d, m );
  JsonParser parser( *m );

  const uint8_t * buf = (const uint8_t *) bb;
  JsonBufInput bin( &((const char *) buf)[ off ], 0, end - off );
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
  if ( this->mem != NULL ) {
    if ( this->mem->ref_cnt != MDMsgMem::NO_REF_COUNT )
      if ( --this->mem->ref_cnt == 0 )
          delete this->mem;
  }
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
                   MDMsgMem *m,  bool is_yaml ) noexcept
{
  void * ptr;
  int    status;

  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  this->release();
  this->mem = m;

  m->alloc( sizeof( JsonMsg ), &ptr );
  this->msg = new ( ptr ) JsonMsg( bb, off, end, d, m );
  m->alloc( sizeof( JsonParser ), &ptr );
  this->parser = new ( ptr ) JsonParser( *m );

  const uint8_t * buf = (const uint8_t *) bb;
  m->alloc( sizeof( JsonBufInput ), &ptr );
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
JsonMsg::get_sub_msg( MDReference &mref,  MDMsg *&msg ) noexcept
{
  JsonMsg * jmsg;
  void    * ptr;
  if ( mref.ftype != MD_MESSAGE ) {
    msg = NULL;
    return Err::BAD_SUB_MSG;
  }
  this->mem->alloc( sizeof( JsonMsg ), &ptr );
  jmsg = new ( ptr ) JsonMsg( NULL, 0, 0, this->dict, this->mem );
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

