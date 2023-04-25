#include <stdio.h>
#include <raimd/md_hash.h>

using namespace rai;
using namespace md;

const char *
HashMsg::get_proto_string( void ) noexcept
{
  return "MD_HASH";
}

uint32_t
HashMsg::get_type_id( void ) noexcept
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

static bool
is_hash( void *bb,  size_t off,  size_t &end )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off,
            msz = ListData::mem_size( buf, len, HashData::hsh8_sig,
                                     HashData::hsh16_sig, HashData::hsh32_sig );
  end = off + msz;
  return msz != 0 && msz <= len;
}

bool
HashMsg::is_hashmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  return is_hash( bb, off, end );
}

MDMsg *
HashMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                 MDMsgMem *m ) noexcept
{
  if ( ! is_hash( bb, off, end ) )
    return NULL;
#ifdef MD_REF_COUNT
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
#endif
  /* check if another message is the first opaque field of the HashMsg */
  void * ptr;
  m->alloc( sizeof( HashMsg ), &ptr );
  return new ( ptr ) HashMsg( bb, off, end, d, m );
}

void
HashMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( hashmsg_match );
}

int
HashMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( HashFieldIter ), &ptr );
  iter = new ( ptr ) HashFieldIter( *this );
  return 0;
}

int
HashFieldIter::get_name( MDName &name ) noexcept
{
  if ( this->val.keylen == 0 ) {
    if ( this->hash.hindex( this->field_start+1, this->val ) != HASH_OK )
      return Err::NOT_FOUND;
    this->val.key[ this->val.keylen ] = '\0';
  }
  name.fname    = this->val.key;
  name.fnamelen = this->val.keylen + 1;
  name.fid = 0;
  return 0;
}

int
HashFieldIter::get_reference( MDReference &mref ) noexcept
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
HashFieldIter::first( void ) noexcept
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
HashFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  this->val.keylen  = 0;
  if ( this->field_start >= this->hash.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

