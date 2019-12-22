#include <stdio.h>
#include <raimd/md_stream.h>

using namespace rai;
using namespace md;

const char *
StreamMsg::get_proto_string( void )
{
  return "MD_STREAM";
}

uint32_t
StreamMsg::get_type_id( void )
{
  return MD_STREAM;
}

static MDMatch streammsg_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_STREAM,
  .buf         = { StreamData::str8_sig & 0xffU },
  .hint        = { 0 },
  .is_msg_type = StreamMsg::is_streammsg,
  .unpack      = StreamMsg::unpack
};

bool
StreamMsg::is_streammsg( void *bb,  size_t off,  size_t end,  uint32_t )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off, slen, glen, plen;
  return StreamGeom::get_stream_segments( buf, len, slen, glen, plen );
}

StreamMsg::StreamMsg( void *bb,  size_t off,  size_t end,  MDDict *d,
                      MDMsgMem *m ) : MDMsg( bb, off, end, d, m )
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  StreamGeom::get_stream_segments( buf, len, this->slen, this->glen,
                                   this->plen );
}

MDMsg *
StreamMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                   MDMsgMem *m )
{
  if ( ! is_streammsg( bb, off, end, h ) )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
  /* check if another message is the first opaque field of the StreamMsg */
  void * ptr;
  m->alloc( sizeof( StreamMsg ), &ptr );
  return new ( ptr ) StreamMsg( bb, off, end, d, m );
}

void
StreamMsg::init_auto_unpack( void )
{
  MDMsg::add_match( streammsg_match );
}

int
StreamMsg::get_field_iter( MDFieldIter *&iter )
{
  void * ptr;
  this->mem->alloc( sizeof( StreamFieldIter ), &ptr );
  iter = new ( ptr ) StreamFieldIter( *this );
  return 0;
}

int
StreamFieldIter::get_name( MDName &name )
{
  size_t i     = this->field_start,
         count = this->strm.stream.count();
  name.fid = 0;
  name.fnamelen = 1;
  if ( i < count )
    name.fname = "s";
  else {
    i -= count;
    count = this->strm.group.count();
    if ( i < count )
      name.fname = "g";
    else
      name.fname = "p";
  }
  return 0;
}

int
StreamFieldIter::get_reference( MDReference &mref )
{
  size_t  i     = this->field_start,
          count = this->strm.stream.count();
  ListVal lv;

  if ( i < count )
    this->strm.stream.lindex( i, lv );
  else {
    i -= count;
    count = this->strm.group.count();
    if ( i < count )
      this->strm.group.lindex( i, lv );
    else {
      i -= count;
      this->strm.pending.lindex( i, lv );
    }
  }
  mref.zero();
  mref.ftype = MD_LIST;
  mref.fptr  = (uint8_t *) lv.data;
  mref.fsize = lv.sz;
  if ( lv.sz2 > 0 ) {
    this->iter_msg.mem->alloc( lv.sz + lv.sz2, &mref.fptr );
    mref.fsize += lv.sz2;
    ::memcpy( mref.fptr, lv.data, lv.sz );
    ::memcpy( &mref.fptr[ lv.sz ], lv.data2, lv.sz2 );
  }
  return 0;
}

int
StreamFieldIter::first( void )
{
  this->field_start = 0;
  this->field_end   = 0;
  if ( this->strm.stream.count() + this->strm.group.count() +
       this->strm.pending.count() == 0 )
    return Err::NOT_FOUND;
  this->field_end   = 1;
  return 0;
}

int
StreamFieldIter::next( void )
{
  this->field_start = this->field_end;
  if ( this->field_start >= this->strm.stream.count() +
       this->strm.group.count() + this->strm.pending.count() )
    return Err::NOT_FOUND;
  this->field_end = this->field_start + 1;
  return 0;
}

