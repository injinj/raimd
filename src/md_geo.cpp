#include <stdio.h>
#include <raimd/md_geo.h>

using namespace rai;
using namespace md;

const char *
GeoMsg::get_proto_string( void ) noexcept
{
  return "MD_GEO";
}

uint32_t
GeoMsg::get_type_id( void ) noexcept
{
  return MD_GEO;
}

static MDMatch geomsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_GEO,
  .buf         = { GeoData::geo8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = GeoMsg::is_geomsg,
  .unpack      = GeoMsg::unpack
};

static bool
is_geo( void *bb,  size_t off,  size_t &end )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off,
            msz = ListData::mem_size( buf, len, GeoData::geo8_sig,
                                      GeoData::geo16_sig, GeoData::geo32_sig );
  end = off + msz;
  return msz != 0 && msz <= len;
}

bool
GeoMsg::is_geomsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  return is_geo( bb, off, end );
}

MDMsg *
GeoMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                 MDMsgMem *m ) noexcept
{
  if ( ! is_geo( bb, off, end ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the GeoMsg */
  void * ptr;
  m->alloc( sizeof( GeoMsg ), &ptr );
  return new ( ptr ) GeoMsg( bb, off, end, d, m );
}

void
GeoMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( geomsg_match );
}

int
GeoMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( GeoFieldIter ), &ptr );
  iter = new ( ptr ) GeoFieldIter( *this );
  return 0;
}

static size_t
hex_str( GeoIndx idx,  char *s )
{
  static const char h[] = "0123456789abcdef";
  int i = 64;
  s[ 0 ] = '0'; s[ 1 ] = 'x';
  for ( int j = 2; j < 2 + 16; j++ ) {
    i -= 4;
    s[ j ] = h[ ( idx >> i ) & 0xf ];
  }
  s[ 2 + 16 ] = '\0';
  return 2 + 16;
}

int
GeoFieldIter::get_name( MDName &name ) noexcept
{
  if ( this->keylen == 0 ) {
    if ( this->geo.geoindex( this->field_start+1, this->val ) != GEO_OK )
      return Err::NOT_FOUND;
    this->keylen = hex_str( this->val.score, this->key );
  }
  name.fname    = this->key;
  name.fnamelen = this->keylen;
  name.fid = 0;
  return 0;
}

int
GeoFieldIter::get_reference( MDReference &mref ) noexcept
{
  if ( this->keylen == 0 ) {
    MDName n;
    this->get_name( n );
  }
  mref.zero();
  mref.ftype = MD_STRING;
  mref.fptr  = (uint8_t *) this->val.data;
  mref.fsize = this->val.sz;
  if ( this->val.sz2 > 0 ) {
    this->iter_msg.mem->alloc( this->val.sz + this->val.sz2, &mref.fptr );
    mref.fsize += this->val.sz2;
    ::memcpy( mref.fptr, this->val.data, this->val.sz );
    ::memcpy( &mref.fptr[ this->val.sz ], this->val.data2, this->val.sz2 );
  }
  return 0;
}

int
GeoFieldIter::first( void ) noexcept
{
  this->field_start = 0;
  this->field_end   = 0;
  this->keylen      = 0;
  if ( this->geo.hcount() == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
GeoFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  this->keylen      = 0;
  if ( this->field_start >= this->geo.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

