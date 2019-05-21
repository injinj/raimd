#include <raimd/json_msg.h>
#include <raimd/json.h>

using namespace rai;
using namespace md;

const char *
JsonMsg::get_proto_string( void )
{
  return "JSON";
}

static MDMatch json_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 1, /* cnt of hint[] */
  .ftype       = MD_MESSAGE,
  .buf         = { '{', 0, 0, 0 },
  .hint        = { 0x4a014cc2, 0 },
  .is_msg_type = JsonMsg::is_jsonmsg,
  .unpack      = (md_msg_unpack_f) JsonMsg::unpack
};

bool
JsonMsg::is_jsonmsg( void *bb,  size_t off,  size_t end,  uint32_t h )
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
                 MDDict *d,  MDMsgMem *m )
{
  if ( ! JsonMsg::is_jsonmsg( bb, off, end, h ) )
    return NULL;
  return JsonMsg::unpack_any( bb, off, end, h, d, m );
}

JsonMsg *
JsonMsg::unpack_any( void *bb,  size_t off,  size_t end,  uint32_t h,
                     MDDict *d,  MDMsgMem *m )
{
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;

  void * ptr;
  m->alloc( sizeof( JsonMsg ) + sizeof( JsonParser ), &ptr );

  JsonMsg * msg = new ( ptr ) JsonMsg( bb, off, end, d, m );
  ptr = (void *) &msg[ 1 ];
  JsonParser parser( *m );

  const uint8_t * buf = (const uint8_t *) bb;
  JsonBufInput bin( (const char *) buf, off, end );
  if ( parser.parse( bin ) == 0 && parser.value != NULL ) {
    msg->js = parser.value;
    return msg;
  }
  return NULL;
}

void
JsonMsg::init_auto_unpack( void )
{
  MDMsg::add_match( json_match );
}

int
JsonMsg::get_field_iter( MDFieldIter *&iter )
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
JsonMsg::get_reference( MDReference &mref )
{
  return JsonMsg::value_to_ref( mref, *this->js );
}

int
JsonMsg::get_sub_msg( MDReference &mref,  MDMsg *&msg )
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
JsonFieldIter::get_name( MDName &name )
{
  JsonObject::Pair &pair = this->obj.val[ this->field_start ];
  name.fname    = (char *) pair.name;
  name.fnamelen = ::strlen( name.fname ) + 1;
  name.fid      = 0;
  return 0;
}

int
JsonMsg::value_to_ref( MDReference &mref,  JsonValue &x )
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
      mref.fsize = ::strlen( str->val ) + 1;
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
JsonFieldIter::get_reference( MDReference &mref )
{
  JsonObject::Pair &pair = this->obj.val[ this->field_start ];
  return JsonMsg::value_to_ref( mref, *pair.val );
}

int
JsonMsg::get_array_ref( MDReference &mref,  size_t i,  MDReference &aref )
{
  JsonArray *arr = (JsonArray *) (void *) mref.fptr;
  JsonValue *val = arr->val[ i ];
  return this->value_to_ref( aref, *val );
}

int
JsonFieldIter::find( const char *name )
{
  if ( name != NULL ) {
    for ( size_t i = 0; i < this->obj.length; i++ ) {
      JsonObject::Pair &pair = this->obj.val[ i ];
      if ( ::strcmp( name, pair.name ) == 0 ) {
        this->field_start = i;
        this->field_end   = i + 1;
        return 0;
      }
    }
  }
  return Err::NOT_FOUND;
}

int
JsonFieldIter::first( void )
{
  this->field_start = 0;
  this->field_end   = 0;
  if ( this->obj.length == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
JsonFieldIter::next( void )
{
  this->field_start = this->field_end;
  if ( this->field_start >= this->obj.length )
    return Err::NOT_FOUND;
  this->field_end = this->field_start + 1;
  return 0;
}

