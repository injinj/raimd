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

extern "C" {

int
md_msg_writer_append_iter( MDMsgWriter_t *w, MDFieldIter_t *iter )
{
  switch ( w->wr_type ) {
    case RVMSG_TYPE_ID:
      ((RvMsgWriter *) w)->append_iter( static_cast<MDFieldIter *>( iter ) );
      break;
    case TIBMSG_TYPE_ID:
      ((TibMsgWriter *) w)->append_iter( static_cast<MDFieldIter *>( iter ) );
      break;
    case TIB_SASS_TYPE_ID:
      ((TibSassMsgWriter *) w)->append_iter( static_cast<MDFieldIter *>( iter ) );
      break;
#if 0
    case JSON_TYPE_ID:
      ((JsonMsgWriter *) w)->append_iter( static_cast<MDFieldIter *>( iter ) );
      break;
    case RWF_MSG_TYPE_ID:
      ((RwfMsgWriter *) w)->append_iter( static_cast<MDFieldIter *>( iter ) );
      break;
#endif
    default:
      return -1;
  }
  return w->err;
}

size_t
md_msg_writer_update_hdr( MDMsgWriter_t *w )
{
  switch ( w->wr_type ) {
    case RVMSG_TYPE_ID:
      return ((RvMsgWriter *) w)->update_hdr();
    case TIBMSG_TYPE_ID:
      return ((TibMsgWriter *) w)->update_hdr();
    case TIB_SASS_TYPE_ID:
      return ((TibSassMsgWriter *) w)->update_hdr();
    case JSON_TYPE_ID:
      return ((JsonMsgWriter *) w)->update_hdr();
    case RWF_MSG_TYPE_ID:
      return ((RwfMsgWriter *) w)->update_hdr();
    case RWF_FIELD_LIST_TYPE_ID:
      return ((RwfFieldListWriter *) w)->update_hdr();
    default:
      return 0;
  }
}

int
md_msg_writer_convert_msg( MDMsgWriter_t *w, MDMsg_t *m, bool skip_hdr )
{
  switch ( w->wr_type ) {
    case RVMSG_TYPE_ID:
      ((RvMsgWriter *) w)->convert_msg( *static_cast<MDMsg *>( m ), skip_hdr );
      break;
    case TIBMSG_TYPE_ID:
      ((TibMsgWriter *) w)->convert_msg( *static_cast<MDMsg *>( m ), skip_hdr );
      break;
    case TIB_SASS_TYPE_ID:
      ((TibSassMsgWriter *) w)->convert_msg( *static_cast<MDMsg *>( m ), skip_hdr );
      break;
    case JSON_TYPE_ID:
      ((JsonMsgWriter *) w)->convert_msg( *static_cast<MDMsg *>( m ) );
      break;
    case RWF_MSG_TYPE_ID:
    case RWF_FIELD_LIST_TYPE_ID:
      ((RwfFieldListWriter *) w)->convert_msg( *static_cast<MDMsg *>( m ), skip_hdr );
      break;
    default:
      return -1;
  }
  return w->err;
}

int
md_msg_writer_append_sass_hdr( MDMsgWriter_t *w,  MDFormClass_t *form,
                               uint16_t msg_type,  uint16_t rec_type,
                               uint16_t seqno,  uint16_t status,
                               const char *subj,  size_t sublen )
{
  switch ( w->wr_type ) {
    case RVMSG_TYPE_ID:
      append_sass_hdr( *(RvMsgWriter *) w, (MDFormClass *) form, msg_type,
                       rec_type, seqno, status, subj, sublen );
      break;
    case TIBMSG_TYPE_ID:
      append_sass_hdr( *(TibMsgWriter *) w, (MDFormClass *) form, msg_type,
                       rec_type, seqno, status, subj, sublen );
      break;
    case TIB_SASS_TYPE_ID:
      append_sass_hdr( *(TibSassMsgWriter *) w, (MDFormClass *) form, msg_type,
                       rec_type, seqno, status, subj, sublen );
      break;
    case JSON_TYPE_ID:
      append_sass_hdr( *(JsonMsgWriter *) w, (MDFormClass *) form, msg_type,
                       rec_type, seqno, status, subj, sublen );
      break;
    case RWF_MSG_TYPE_ID:
    case RWF_FIELD_LIST_TYPE_ID:
      append_sass_hdr( *(RwfFieldListWriter *) w, (MDFormClass *) form, msg_type,
                       rec_type, seqno, status, subj, sublen );
      break;
    default:
      return -1;
  }
  return w->err;
}

int
md_msg_writer_append_form_record( MDMsgWriter_t *w )
{
  switch ( w->wr_type ) {
#if 0
    case RVMSG_TYPE_ID:
      return ((RvMsgWriter *) w)->append_form_record();
    case TIBMSG_TYPE_ID:
      return ((TibMsgWriter *) w)->append_form_record();
#endif
    case TIB_SASS_TYPE_ID:
      ((TibSassMsgWriter *) w)->append_form_record();
      break;
#if 0
    case JSON_TYPE_ID:
      return ((JsonMsgWriter *) w)->append_form_record();
    case RWF_MSG_TYPE_ID:
      return ((RwfMsgWriter *) w)->append_form_record();
#endif
    default:
      return -1;
  }
  return w->err;
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

}
