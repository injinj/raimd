#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdbool.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdlib.h>
#include <raimd/md_replay.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/sass.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/rwf_writer.h>
#include <raimd/md_hash_tab.h>
#include <raimd/dict_load.h>

/* C version of read_msg.cpp using C API */

static const char*
get_arg( int argc, char* argv[], int b, const char* f, const char* def )
{
  for ( int i = 1; i < argc - b; i++ )
    if ( strcmp( f, argv[ i ] ) == 0 ) /* -p port */
      return argv[ i + b ];
  return def; /* default value */
}

typedef struct {
  MDSubjectKey_t base;
  uint16_t       rec_type, flist;
  uint32_t       seqno;
  MDFormClass_t* form;
} MetaData_t;

MetaData_t*
meta_data_make( const char* subj, size_t len, uint16_t rec_type, uint16_t flist,
                MDFormClass_t* form, uint32_t seqno )
{
  void* m = malloc( sizeof( MetaData_t ) + len + 1 );
  char* p = &( (char*) m )[ sizeof( MetaData_t ) ];
  memcpy( p, subj, len );
  p[ len ] = '\0';

  MetaData_t* md = (MetaData_t*) m;
  md_subject_key_init( &md->base, p, len );
  md->rec_type = rec_type;
  md->flist    = flist;
  md->seqno    = seqno;
  md->form     = form;

  return md;
}

void
get_subj_key( void* value, MDSubjectKey_t* k )
{
  k->subj = ( (MetaData_t*) value )->base.subj;
  k->len  = ( (MetaData_t*) value )->base.len;
}

int
main( int argc, char** argv )
{
  MDOutput_t *mout, *bout; /* message output, uses printf to stdout */
  MDMsgMem_t* mem; /* memory for printing (converting numbers to strings) */
  MDDict_t *  dict = NULL, /* dictionaries, cfile and RDM/appendix_a */
    *cfile_dict = NULL, *rdm_dict = NULL, *flist_dict = NULL;
  MDReplay_t * replay = NULL;
  MDHashTab_t* sub_ht = NULL;
  const char * fn     = get_arg( argc, argv, 1, "-f", NULL ),
             * out    = get_arg( argc, argv, 1, "-o", NULL ),
             * path   = get_arg( argc, argv, 1, "-p", getenv( "cfile_path" ) ),
             * fmt    = get_arg( argc, argv, 1, "-t", NULL );
  bool     quiet = get_arg( argc, argv, 0, "-q", NULL ) != NULL, binout = false;
  uint64_t msg_count = 0, err_cnt = 0, discard_cnt = 0;
  uint32_t cvt_type_id = 0;

  md_output_init( &mout );
  md_output_init( &bout );
  md_msg_mem_create( &mem );

  if ( get_arg( argc, argv, 0, "-h", NULL ) != NULL ) {
    fprintf(
      stderr,
      "Usage: %s [-f file] [-o out] [-p cfile_path] [-r flds] "
      "[-k flds] [-q]\n"
      "Test reading messages from files on command line\n"
      "  -f file           = replay file to read\n"
      "  -o file           = output file to write\n"
      "  -p cfile_path     = load dictionary files from path\n"
      "  -t format         = convert message to (eg TIBMSG,RVMSG,RWFMSG)\n"
      "  -q                = quiet, no printing of messages\n"
      "A file is a binary dump of messages, each with these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n",
      argv[ 0 ] );
    return 1;
  }

  if ( path != NULL )
    dict = md_load_dict_files( path, true );
  if ( dict != NULL ) {
    for ( MDDict_t* d = dict; d != NULL; d = d->next ) {
      if ( d->dict_type[ 0 ] == 'c' )
        cfile_dict = d;
      else if ( d->dict_type[ 0 ] == 'a' )
        rdm_dict = d;
      else if ( d->dict_type[ 0 ] == 'f' )
        flist_dict = d;
    }
  }
  else {
    fprintf( stderr, "no dictionary found (%s)\n", path );
  }

  if ( out != NULL ) {
    if ( strcmp( out, "-" ) == 0 )
      quiet = true;
    else if ( md_output_open( bout, out, "wb" ) != 0 ) {
      fprintf( stderr, "open error %s\n", out );
      return 1;
    }
    binout = true;
  }

  md_init_auto_unpack();

  if ( fmt != NULL ) {
    uint32_t   i;
    MDMatch_t* m;
    for ( m = md_msg_first_match( &i ); m; m = md_msg_next_match( &i ) ) {
      if ( strcasecmp( m->name, fmt ) == 0 ) {
        cvt_type_id = m->hint[ 0 ];
        break;
      }
    }

    switch ( cvt_type_id ) {
      case JSON_TYPE_ID:
      case MARKETFEED_TYPE_ID:
      case RVMSG_TYPE_ID:
      case RWF_FIELD_LIST_TYPE_ID:
      case RWF_MSG_TYPE_ID:
      case TIBMSG_TYPE_ID:
      case TIB_SASS_TYPE_ID: break;
      default:
        fprintf( stderr, "format \"%s\" %s, types:\n", fmt,
                 cvt_type_id == 0 ? "not found" : "converter not implemented" );
        for ( m = md_msg_first_match( &i ); m; m = md_msg_next_match( &i ) ) {
          if ( m->hint[ 0 ] != 0 )
            fprintf( stderr, "  %s : %x\n", m->name, m->hint[ 0 ] );
        }
        return 1;
    }
  }
  md_replay_create( &replay, NULL );

  if ( !md_replay_open( replay, fn == NULL ? "-" : fn ) ) {
    fprintf( stderr, "open %s failed\n", fn );
    return 1;
  }
  if ( !quiet )
    printf( "reading from %s\n", ( fn == NULL ? "stdin" : fn ) );

  md_hash_tab_create( &sub_ht, 128, get_subj_key );

  for ( bool b = md_replay_first( replay ); b; b = md_replay_next( replay ) ) {
    /* strip newline on subject */
    char*  subj  = replay->subj;
    size_t slen  = replay->subjlen;
    subj[ slen ] = '\0';

    /* try to unpack it */
    md_msg_mem_reuse( mem ); /* reset mem for next msg */
    MDMsg_t* m =
      md_msg_unpack( replay->msgbuf, 0, replay->msglen, 0, dict, mem );
    void*  msg    = replay->msgbuf;
    size_t msg_sz = replay->msglen;
    int    status = -1;

    if ( m == NULL ) {
      err_cnt++;
      continue;
    }

    if ( cvt_type_id != 0 && m != NULL ) {
      MDFormClass_t* form    = NULL;
      size_t         buf_sz  = ( msg_sz > 1024 ? msg_sz : 1024 );
      void*          buf_ptr = md_msg_mem_make( mem, buf_sz );
      uint16_t       flist = 0, msg_type = MD_UPDATE_TYPE, rec_type = 0;
      uint32_t       seqno = 0;

      switch ( md_msg_get_type_id( m ) ) {
        case MARKETFEED_TYPE_ID: {
          md_msg_get_sass_msg_type( m, &msg_type );
          md_msg_mf_get_flist( m, &flist );
          md_msg_mf_get_rtl( m, &seqno );
          break;
        }
        case RWF_MSG_TYPE_ID: {
          md_msg_get_sass_msg_type( m, &msg_type );
          MDMsg_t* fl = md_msg_rwf_get_container_msg( m );
          md_msg_rwf_get_flist( fl, &flist );
          md_msg_rwf_get_msg_seqnum( m, &seqno );
          break;
        }
        case RWF_FIELD_LIST_TYPE_ID: {
          md_msg_rwf_get_flist( m, &flist );
          break;
        }
        default: {
          MDFieldReader_t* rd = md_msg_get_field_reader( m );
          if ( md_field_reader_find( rd, MD_SASS_MSG_TYPE,
                                     MD_SASS_MSG_TYPE_LEN ) )
            md_field_reader_get_uint( rd, &msg_type, sizeof( msg_type ) );
          if ( md_field_reader_find( rd, MD_SASS_REC_TYPE,
                                     MD_SASS_REC_TYPE_LEN ) )
            md_field_reader_get_uint( rd, &rec_type, sizeof( rec_type ) );
          if ( md_field_reader_find( rd, MD_SASS_SEQ_NO, MD_SASS_SEQ_NO_LEN ) )
            md_field_reader_get_uint( rd, &seqno, sizeof( seqno ) );
          break;
        }
      }
      status = -1;

      if ( cvt_type_id == TIBMSG_TYPE_ID || cvt_type_id == TIB_SASS_TYPE_ID ) {
        if ( flist != 0 && flist_dict != NULL && cfile_dict != NULL ) {
          MDLookup_t *by = NULL, *fc = NULL;
          md_lookup_create_by_fid( &by, flist );

          if ( md_dict_lookup( flist_dict, by ) ) {
            md_lookup_create_by_name( &fc, by->fname, by->fname_len );

            if ( md_dict_get( cfile_dict, fc ) && fc->ftype == MD_MESSAGE ) {
              rec_type = fc->fid;
              if ( fc->map_num != 0 )
                form = md_dict_get_form_class( cfile_dict, fc );
              else
                form = NULL;

              MDSubjectKey_t k;
              md_subject_key_init( &k, subj, slen );

              MetaData_t* data;
              size_t      pos;

              data = (MetaData_t*) md_hash_tab_find( sub_ht, &k, &pos );
              if ( data == NULL ) {
                md_hash_tab_insert( sub_ht, pos,
                                    meta_data_make( subj, slen, rec_type, flist,
                                                    form, ++seqno ) );
              }
              else {
                data->rec_type = rec_type;
                data->flist    = flist;
                data->form     = form;
                seqno          = ++data->seqno;
              }
            }
          }
          if ( fc != NULL )
            md_lookup_destroy( fc );
          if ( by != NULL )
            md_lookup_destroy( by );
        }
      }
      else if ( cvt_type_id == RWF_MSG_TYPE_ID ||
                cvt_type_id == RWF_FIELD_LIST_TYPE_ID ) {
        if ( rec_type != 0 && flist_dict != NULL && cfile_dict != NULL ) {
          MDLookup_t *by = NULL, *fl = NULL;
          md_lookup_create_by_fid( &by, rec_type );

          if ( md_dict_lookup( cfile_dict, by ) && by->ftype == MD_MESSAGE ) {
            md_lookup_create_by_name( &fl, by->fname, by->fname_len );
            if ( md_dict_get( flist_dict, fl ) ) {
              flist = fl->fid;
            }
          }

          if ( fl != NULL )
            md_lookup_destroy( fl );
          if ( by != NULL )
            md_lookup_destroy( by );
        }
      }

      if ( seqno == 0 || cvt_type_id != 0 ) {
        MDSubjectKey_t k;
        md_subject_key_init( &k, subj, slen );

        MetaData_t* data;
        size_t      pos;

        if ( ( data = (MetaData_t*) md_hash_tab_find( sub_ht, &k, &pos ) ) ==
             NULL ) {
          md_hash_tab_insert(
            sub_ht, pos,
            meta_data_make( subj, slen, rec_type, flist, form, ++seqno ) );
        }
        else {
          if ( rec_type == 0 )
            rec_type = data->rec_type;
          if ( flist == 0 )
            flist = data->flist;
          if ( form == NULL )
            form = data->form;
          if ( seqno == 0 )
            seqno = ++data->seqno;
        }
      }

      switch ( cvt_type_id ) {
        case JSON_TYPE_ID: {
          MDMsgWriter_t* w =
            json_msg_writer_create( mem, NULL, buf_ptr, buf_sz );

          if ( ( status = md_msg_writer_convert_msg( w, m, false ) ) == 0 ) {
            msg    = w->buf;
            msg_sz = md_msg_writer_update_hdr( w );
            m      = json_msg_unpack( msg, 0, msg_sz, 0, dict, mem );
          }
          break;
        }
        case MARKETFEED_TYPE_ID: {
          break;
        }
        case RVMSG_TYPE_ID: {
          MDMsgWriter_t* w = rv_msg_writer_create( mem, NULL, buf_ptr, buf_sz );

          md_msg_writer_append_sass_hdr( w, form, msg_type, rec_type, seqno, 0,
                                         subj, slen );

          if ( ( status = md_msg_writer_convert_msg( w, m, false ) ) == 0 ) {
            msg    = w->buf;
            msg_sz = md_msg_writer_update_hdr( w );
            m      = rv_msg_unpack( msg, 0, msg_sz, 0, dict, mem );
          }
          break;
        }
        case RWF_MSG_TYPE_ID: {
          RwfMsgClass msg_class =
            ( msg_type == MD_INITIAL_TYPE ? REFRESH_MSG_CLASS
                                          : UPDATE_MSG_CLASS );
          uint32_t stream_id = md_dict_hash( subj, slen );

          MDMsgWriter_t* w =
            rwf_msg_writer_create( mem, rdm_dict, buf_ptr, buf_sz, msg_class,
                                   MARKET_PRICE_DOMAIN, stream_id );

          if ( msg_class == REFRESH_MSG_CLASS ) {
            md_msg_writer_rwf_set( w, X_CLEAR_CACHE );
            md_msg_writer_rwf_set( w, X_REFRESH_COMPLETE );
          }
          md_msg_writer_rwf_add_seq_num( w, seqno );
          md_msg_writer_rwf_add_msg_key( w, subj, slen );

          MDMsgWriter_t* fl = md_msg_writer_rwf_add_field_list( w );

          if ( flist != 0 )
            md_msg_writer_rwf_add_flist( fl, flist );

          status = w->err;

          if ( status == 0 )
            status = md_msg_writer_convert_msg( fl, m, true );

          if ( status == 0 )
            md_msg_writer_rwf_end_msg( w );

          if ( ( status = w->err ) == 0 ) {
            msg    = w->buf;
            msg_sz = w->off;
            m      = rwf_msg_unpack( msg, 0, msg_sz, 0, rdm_dict, mem );
          }
          break;
        }
        case RWF_FIELD_LIST_TYPE_ID: {
          MDMsgWriter_t* w =
            rwf_msg_writer_field_list_create( mem, rdm_dict, buf_ptr, buf_sz );

          if ( flist != 0 )
            md_msg_writer_rwf_add_flist( w, flist );

          if ( ( status = md_msg_writer_convert_msg( w, m, false ) ) == 0 ) {
            msg_sz = md_msg_writer_update_hdr( w );
            msg    = w->buf;
            m = rwf_msg_unpack_field_list( msg, 0, msg_sz, 0, rdm_dict, mem );
          }
          break;
        }
        case TIBMSG_TYPE_ID: {
          MDMsgWriter_t* w =
            tib_msg_writer_create( mem, NULL, buf_ptr, buf_sz );

          md_msg_writer_append_sass_hdr( w, form, msg_type, rec_type, seqno, 0,
                                         subj, slen );

          if ( ( status = md_msg_writer_convert_msg( w, m, true ) ) == 0 ) {
            msg    = w->buf;
            msg_sz = md_msg_writer_update_hdr( w );
            m      = tib_msg_unpack( msg, 0, msg_sz, 0, cfile_dict, mem );
          }
          break;
        }
        case TIB_SASS_TYPE_ID: {
          if ( form != NULL ) {
            MDMsgWriter_t* w = tib_sass_msg_writer_create_with_form(
              mem, form, buf_ptr, buf_sz );

            md_msg_writer_append_sass_hdr( w, form, msg_type, rec_type, seqno,
                                           0, subj, slen );

            if ( msg_type == MD_INITIAL_TYPE )
              md_msg_writer_append_form_record( w );

            if ( ( status = md_msg_writer_convert_msg( w, m, true ) ) == 0 ) {
              msg    = w->buf;
              msg_sz = md_msg_writer_update_hdr( w );
              m = tib_sass_msg_unpack( msg, 0, msg_sz, 0, cfile_dict, mem );
            }
          }
          else {
            MDMsgWriter_t* w =
              tib_sass_msg_writer_create( mem, cfile_dict, buf_ptr, buf_sz );

            md_msg_writer_append_sass_hdr( w, form, msg_type, rec_type, seqno,
                                           0, subj, slen );

            if ( ( status = md_msg_writer_convert_msg( w, m, true ) ) == 0 ) {
              msg    = w->buf;
              msg_sz = md_msg_writer_update_hdr( w );
              m = tib_sass_msg_unpack( msg, 0, msg_sz, 0, cfile_dict, mem );
            }
          }
          break;
        }
      }

      if ( status != 0 || m == NULL ) {
        if ( m != NULL ) {
          md_output_printf( mout, "%s: seqno %u error %d\n", subj, seqno,
                            status );
          md_msg_print( m, mout );
        }
        err_cnt++;
        continue;
      }
    }

    if ( !quiet && m != NULL ) {
      /* print it */
      md_output_printf( mout, "\n## %s (fmt %s)\n", subj,
                        md_msg_get_proto_string( m ) );
      md_msg_print( m, mout );
    }

    if ( binout ) {
      if ( m != NULL ) {
        uint32_t t = md_msg_get_type_id( m );
        if ( ( t == RWF_FIELD_LIST_TYPE_ID || t == RWF_MSG_TYPE_ID ) &&
             get_u32_md_big( msg ) != RWF_FIELD_LIST_TYPE_ID ) {
          uint8_t buf[ 8 ];
          set_u32_md_big( buf, RWF_FIELD_LIST_TYPE_ID );
          set_u32_md_big( &buf[ 4 ], md_msg_rwf_get_base_type_id( m ) );
          md_output_printf( bout, "%s\n%" PRIu64 "\n", subj,
                            (uint64_t) ( msg_sz + 8 ) );
          md_output_write( bout, buf, 8 );
        }
        else {
          md_output_printf( bout, "%s\n%" PRIu64 "\n", subj,
                            (uint64_t) msg_sz );
        }
      }
      if ( md_output_write( bout, msg, msg_sz ) != msg_sz )
        err_cnt++;
    }
    else {
      discard_cnt++;
    }
    msg_count++;
  }

  fprintf( stderr, "found %" PRIu64 " messages, %" PRIu64 " errors\n",
           msg_count, err_cnt );
  if ( discard_cnt > 0 )
    fprintf( stderr, "discarded %" PRIu64 "\n", discard_cnt );

  if ( md_output_close( mout ) != 0 )
    err_cnt++;
  if ( md_output_close( bout ) != 0 )
    err_cnt++;

  md_hash_tab_destroy( sub_ht );

  return err_cnt == 0 ? 0 : 2;
}
