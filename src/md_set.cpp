#include <stdio.h>
#include <raimd/md_set.h>

using namespace rai;
using namespace md;

const char *
SetMsg::get_proto_string( void ) noexcept
{
  return "MD_SET";
}

uint32_t
SetMsg::get_type_id( void ) noexcept
{
  return MD_SET;
}

static MDMatch setmsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_SET,
  .buf         = { SetData::set8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = SetMsg::is_setmsg,
  .unpack      = SetMsg::unpack
};

static bool
is_set( void *bb,  size_t off,  size_t &end )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off,
            msz = ListData::mem_size( buf, len, SetData::set8_sig,
                                      SetData::set16_sig, SetData::set32_sig );
  end = off + msz;
  return msz != 0 && msz <= len;
}

bool
SetMsg::is_setmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  return is_set( bb, off, end );
}

MDMsg *
SetMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                MDMsgMem &m ) noexcept
{
  if ( ! is_set( bb, off, end ) )
    return NULL;
  /* check if another message is the first opaque field of the SetMsg */
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( SetMsg ), &ptr );
  return new ( ptr ) SetMsg( bb, off, end, d, m );
}

void
SetMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( setmsg_match );
}

int
SetMsg::get_reference( MDReference &mref ) noexcept
{
  mref.zero();
  mref.ftype = MD_SET;
  mref.fptr  = (uint8_t *) this->msg_buf;
  mref.fptr  = &mref.fptr[ this->msg_off ];
  mref.fsize = this->msg_end - this->msg_off;
  return 0;
}

