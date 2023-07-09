#include <stdio.h>
#include <raimd/md_list.h>

using namespace rai;
using namespace md;

const char *
ListMsg::get_proto_string( void ) noexcept
{
  return "MD_LIST";
}

uint32_t
ListMsg::get_type_id( void ) noexcept
{
  return MD_LIST;
}

static MDMatch listmsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_LIST,
  .buf         = { ListData::lst8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = ListMsg::is_listmsg,
  .unpack      = ListMsg::unpack
};

static bool
is_list( void *bb,  size_t off,  size_t &end )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off,
            msz = ListData::mem_size( buf, len, ListData::lst8_sig,
                                     ListData::lst16_sig, ListData::lst32_sig );
  end = off + msz;
  return msz != 0 && msz <= len;
}

bool
ListMsg::is_listmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  return is_list( bb, off, end );
}

MDMsg *
ListMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                 MDMsgMem &m ) noexcept
{
  if ( ! is_list( bb, off, end ) )
    return NULL;
  /* check if another message is the first opaque field of the ListMsg */
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( ListMsg ), &ptr );
  return new ( ptr ) ListMsg( bb, off, end, d, m );
}

void
ListMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( listmsg_match );
}

int
ListMsg::get_reference( MDReference &mref ) noexcept
{
  mref.zero();
  mref.ftype = MD_LIST;
  mref.fptr  = (uint8_t *) this->msg_buf;
  mref.fptr  = &mref.fptr[ this->msg_off ];
  mref.fsize = this->msg_end - this->msg_off;
  return 0;
}

