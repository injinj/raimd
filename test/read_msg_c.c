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

#define FIELD_INDEX_MASK 31

typedef struct {
  MDName_t * fld;
  size_t     idx_char;
  int        fld_cnt,
             fld_idx[ FIELD_INDEX_MASK + 1 ],
             fld_idx2[ FIELD_INDEX_MASK + 1 ];
} FieldIndex_t;

void
field_index_init( FieldIndex_t* fi )
{
  fi->fld = NULL;
  fi->idx_char = 0;
  fi->fld_cnt = 0;
}

bool
field_index_match( FieldIndex_t* fi, MDName_t* nm )
{
  if ( nm->fnamelen <= fi->idx_char )
    return false;
  int j = fi->fld_idx[ nm->fname[ fi->idx_char ] & 31 ],
      k = fi->fld_idx2[ nm->fname[ fi->idx_char / 2 ] & 31 ];
  if (j == 0 || k == 0)
    return false;
  if (k > j)
    j = k;
  for (; j <= fi->fld_cnt; j++) {
    if ( md_name_equals_name( nm, &fi->fld[j - 1] ) )
      return true;
  }
  return false;
}

void
field_index_index(FieldIndex_t* fi)
{
  int i, j;
  fi->idx_char = 0;
  memset(fi->fld_idx, 0, sizeof(fi->fld_idx));
  memset(fi->fld_idx2, 0, sizeof(fi->fld_idx2));
  for (i = 0; i < fi->fld_cnt; i++) {
    if (fi->idx_char == 0 || fi->fld[i].fnamelen < fi->idx_char)
      fi->idx_char = fi->fld[i].fnamelen;
  }
  fi->idx_char = (fi->idx_char >= 3 ? fi->idx_char - 2 : 1);
  for (i = 0; i < fi->fld_cnt; i++) {
    j = fi->fld[i].fname[fi->idx_char] & 31;
    if (fi->fld_idx[j] == 0) fi->fld_idx[j] = i + 1;
    j = fi->fld[i].fname[fi->idx_char / 2] & 31;
    if (fi->fld_idx2[j] == 0) fi->fld_idx2[j] = i + 1;
  }
}

int
filter(void* w, MDMsg_t* m, FieldIndex_t* rm,
       FieldIndex_t* kp, size_t* msg_sz, uint32_t* fldcnt)
{
  MDFieldIter_t* iter;
  MDName_t nm;
  int status;
  *fldcnt = 0;
  *msg_sz = 0;
  if ((status = md_msg_get_field_iter(m, &iter)) == 0) {
    if ((status = md_field_iter_first(iter)) == 0) {
      do {
        status = md_field_iter_get_name(iter, &nm);
        if (status == 0) {
          if ((rm->fld_cnt == 0 || !field_index_match(rm, &nm)) &&
               (kp->fld_cnt == 0 || field_index_match(kp, &nm))) {
            status = md_msg_writer_append_iter(w, iter);
            (*fldcnt)++;
          }
        }
        if (status != 0)
          break;
      } while ((status = md_field_iter_next(iter)) == 0);
    }
  }
  if (status != -2) /* Err::NOT_FOUND */
    return status;
  *msg_sz = md_msg_writer_update_hdr(w);
  return 0;
}

static void*
memdiff(void* msg, void* buf, size_t sz)
{
  for (size_t i = 0; i < sz; i++) {
    if (((uint8_t*)msg)[i] != ((uint8_t*)buf)[i])
      return &((uint8_t*)msg)[i];
  }
  return NULL;
}

void
cmp_msg(MDOutput_t* mout, MDMsg_t* m, MDDict_t* dict, void* msg, size_t msg_sz)
{
  MDMsgMem_t mem;
  void* p;
  int diff = 0, status, status2;
  uint8_t* buf = &((uint8_t*)m->msg_buf)[m->msg_off];
  size_t sz = m->msg_end - m->msg_off;

  md_msg_mem_init(&mem);

  if (msg_sz != sz) {
    md_output_printf(mout, "msg_sz %" PRIu64 " != %" PRIu64 "\n", (uint64_t)msg_sz, (uint64_t)sz);
    diff++;
  }
  size_t len = (msg_sz < sz ? msg_sz : sz);
  for (size_t off = 0; off < len;) {
    p = memdiff(&((char*)msg)[off], &buf[off], len - off);
    if (p == NULL)
      break;
    off = (char*)p - (char*)msg;
    md_output_printf(mout, "memcmp diff 0x%" PRIx64 "\n", (uint64_t)off);
    off++;
    diff++;
  }
  if (diff != 0) {
    md_output_printf(mout, "buf:\n");
    md_output_print_hex(mout, buf, sz);
    md_output_printf(mout, "msg:\n");
    md_output_print_hex(mout, msg, msg_sz);
  }
  
  MDMsg_t* m2 = md_msg_unpack(msg, 0, msg_sz, 0, dict, &mem);
  MDFieldIter_t *iter, *iter2;
  
  if (m2 != NULL) {
    if ((status = md_msg_get_field_iter(m, &iter)) == 0 &&
         (status2 = md_msg_get_field_iter(m2, &iter2)) == 0) {
      if ((status = md_field_iter_first(iter)) == 0 &&
           (status2 = md_field_iter_first(iter2)) == 0) {
        do {
          MDName_t n, n2;
          MDReference_t mref, href,
                        mref2, href2;
          bool equal = false;
          
          if (md_field_iter_get_name(iter, &n) == 0 &&
               md_field_iter_get_reference(iter, &mref) == 0) {
            md_field_iter_get_hint_reference(iter, &href);

            if (md_field_iter_get_name(iter2, &n2) == 0 &&
                 md_field_iter_get_reference(iter2, &mref2) == 0) {
              md_field_iter_get_hint_reference(iter2, &href2);

              if (md_name_equals_name(&n, &n2) &&
                  md_reference_equals(&mref, &mref2) &&
                  md_reference_equals(&href, &href2)) {
                equal = true;
              }
            }
          }
          if (!equal) {
            md_field_iter_print(iter, mout);
            md_field_iter_print(iter2, mout);
          }
        } while ((status = md_field_iter_next(iter)) == 0 &&
                  (status2 = md_field_iter_next(iter2)) == 0);
      }
    }
  }
  
  md_msg_mem_release(&mem);
}

typedef struct {
  char** sub;
  int sub_cnt;
} WildIndex_t;

void wild_index_init(WildIndex_t* wi) {
  wi->sub = NULL;
  wi->sub_cnt = 0;
}

bool
wild_index_match(WildIndex_t* wi, const char* subject)
{
  for (int i = 0; i < wi->sub_cnt; i++) {
    const char* w = wi->sub[i],
               * t = subject;
    for (;;) {
      if ((*w == '\0' && *t == '\0') || *w == '>')
        return true;
      if (*w == '\0' || *t == '\0')
        break;
      if (*w == '*') {
        w++;
        while (*t != '\0' && *t != '.')
          t++;
      }
      else if (*w++ != *t++)
        break;
    }
  }
  return false;
}

typedef struct {
  MDSubjectKey_t base;
  uint16_t rec_type,
           flist;
  uint32_t seqno;
  MDFormClass_t* form;
} MetaData_t;

MetaData_t*
meta_data_make(const char* subj, size_t len, uint16_t rec_type,
               uint16_t flist, MDFormClass_t* form, uint32_t seqno)
{
  void* m = malloc(sizeof(MetaData_t) + len + 1);
  char* p = &((char*)m)[sizeof(MetaData_t)];
  memcpy(p, subj, len);
  p[len] = '\0';
  
  MetaData_t* md = (MetaData_t*)m;
  md_subject_key_init(&md->base, p, len);
  md->rec_type = rec_type;
  md->flist = flist;
  md->seqno = seqno;
  md->form = form;
  
  return md;
}

void
get_subj_key( void *value,  MDSubjectKey_t *k )
{
  k->subj = ((MetaData_t *) value)->base.subj;
  k->len  = ((MetaData_t *) value)->base.len;
}

static const char* get_arg(int argc, char* argv[], int b, const char* f,
                         const char* def) {
  for (int i = 1; i < argc - b; i++)
    if (strcmp(f, argv[i]) == 0) /* -p port */
      return argv[i + b];
  return def; /* default value */
}

int
main(int argc, char** argv)
{
  MDOutput_t *mout, *bout; /* message output, uses printf to stdout */
  MDMsgMem_t mem;  /* memory for printing (converting numbers to strings) */
  MDDict_t * dict = NULL, /* dictionaries, cfile and RDM/appendix_a */
           * cfile_dict = NULL,
           * rdm_dict = NULL,
           * flist_dict = NULL;
  const char* fn = get_arg(argc, argv, 1, "-f", NULL),
            * out = get_arg(argc, argv, 1, "-o", NULL),
            * path = get_arg(argc, argv, 1, "-p", getenv("cfile_path")),
            * rmfld = get_arg(argc, argv, 1, "-r", NULL),
            * kpfld = get_arg(argc, argv, 1, "-k", NULL),
            * fmt = get_arg(argc, argv, 1, "-t", NULL),
            * sub = get_arg(argc, argv, 1, "-s", NULL);
  bool quiet = get_arg(argc, argv, 0, "-q", NULL) != NULL,
       binout = false;
  uint32_t cvt_type_id = 0;

  md_output_init(&mout);
  md_output_init(&bout);
  md_msg_mem_init(&mem);

  if (get_arg(argc, argv, 0, "-h", NULL) != NULL) {
    fprintf(stderr,
      "Usage: %s [-f file] [-o out] [-p cfile_path] [-r flds] "
                "[-k flds] [-s subs ] [-q]\n"
      "Test reading messages from files on command line\n"
      "  -f file           = replay file to read\n"
      "  -o file           = output file to write\n"
      "  -p cfile_path     = load dictionary files from path\n"
      "  -r fields         = remove fields from message\n"
      "  -k fields         = keep fields in message\n"
      "  -t format         = convert message to (eg TIBMSG,RVMSG,RWFMSG)\n"
      "  -s wildcard       = filter subjects using wildcard\n"
      "  -q                = quiet, no printing of messages\n"
      "A file is a binary dump of messages, each with these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n", argv[0]);
    return 1;
  }
  
  if (path != NULL)
    dict = md_load_dict_files(path, true);
  if (dict != NULL) {
    for (MDDict_t* d = dict; d != NULL; d = d->next) {
      if (d->dict_type[0] == 'c')
        cfile_dict = d;
      else if (d->dict_type[0] == 'a')
        rdm_dict = d;
      else if (d->dict_type[0] == 'f')
        flist_dict = d;
    }
  }
  
  if (out != NULL) {
    if (strcmp(out, "-") == 0)
      quiet = true;
    else if (md_output_open(bout, out, "wb") != 0) {
      perror(out);
      return 1;
    }
    binout = true;
  }
  
  md_init_auto_unpack();
  
  if (fmt != NULL) {
    uint32_t i;
    MDMatch_t * m;
    for (m = md_msg_first_match(&i); m; m = md_msg_next_match(&i)) {
      if (strcasecmp(m->name, fmt) == 0) {
        cvt_type_id = m->hint[0];
        break;
      }
    }
    
    switch (cvt_type_id) {
      case JSON_TYPE_ID:
      case MARKETFEED_TYPE_ID:
      case RVMSG_TYPE_ID:
      case RWF_FIELD_LIST_TYPE_ID:
      case RWF_MSG_TYPE_ID:
      case TIBMSG_TYPE_ID:
      case TIB_SASS_TYPE_ID:
        break;
      default:
        fprintf(stderr, "format \"%s\" %s, types:\n", fmt,
                 cvt_type_id == 0 ? "not found" : "converter not implemented");
        for (m = md_msg_first_match(&i); m; m = md_msg_next_match(&i)) {
          if (m->hint[0] != 0)
            fprintf(stderr, "  %s : %x\n", m->name, m->hint[0]);
        }
        return 1;
    }
  }
  
  if (!quiet) 
    printf("reading from %s\n", (fn == NULL ? "stdin" : fn));

  FieldIndex_t rm, kp;
  WildIndex_t wild;
  field_index_init(&rm);
  field_index_init(&kp);
  wild_index_init(&wild);
  
  rm.fld = (MDName_t*)malloc(sizeof(MDName_t) * argc * 2);
  kp.fld = &rm.fld[argc];
  wild.sub = (char**)malloc(sizeof(char*) * argc);
  
  int x = 0, y = 0, xx = 0, yy = 0, xxx = 0, yyy = 0;
  if (rmfld != NULL || kpfld != NULL || sub != NULL) {
    int i;
    for (i = 1; i < argc; i++) {
      memset(&rm.fld[i], 0, sizeof(MDName_t));
      memset(&kp.fld[i], 0, sizeof(MDName_t));
      
      if (argv[i] == rmfld) {
        x = i;
        y = argc;
      }
      else if (argv[i] == kpfld) {
        xx = i;
        yy = argc;
      }
      else if (argv[i] == sub) {
        xxx = i;
        yyy = argc;
      }
      else if (y == argc && argv[i][0] == '-')
        y = i;
      else if (yy == argc && argv[i][0] == '-')
        yy = i;
      else if (yyy == argc && argv[i][0] == '-')
        yyy = i;
      
      if (y == argc) {
        rm.fld[i].fname = argv[i];
        rm.fld[i].fnamelen = strlen(argv[i]) + 1;
      }
      else if (yy == argc) {
        kp.fld[i].fname = argv[i];
        kp.fld[i].fnamelen = strlen(argv[i]) + 1;
      }
      else if (yyy == argc) {
        wild.sub[i] = argv[i];
      }
    }
    
    rm.fld = &rm.fld[x];
    rm.fld_cnt = y - x;
    kp.fld = &kp.fld[xx];
    kp.fld_cnt = yy - xx;
    wild.sub = &wild.sub[xxx];
    wild.sub_cnt = yyy - xxx;
    
    field_index_index(&rm);
    field_index_index(&kp);
  }

  /* Read subject, size, message data */
  FILE* filep = (fn == NULL ? stdin : fopen(fn, "rb"));
  if (filep == NULL) {
    perror(fn);
    return 1;
  }
  
  MDReplay_t replay;
  md_replay_init(&replay, filep);
  
  MDHashTab_t sub_ht;
  uint64_t msg_count = 0,
           err_cnt = 0,
           discard_cnt = 0;

  md_hash_tab_init( &sub_ht, 128, get_subj_key );
  for (bool b = md_replay_first(&replay); b; b = md_replay_next(&replay)) {
    /* strip newline on subject */
    char* subj = replay.subj;
    size_t slen = replay.subjlen;
    subj[slen] = '\0';
    
    if (wild.sub_cnt != 0 && !wild_index_match(&wild, subj))
      continue;

    /* try to unpack it */
    md_msg_mem_reuse(&mem); /* reset mem for next msg */
    MDMsg_t* m = md_msg_unpack(replay.msgbuf, 0, replay.msglen, 0, dict, &mem);
    void* msg = replay.msgbuf;
    size_t msg_sz = replay.msglen;
    uint32_t fldcnt = 0;
    int status = -1;

    if (m == NULL) {
      err_cnt++;
      continue;
    }
    
    if (rm.fld_cnt != 0 || kp.fld_cnt != 0) {
      size_t buf_sz = msg_sz * 2;
      void* buf_ptr = md_msg_mem_make(&mem, buf_sz);
      
      if (md_msg_get_type_id(m) == TIBMSG_TYPE_ID) {
        MDMsgWriter_t* w = tib_msg_writer_create(&mem, cfile_dict, buf_ptr, buf_sz);
        status = filter(w, m, &rm, &kp, &msg_sz, &fldcnt);
        m = NULL;
        
        if (status == 0 && fldcnt > 0) {
          msg = w->buf;
          m = tib_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
        }
      }
      else if (md_msg_get_type_id(m) == RVMSG_TYPE_ID) {
        MDMsgWriter_t* w = rv_msg_writer_create(&mem, cfile_dict, buf_ptr, buf_sz);
        status = filter(w, m, &rm, &kp, &msg_sz, &fldcnt);
        m = NULL;
        
        if (status == 0 && fldcnt > 0) {
          msg = w->buf;
          m = rv_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
        }
      }
      else if (md_msg_get_type_id(m) == TIB_SASS_TYPE_ID) {
        MDMsgWriter_t* w = tib_sass_msg_writer_create(&mem, cfile_dict, buf_ptr, buf_sz);
        status = filter(w, m, &rm, &kp, &msg_sz, &fldcnt);
        m = NULL;
        
        if (status == 0 && fldcnt > 0) {
          msg = w->buf;
          m = tib_sass_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
        }
      }
      
      if (status != 0) {
        err_cnt++;
        continue;
      }
    }
    
    if (cvt_type_id != 0 && m != NULL) {
      MDFormClass_t* form = NULL;
      size_t buf_sz = msg_sz;
      void* buf_ptr = md_msg_mem_make(&mem, buf_sz);
      uint16_t flist = 0,
               msg_type = MD_UPDATE_TYPE,
               rec_type = 0;
      uint32_t seqno = 0;

      switch (md_msg_get_type_id(m)) {
        case MARKETFEED_TYPE_ID: {
          md_msg_get_sass_msg_type( m, &msg_type );
          md_msg_mf_get_flist(m, &flist);
          md_msg_mf_get_rtl(m, &seqno);
          break;
        }
        case RWF_MSG_TYPE_ID: {
          md_msg_get_sass_msg_type( m, &msg_type );
          MDMsg_t * fl = md_msg_rwf_get_container_msg( m );
          md_msg_rwf_get_flist( fl, &flist );
          md_msg_rwf_get_msg_seqnum( m, &seqno );
          break;
        }
        case RWF_FIELD_LIST_TYPE_ID: {
          md_msg_rwf_get_flist( m, &flist );
          break;
        }
        default: {
          MDFieldReader_t* rd = md_msg_get_field_reader(m);
          if (md_field_reader_find(rd, MD_SASS_MSG_TYPE, MD_SASS_MSG_TYPE_LEN))
            md_field_reader_get_uint(rd, &msg_type, sizeof(msg_type));
          if (md_field_reader_find(rd, MD_SASS_SEQ_NO, MD_SASS_SEQ_NO_LEN))
            md_field_reader_get_uint(rd, &seqno, sizeof(seqno));
          break;
        }
      }
      
      status = -1;
      
      if (cvt_type_id == TIBMSG_TYPE_ID || cvt_type_id == TIB_SASS_TYPE_ID) {
        if (flist != 0 && flist_dict != NULL && cfile_dict != NULL) {
          MDLookup_t by;
          md_lookup_init_by_fid(&by, flist);
          
          if (md_dict_lookup(flist_dict, &by)) {
            MDLookup_t fc;
            md_lookup_init_by_name(&fc, by.fname, by.fname_len);
            
            if (md_dict_get(cfile_dict, &fc) && fc.ftype == MD_MESSAGE) {
              rec_type = fc.fid;
              if (fc.map_num != 0)
                form = md_dict_get_form_class(cfile_dict, &fc);
              else
                form = NULL;
              
              MDSubjectKey_t k;
              md_subject_key_init( &k, subj, slen );
              
              MetaData_t* data;
              size_t pos;
              
              data = (MetaData_t*) md_hash_tab_find( &sub_ht, &k, &pos );
              if ( data == NULL ) {
                md_hash_tab_insert(&sub_ht, pos, meta_data_make(subj, slen, rec_type, flist, form, ++seqno));
              }
              else {
                data->rec_type = rec_type;
                data->flist = flist;
                data->form = form;
                seqno = ++data->seqno;
              }
            }
          }
        }
        else {
          MDSubjectKey_t k;
          md_subject_key_init(&k, subj, slen);
          
          MetaData_t* data = (MetaData_t*)md_hash_tab_find(&sub_ht, &k, NULL);
          if (data != NULL) {
            rec_type = data->rec_type;
            flist = data->flist;
            form = data->form;
            seqno = ++data->seqno;
          }
        }
      }
      
      if (seqno == 0) {
        MDSubjectKey_t k;
        md_subject_key_init(&k, subj, slen);
        
        MetaData_t* data;
        size_t pos;
        
        if ((data = (MetaData_t*)md_hash_tab_find(&sub_ht, &k, &pos)) == NULL) {
          md_hash_tab_insert(&sub_ht, pos, meta_data_make(subj, slen, rec_type, flist, form, ++seqno));
        }
        else {
          rec_type = data->rec_type;
          flist = data->flist;
          form = data->form;
          seqno = ++data->seqno;
        }
      }
      
      switch (cvt_type_id) {
        case JSON_TYPE_ID: {
          MDMsgWriter_t* w = json_msg_writer_create(&mem, NULL, buf_ptr, buf_sz);
          
          if ( (status = md_msg_writer_convert_msg(w, m, false)) == 0 ) {
            msg    = w->buf;
            msg_sz = md_msg_writer_update_hdr(w);
            m = json_msg_unpack(msg, 0, msg_sz, 0, dict, &mem);
          }
          break;
        }
        case MARKETFEED_TYPE_ID: {
          break;
        }
        case RVMSG_TYPE_ID: {
          MDMsgWriter_t* w = rv_msg_writer_create(&mem, NULL, buf_ptr, buf_sz);
          
          md_msg_writer_append_sass_hdr(w, form, msg_type, rec_type, seqno, 0, subj, slen);
          
          if ( (status = md_msg_writer_convert_msg(w, m, false)) == 0 ) {
            msg    = w->buf;
            msg_sz = md_msg_writer_update_hdr(w);
            m = rv_msg_unpack(msg, 0, msg_sz, 0, dict, &mem);
          }
          break;
        }
        case RWF_MSG_TYPE_ID: {
          RwfMsgClass msg_class = (msg_type == MD_INITIAL_TYPE ?
                                   REFRESH_MSG_CLASS : UPDATE_MSG_CLASS);
          uint32_t stream_id = md_dict_hash(subj, slen);
          
          MDMsgWriter_t* w = rwf_msg_writer_create(&mem, rdm_dict, buf_ptr, buf_sz,
                                              msg_class, MARKET_PRICE_DOMAIN, stream_id);
          
          if (msg_class == REFRESH_MSG_CLASS) {
            md_msg_writer_rwf_set(w, X_CLEAR_CACHE );
            md_msg_writer_rwf_set(w, X_REFRESH_COMPLETE);
          }
          md_msg_writer_rwf_add_seq_num(w, seqno);
          md_msg_writer_rwf_add_msg_key(w, subj, slen);
          
          MDMsgWriter_t* fl = md_msg_writer_rwf_add_field_list(w);
          
          if (flist != 0)
            md_msg_writer_rwf_add_flist(fl, flist);
          
          status = w->err;
          
          if (status == 0)
            status = md_msg_writer_convert_msg(fl, m, true);
          
          if (status == 0)
            md_msg_writer_rwf_end_msg(w);
          
          if ((status = w->err) == 0) {
            msg = w->buf;
            msg_sz = w->off;
            m = rwf_msg_unpack(msg, 0, msg_sz, 0, rdm_dict, &mem);
          }
          break;
        }
        case RWF_FIELD_LIST_TYPE_ID: {
          MDMsgWriter_t* w = rwf_msg_writer_field_list_create(&mem, rdm_dict, buf_ptr, buf_sz);
          
          if (flist != 0)
            md_msg_writer_rwf_add_flist(w, flist);
          
          if ((status = md_msg_writer_convert_msg(w, m, false)) == 0) {
            msg = w->buf;
            msg_sz = md_msg_writer_update_hdr(w);
            m = rwf_msg_unpack_field_list(msg, 0, msg_sz, 0, rdm_dict, &mem);
          }
          break;
        }
        case TIBMSG_TYPE_ID: {
          MDMsgWriter_t * w = tib_msg_writer_create(&mem, NULL, buf_ptr, buf_sz);
          
          md_msg_writer_append_sass_hdr(w, form, msg_type, rec_type, seqno, 0, subj, slen);
          
          if ((status = md_msg_writer_convert_msg(w, m, true)) == 0) {
            msg = w->buf;
            msg_sz = md_msg_writer_update_hdr(w);
            m = tib_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
          }
          break;
        }
        case TIB_SASS_TYPE_ID: {
          if (form != NULL) {
            MDMsgWriter_t * w = tib_sass_msg_writer_create_with_form(&mem, form, buf_ptr, buf_sz);
            
            md_msg_writer_append_sass_hdr(w, form, msg_type, rec_type, seqno, 0, subj, slen);
            
            if (msg_type == MD_INITIAL_TYPE)
              md_msg_writer_append_form_record(w);
            
            if ((status = md_msg_writer_convert_msg(w, m, true)) == 0) {
              msg = w->buf;
              msg_sz = md_msg_writer_update_hdr(w);
              m = tib_sass_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
            }
          }
          else {
            MDMsgWriter_t * w = tib_sass_msg_writer_create(&mem, cfile_dict, buf_ptr, buf_sz);
            
            md_msg_writer_append_sass_hdr(w, form, msg_type, rec_type, seqno, 0, subj, slen);
            
            if ((status = md_msg_writer_convert_msg(w, m, true)) == 0) {
              msg = w->buf;
              msg_sz = md_msg_writer_update_hdr(w);
              m = tib_sass_msg_unpack(msg, 0, msg_sz, 0, cfile_dict, &mem);
            }
          }
          break;
        }
      }
      
      if (status != 0 || m == NULL) {
        if (m != NULL) {
          md_output_printf(mout, "%s: seqno %u error %d\n", subj, seqno, status);
          md_msg_print(m, mout);
        }
        err_cnt++;
        continue;
      }
    }
    
    if (!quiet && m != NULL) {
      /* print it */
      md_output_printf(mout, "\n## %s (fmt %s)\n", subj, md_msg_get_proto_string(m));
      md_msg_print(m, mout);
    }
    
    if (binout) {
      if (m != NULL) {
#if 0
        switch (md_msg_get_type_id(m)) {
          case RWF_FIELD_LIST_TYPE_ID:
          case RWF_MSG_TYPE_ID:
            if (get_u32_md_big(msg) != RWF_FIELD_LIST_TYPE_ID) {
              uint8_t buf[8];
              set_u32_md_big(buf, RWF_FIELD_LIST_TYPE_ID);
              set_u32_md_big(&buf[4], ((RwfMsg*)m)->base.type_id);
              md_output_printf(bout, "%s\n%" PRIu64 "\n", subj, (uint64_t)(msg_sz + 8));
              md_output_write(bout, buf, 8);
              break;
            }
            /* FALLTHRU */
          default:
            md_output_printf(bout, "%s\n%" PRIu64 "\n", subj, (uint64_t)msg_sz);
            break;
        }
#endif
        md_output_printf(bout, "%s\n%" PRIu64 "\n", subj, (uint64_t)msg_sz);
        if (md_output_write(bout, msg, msg_sz) != msg_sz)
          err_cnt++;
      }
      else {
        discard_cnt++;
      }
    }
    msg_count++;
  }
  
  fprintf(stderr, "found %" PRIu64 " messages, %" PRIu64 " errors\n",
           msg_count, err_cnt);
  if (discard_cnt > 0)
    fprintf(stderr, "discarded %" PRIu64 "\n", discard_cnt);
  
  if (fn != NULL && filep != NULL)
    fclose(filep);
  
  if (md_output_close(bout) != 0) {
    perror(out);
    err_cnt++;
  }
  
  free(rm.fld);
  free(wild.sub);
  md_hash_tab_release(&sub_ht);
  
  return err_cnt == 0 ? 0 : 2;
}
