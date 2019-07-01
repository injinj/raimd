#include <stdio.h>
#include <raimd/md_hash.h>

using namespace rai;
using namespace md;

const char *
HashMsg::get_proto_string( void )
{
  return "MD_HASH";
}

uint32_t
HashMsg::get_type_id( void )
{
  return MD_HASH;
}

static MDMatch hashmsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_HASH,
  .buf         = { HashData::hsh8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = HashMsg::is_hashmsg,
  .unpack      = HashMsg::unpack
};

static inline bool is_mask( uint32_t m ) { return ( m & ( m + 1 ) ) == 0; }

bool
HashMsg::is_hashmsg( void *bb,  size_t off,  size_t end,  uint32_t )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  if ( len >= 4 && get_u16<MD_LITTLE>( buf ) == HashData::hsh8_sig &&
       is_mask( buf[ 2 ] ) && is_mask( buf[ 3 ] ) )
    return true;
  if ( len >= 8 && get_u32<MD_LITTLE>( buf ) == HashData::hsh16_sig &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 4 ] ) ) &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 6 ] ) ) )
    return true;
  if ( len >= 16 && get_u64<MD_LITTLE>( buf ) == HashData::hsh32_sig &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 8 ] ) ) &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 12 ] ) ) )
    return true;
  return false;
}

MDMsg *
HashMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                 MDMsgMem *m )
{
  if ( ! is_hashmsg( bb, off, end, h ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the HashMsg */
  void * ptr;
  m->alloc( sizeof( HashMsg ), &ptr );
  return new ( ptr ) HashMsg( bb, off, end, d, m );
}

void
HashMsg::init_auto_unpack( void )
{
  MDMsg::add_match( hashmsg_match );
}

int
HashMsg::get_field_iter( MDFieldIter *&iter )
{
  void * ptr;
  this->mem->alloc( sizeof( HashFieldIter ), &ptr );
  iter = new ( ptr ) HashFieldIter( *this );
  return 0;
}

int
HashFieldIter::get_name( MDName &name )
{
  if ( this->val.keylen == 0 ) {
    if ( this->hash.hindex( this->field_start+1, this->val ) != HASH_OK )
      return Err::NOT_FOUND;
    this->val.key[ this->val.keylen ] = '\0';
  }
  name.fname    = this->val.key;
  name.fnamelen = this->val.keylen;
  name.fid = 0;
  return 0;
}

int
HashFieldIter::get_reference( MDReference &mref )
{
  if ( this->val.keylen == 0 ) {
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
HashFieldIter::first( void )
{
  this->field_start = 0;
  this->field_end   = 0;
  this->val.keylen  = 0;
  if ( this->hash.hcount() == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
HashFieldIter::next( void )
{
  this->field_start = this->field_end;
  this->val.keylen  = 0;
  if ( this->field_start >= this->hash.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

