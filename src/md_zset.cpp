#include <stdio.h>
#include <raimd/md_zset.h>

using namespace rai;
using namespace md;

const char *
ZSetMsg::get_proto_string( void )
{
  return "MD_ZSET";
}

static MDMatch zsetmsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_ZSET,
  .buf         = { ZSetData::zst8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = ZSetMsg::is_zsetmsg,
  .unpack      = ZSetMsg::unpack
};

static inline bool is_mask( uint32_t m ) { return ( m & ( m + 1 ) ) == 0; }

bool
ZSetMsg::is_zsetmsg( void *bb,  size_t off,  size_t end,  uint32_t h )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  if ( len >= 4 && get_u16<MD_LITTLE>( buf ) == ZSetData::zst8_sig &&
       is_mask( buf[ 2 ] ) && is_mask( buf[ 3 ] ) )
    return true;
  if ( len >= 8 && get_u32<MD_LITTLE>( buf ) == ZSetData::zst16_sig &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 4 ] ) ) &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 6 ] ) ) )
    return true;
  if ( len >= 16 && get_u64<MD_LITTLE>( buf ) == ZSetData::zst32_sig &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 8 ] ) ) &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 12 ] ) ) )
    return true;
  return false;
}

MDMsg *
ZSetMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                 MDMsgMem *m )
{
  if ( ! is_zsetmsg( bb, off, end, h ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the ZSetMsg */
  void * ptr;
  m->alloc( sizeof( ZSetMsg ), &ptr );
  return new ( ptr ) ZSetMsg( bb, off, end, d, m );
}

void
ZSetMsg::init_auto_unpack( void )
{
  MDMsg::add_match( zsetmsg_match );
}

int
ZSetMsg::get_field_iter( MDFieldIter *&iter )
{
  void * ptr;
  this->mem->alloc( sizeof( ZSetFieldIter ), &ptr );
  iter = new ( ptr ) ZSetFieldIter( *this );
  return 0;
}

int
ZSetFieldIter::get_name( MDName &name )
{
  if ( this->keylen == 0 ) {
    if ( this->zset.zindex( this->field_start+1, this->val ) != ZSET_OK )
      return Err::NOT_FOUND;
    this->keylen = dec64_to_string( (Dec64Store *) &this->val.score,
                                    this->key );
  }
  name.fname    = this->key;
  name.fnamelen = this->keylen;
  name.fid = 0;
  return 0;
}

int
ZSetFieldIter::get_reference( MDReference &mref )
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
ZSetFieldIter::first( void )
{
  this->field_start = 0;
  this->field_end   = 0;
  this->keylen      = 0;
  if ( this->zset.hcount() == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
ZSetFieldIter::next( void )
{
  this->field_start = this->field_end;
  this->keylen      = 0;
  if ( this->field_start >= this->zset.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

