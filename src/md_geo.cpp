#include <stdio.h>
#include <raimd/md_geo.h>

using namespace rai;
using namespace md;

const char *
GeoMsg::get_proto_string( void )
{
  return "MD_GEO";
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

static inline bool is_mask( uint32_t m ) { return ( m & ( m + 1 ) ) == 0; }

bool
GeoMsg::is_geomsg( void *bb,  size_t off,  size_t end,  uint32_t )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  if ( len >= 4 && get_u16<MD_LITTLE>( buf ) == GeoData::geo8_sig &&
       is_mask( buf[ 2 ] ) && is_mask( buf[ 3 ] ) )
    return true;
  if ( len >= 8 && get_u32<MD_LITTLE>( buf ) == GeoData::geo16_sig &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 4 ] ) ) &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 6 ] ) ) )
    return true;
  if ( len >= 16 && get_u64<MD_LITTLE>( buf ) == GeoData::geo32_sig &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 8 ] ) ) &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 12 ] ) ) )
    return true;
  return false;
}

MDMsg *
GeoMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                 MDMsgMem *m )
{
  if ( ! is_geomsg( bb, off, end, h ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the GeoMsg */
  void * ptr;
  m->alloc( sizeof( GeoMsg ), &ptr );
  return new ( ptr ) GeoMsg( bb, off, end, d, m );
}

void
GeoMsg::init_auto_unpack( void )
{
  MDMsg::add_match( geomsg_match );
}

int
GeoMsg::get_field_iter( MDFieldIter *&iter )
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
GeoFieldIter::get_name( MDName &name )
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
GeoFieldIter::get_reference( MDReference &mref )
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
GeoFieldIter::first( void )
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
GeoFieldIter::next( void )
{
  this->field_start = this->field_end;
  this->keylen      = 0;
  if ( this->field_start >= this->geo.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

