#include <stdio.h>
#include <raimd/md_list.h>

using namespace rai;
using namespace md;

const char *
ListMsg::get_proto_string( void )
{
  return "MD_LIST";
}

uint32_t
ListMsg::get_type_id( void )
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

static inline bool is_mask( uint32_t m ) { return ( m & ( m + 1 ) ) == 0; }

bool
ListMsg::is_listmsg( void *bb,  size_t off,  size_t end,  uint32_t )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  if ( len >= 4 && get_u16<MD_LITTLE>( buf ) == ListData::lst8_sig &&
       is_mask( buf[ 2 ] ) && is_mask( buf[ 3 ] ) )
    return true;
  if ( len >= 8 && get_u32<MD_LITTLE>( buf ) == ListData::lst16_sig &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 4 ] ) ) &&
       is_mask( get_u16<MD_LITTLE>( &buf[ 6 ] ) ) )
    return true;
  if ( len >= 16 && get_u64<MD_LITTLE>( buf ) == ListData::lst32_sig &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 8 ] ) ) &&
       is_mask( get_u32<MD_LITTLE>( &buf[ 12 ] ) ) )
    return true;
  return false;
}

MDMsg *
ListMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                 MDMsgMem *m )
{
  if ( ! is_listmsg( bb, off, end, h ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the ListMsg */
  void * ptr;
  m->alloc( sizeof( ListMsg ), &ptr );
  return new ( ptr ) ListMsg( bb, off, end, d, m );
}

void
ListMsg::init_auto_unpack( void )
{
  MDMsg::add_match( listmsg_match );
}

int
ListMsg::get_reference( MDReference &mref )
{
  mref.zero();
  mref.ftype = MD_LIST;
  mref.fptr  = (uint8_t *) this->msg_buf;
  mref.fptr  = &mref.fptr[ this->msg_off ];
  mref.fsize = this->msg_end - this->msg_off;
  return 0;
}

