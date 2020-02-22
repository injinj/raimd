#include <stdio.h>
#include <raimd/md_zset.h>

using namespace rai;
using namespace md;

const char *
ZSetMsg::get_proto_string( void ) noexcept
{
  return "MD_ZSET";
}

uint32_t
ZSetMsg::get_type_id( void ) noexcept
{
  return MD_ZSET;
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

static bool
is_zset( void *bb,  size_t off,  size_t &end )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off,
            msz = ListData::mem_size( buf, len, ZSetData::zst8_sig,
                                     ZSetData::zst16_sig, ZSetData::zst32_sig );
  end = off + msz;
  return msz != 0 && msz <= len;
}

bool
ZSetMsg::is_zsetmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  return is_zset( bb, off, end );
}

MDMsg *
ZSetMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                 MDMsgMem *m ) noexcept
{
  if ( ! is_zset( bb, off, end ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the ZSetMsg */
  void * ptr;
  m->alloc( sizeof( ZSetMsg ), &ptr );
  return new ( ptr ) ZSetMsg( bb, off, end, d, m );
}

void
ZSetMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( zsetmsg_match );
}

int
ZSetMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( ZSetFieldIter ), &ptr );
  iter = new ( ptr ) ZSetFieldIter( *this );
  return 0;
}

int
ZSetFieldIter::get_name( MDName &name ) noexcept
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
ZSetFieldIter::get_reference( MDReference &mref ) noexcept
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
ZSetFieldIter::first( void ) noexcept
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
ZSetFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  this->keylen      = 0;
  if ( this->field_start >= this->zset.hcount() )
    return Err::NOT_FOUND;
  this->field_end   = this->field_start + 1;
  return 0;
}

