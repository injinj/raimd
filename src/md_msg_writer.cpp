#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdbool.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <raimd/md_dict.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/sass.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/rwf_writer.h>

using namespace rai;
using namespace md;

static inline MDMsgWriterBase &
no_impl( MDMsgWriterDispatch *self ) noexcept
{
  MDMsgWriterBase &b = *static_cast<MDMsgWriterBase *>( self );
  if ( b.err == 0 )
    b.err = Err::NO_MSG_IMPL;
  return b;
}

size_t
MDMsgWriterDispatch::update_hdr( void ) noexcept
{
  no_impl( this );
  return 0;
}

int
MDMsgWriterDispatch::append_iter( MDFieldIter * ) noexcept
{
  return no_impl( this ).err;
}

int
MDMsgWriterDispatch::convert_msg( MDMsg &, bool ) noexcept
{
  return no_impl( this ).err;
}

int
MDMsgWriterDispatch::append_sass_hdr( MDFormClass *, uint16_t, uint16_t,
                                      uint16_t, uint16_t, const char *,
                                      size_t ) noexcept
{
  return no_impl( this ).err;
}

int
MDMsgWriterDispatch::append_form_record( void ) noexcept
{
  return no_impl( this ).err;
}

static inline MDMsgWriterBase *
to_base( MDMsgWriter_t *w ) noexcept
{
  return static_cast<MDMsgWriterBase *>( w );
}

extern "C" {

size_t
md_msg_writer_update_hdr( MDMsgWriter_t *w )
{
  return to_base( w )->update_hdr();
}

int
md_msg_writer_append_iter( MDMsgWriter_t *w, MDFieldIter_t *iter )
{
  return to_base( w )->append_iter( static_cast<MDFieldIter *>( iter ) );
}

int
md_msg_writer_convert_msg( MDMsgWriter_t *w, MDMsg_t *m, bool skip_hdr )
{
  return to_base( w )->convert_msg( *static_cast<MDMsg *>( m ), skip_hdr );
}

int
md_msg_writer_append_sass_hdr( MDMsgWriter_t *w,  MDFormClass_t *form,
                               uint16_t msg_type,  uint16_t rec_type,
                               uint16_t seqno,  uint16_t status,
                               const char *subj,  size_t sublen )
{
  return to_base( w )->append_sass_hdr( (MDFormClass *) form, msg_type,
                                        rec_type, seqno, status, subj, sublen );
}

int
md_msg_writer_append_form_record( MDMsgWriter_t *w )
{
  return to_base( w )->append_form_record();
}

bool
md_msg_get_sass_msg_type( MDMsg_t *m,  uint16_t *msg_type )
{
  switch ( static_cast<MDMsg *>( m )->get_type_id() ) {
    case MARKETFEED_TYPE_ID: {
      MktfdMsg & mf = *static_cast<MktfdMsg *>( m );
      *msg_type = mf_func_to_sass_msg_type( mf.func );
      return true;
    }
    case RWF_MSG_TYPE_ID: {
      RwfMsg & rwf = *static_cast<RwfMsg *>( m );
      *msg_type = rwf_to_sass_msg_type( rwf );
      return true;
    }
    default: {
      MDFieldReader rd( *static_cast<MDMsg *>( m ) );
      if ( rd.find( MD_SASS_MSG_TYPE, MD_SASS_MSG_TYPE_LEN ) ) {
        rd.get_uint( *msg_type );
        return true;
      }
      break;
    }
  }
  return false;
}

} /* extern "C" */
