#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/enum_def.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>

using namespace rai;
using namespace md;

static MDDict *
load_dict_files( const char *path ) noexcept
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  int x, y;
  dict_build.debug_flags = MD_DICT_PRINT_FILES;
  if ( (x = CFile::parse_path( dict_build, path, "tss_fields.cf" )) == 0 ) {
    CFile::parse_path( dict_build, path, "tss_records.cf" );
    dict_build.index_dict( "cfile", dict ); /* dict contains index */
  }
  dict_build.clear_build(); /* frees temp memory used to index dict */
  if ( (y = AppA::parse_path( dict_build, path, "RDMFieldDictionary" )) == 0){
    EnumDef::parse_path( dict_build, path, "enumtype.def" );
    dict_build.index_dict( "app_a", dict ); /* dict is a list */
  }
  dict_build.clear_build();
  if ( dict != NULL ) { /* print which dictionaries loaded */
    fprintf( stderr, "%s dict loaded (size: %u)\n", dict->dict_type,
             dict->dict_size );
    if ( dict->next != NULL )
      fprintf( stderr, "%s dict loaded (size: %u)\n", dict->next->dict_type,
               dict->next->dict_size );
    return dict;
  }
  fprintf( stderr, "cfile status %d+%s, RDM status %d+%s\n",
          x, Err::err( x )->descr, y, Err::err( y )->descr );
  return NULL;
}

struct FieldIndex {
  static const uint32_t mask = 31;
  MDName * fld;
  size_t   idx_char;
  int      fld_cnt,
           fld_idx[ mask + 1 ],
           fld_idx2[ mask + 1 ];
  FieldIndex() : fld( 0 ), idx_char( 0 ), fld_cnt( 0 ) {}

  bool match( MDName &nm ) {
    if ( nm.fnamelen <= this->idx_char )
      return false;
    int j = this->fld_idx[ nm.fname[ this->idx_char ] & mask ],
        k = this->fld_idx2[ nm.fname[ this->idx_char / 2 ] & mask ];
    if ( j == 0 || k == 0 )
      return false;
    if ( k > j )
      j = k;
    for ( ; j <= this->fld_cnt; j++ ) {
      if ( nm.equals( this->fld[ j - 1 ] ) )
        return true;
    }
    return false;
  }
  void index( void ) {
    int i, j;
    this->idx_char = 0;
    ::memset( this->fld_idx, 0, sizeof( this->fld_idx ) );
    ::memset( this->fld_idx2, 0, sizeof( this->fld_idx2 ) );
    for ( i = 0; i < this->fld_cnt; i++ ) {
      if ( this->idx_char == 0 || this->fld[ i ].fnamelen < this->idx_char )
        this->idx_char = this->fld[ i ].fnamelen;
    }
    this->idx_char = ( this->idx_char >= 3 ? this->idx_char - 2 : 1 );
    for ( i = 0; i < this->fld_cnt; i++ ) {
      j = this->fld[ i ].fname[ this->idx_char ] & mask;
      if ( this->fld_idx[ j ] == 0 ) this->fld_idx[ j ] = i + 1;
      j = this->fld[ i ].fname[ this->idx_char / 2 ] & mask;
      if ( this->fld_idx2[ j ] == 0 ) this->fld_idx2[ j ] = i + 1;
    }
  }
};

template< class Writer >
int
filter( Writer &w,  MDMsg *m,  FieldIndex &rm,  FieldIndex &kp,
        size_t &msg_sz )
{
  MDFieldIter *iter;
  MDName nm;
  int status, fldcnt = 0;
  msg_sz = 0;
  if ( (status = m->get_field_iter( iter )) == 0 ) {
    if ( (status = iter->first()) == 0 ) {
      do {
        status = iter->get_name( nm );
        if ( status == 0 ) {
          if ( ( rm.fld_cnt == 0 || ! rm.match( nm ) ) &&
               ( kp.fld_cnt == 0 || kp.match( nm ) ) ) {
            status = w.append_iter( iter );
            fldcnt++;
          }
        }
        if ( status != 0 )
          break;
      } while ( (status = iter->next()) == 0 );
    }
  }
  if ( status != Err::NOT_FOUND )
    return 0;
  msg_sz = w.update_hdr();
  return fldcnt;
}

static void *
memdiff( void *msg, void *buf, size_t sz ) noexcept
{
  for ( size_t i = 0; i < sz; i++  ) {
    if ( ((uint8_t *) msg)[ i ] != ((uint8_t *) buf)[ i ] )
      return &((uint8_t *) msg)[ i ];
  }
  return NULL;
}

void
cmp_msg( MDOutput &mout,  MDMsg *m,  MDDict *dict,  void *msg,
         size_t msg_sz ) noexcept
{
  MDMsgMem mem;
  void * p;
  int diff = 0, status, status2;
  uint8_t * buf = &((uint8_t *) m->msg_buf)[ m->msg_off ];
  size_t sz = m->msg_end - m->msg_off;

  if ( msg_sz != sz ) {
    mout.printf( "msg_sz %" PRIu64 " != %" PRIu64 "\n", msg_sz, sz );
    diff++;
  }
  size_t len = ( msg_sz < sz ? msg_sz : sz );
  for ( size_t off = 0; off < len; ) {
    p = memdiff( &((char *) msg)[ off ], &buf[ off ], len - off );
    if ( p == NULL )
      break;
    off = (char *) p - (char *) msg;
    mout.printf( "memcmp diff 0x%" PRIx64 "\n", off );
    off++;
    diff++;
  }
  if ( diff != 0 ) {
    mout.printf( "buf:\n" );
    mout.print_hex( buf, sz );
    mout.printf( "msg:\n" );
    mout.print_hex( msg, msg_sz );
  }
  MDMsg * m2 = MDMsg::unpack( msg, 0, msg_sz, 0, dict, mem );
  MDFieldIter *iter, *iter2;
  if ( m2 != NULL ) {
    if ( (status = m->get_field_iter( iter )) == 0 &&
         (status2 = m2->get_field_iter( iter2 )) == 0 ) {
      if ( (status = iter->first()) == 0 &&
           (status2 = iter2->first()) == 0 ) {
        do {
          MDName      n, n2;
          MDReference mref, href,
                      mref2, href2;
          bool        equal = false;
          if ( iter->get_name( n ) == 0 &&
               iter->get_reference( mref ) == 0 ) {
            iter->get_hint_reference( href );

            if ( iter->get_name( n2 ) == 0 &&
                 iter->get_reference( mref2 ) == 0 ) {
              iter->get_hint_reference( href2 );

              if ( n.equals( n2 ) && mref.equals( mref2 ) &&
                   href.equals( href2 ) ) {
                equal = true;
              }
            }
          }
          if ( ! equal ) {
            iter->print( &mout );
            iter2->print( &mout );
          }
        } while ( (status = iter->next()) == 0 &&
                  (status2 = iter2->next()) == 0 );
      }
    }
  }
}

struct WildIndex {
  char  ** sub;
  int      sub_cnt;
  WildIndex() : sub( 0 ), sub_cnt( 0 ) {}

  bool match( const char *subject ) {
    for ( int i = 0; i < this->sub_cnt; i++ ) {
      const char * w = this->sub[ i ],
                 * t = subject;
      for (;;) {
        if ( ( *w == '\0' && *t == '\0' ) || *w == '>' )
          return true;
        if ( *w == '\0' || *t == '\0' )
          break;
        if ( *w == '*' ) {
          w++;
          while ( *t != '\0' && *t != '.' )
            t++;
        }
        else if ( *w++ != *t++ )
          break;
      }
    }
    return false;
  }
};

static const char *
get_arg( int argc, char *argv[], int b, const char *f,
         const char *def ) noexcept
{
  for ( int i = 1; i < argc - b; i++ )
    if ( ::strcmp( f, argv[ i ] ) == 0 ) /* -p port */
      return argv[ i + b ];
  return def; /* default value */
}

int
main( int argc, char **argv )
{
  MDOutput mout, bout; /* message output, uses printf to stdout */
  MDMsgMem mem;  /* memory for printing (converting numbers to strings) */
  MDDict * dict = NULL; /* dictinonaries, cfile and RDM/appendix_a */
  char subj[ 1024 ], size[ 128 ];
  size_t sz, buflen = 0;
  const char * fn     = get_arg( argc, argv, 1, "-f", NULL ),
             * out    = get_arg( argc, argv, 1, "-o", NULL ),
             * path   = get_arg( argc, argv, 1, "-p", ::getenv( "cfile_path" ) ),
             * rmfld  = get_arg( argc, argv, 1, "-r", NULL ),
             * kpfld  = get_arg( argc, argv, 1, "-k", NULL ),
             * sub    = get_arg( argc, argv, 1, "-s", NULL );
  bool         quiet  = get_arg( argc, argv, 0, "-q", NULL ) != NULL,
               binout = false;
  char * buf = NULL;
  uint64_t msg_count = 0, err_cnt = 0, discard_cnt = 0;

  if ( get_arg( argc, argv, 0, "-h", NULL ) != NULL ) {
    fprintf( stderr,
      "Usage: %s [-f file] [-o out] [-p cfile_path] [-r flds] "
                "[-k flds] [-s subs ] [-q]\n"
      "Test reading messages from files on command line\n"
      "Each file is formatted as any number of these lines:\n"
      "<subject>\n"
      "<num-bytes>\n"
      "<blob-of-data\n"
      "\n"
      "Example:\n"
      "test.subject\n"
      "15\n"
      "{ \"ten\" : 10 }\n", argv[ 0 ] );
    return 1;
  }
  if ( path != NULL )
    dict = load_dict_files( path );
  if ( out != NULL ) {
    if ( ::strcmp( out, "-" ) == 0 )
      quiet = true;
    else if ( bout.open( out, "wb" ) != 0 ) {
      perror( out );
      return 1;
    }
    binout = true;
  }
  if ( ! quiet ) 
    printf( "reading from %s\n", ( fn == NULL ? "stdin" : fn ) );
  FieldIndex rm, kp;
  WildIndex  wild;
  rm.fld = (MDName *) ::malloc( sizeof( MDName ) * argc * 2 );
  kp.fld = &rm.fld[ argc ];
  wild.sub = (char **) ::malloc( sizeof( char * ) * argc );
  int x = 0, y = 0, xx = 0, yy = 0, xxx = 0, yyy = 0;
  if ( rmfld != NULL || kpfld != NULL || sub != NULL ) {
    int i;
    for ( i = 1; i < argc; i++ ) {
      rm.fld[ i ].zero();
      kp.fld[ i ].zero();
      if ( argv[ i ] == rmfld ) {
        x = i;
        y = argc;
      }
      else if ( argv[ i ] == kpfld ) {
        xx = i;
        yy = argc;
      }
      else if ( argv[ i ] == sub ) {
        xxx = i;
        yyy = argc;
      }
      else if ( y == argc && argv[ i ][ 0 ] == '-' )
        y = i;
      else if ( yy == argc && argv[ i ][ 0 ] == '-' )
        yy = i;
      else if ( yyy == argc && argv[ i ][ 0 ] == '-' )
        yyy = i;
      if ( y == argc ) {
        rm.fld[ i ].fname = argv[ i ];
        rm.fld[ i ].fnamelen = ::strlen( argv[ i ] ) + 1;
      }
      else if ( yy == argc ) {
        kp.fld[ i ].fname = argv[ i ];
        kp.fld[ i ].fnamelen = ::strlen( argv[ i ] ) + 1;
      }
      else if ( yyy == argc ) {
        wild.sub[ i ] = argv[ i ];
      }
    }
    rm.fld       = &rm.fld[ x ];
    rm.fld_cnt   = y - x;
    kp.fld       = &kp.fld[ xx ];
    kp.fld_cnt   = yy - xx;
    wild.sub     = &wild.sub[ xxx ];
    wild.sub_cnt = yyy - xxx;
    rm.index();
    kp.index();
  }
  /* unpack auto recognizes messages by magic numbers, etc */
  /*md_init_auto_unpack();*/

  /* read subject, size, message data */
  FILE *fp = ( fn == NULL ? stdin : fopen( fn, "rb" ) );
  if ( fp == NULL ) {
    perror( fn );
    return 1;
  }
  for (;;) {
    if ( fgets( subj, sizeof( subj ), fp ) == NULL ||
         fgets( size, sizeof( size ), fp ) == NULL )
      break;
    if ( (sz = atoi( size )) == 0 )
      break;
    if ( sz > buflen ) {
      buf = (char *) ::realloc( buf, sz );
      buflen = sz;
    }
    /* strip newline on subject */
    size_t slen = ::strlen( subj );
    while ( slen > 0 && subj[ slen - 1 ] < ' ' )
      subj[ --slen ] = '\0';
    for ( size_t n = 0; n < sz; ) {
      size_t i = fread( &buf[ n ], 1, sz - n, fp ); /* message data */
      if ( i == 0 ) {
        if ( feof( fp ) )
          fprintf( stderr, "eof, truncated msg (%s)\n", subj );
        else
          perror( fn ? fn : "stdin" );
        err_cnt++;
        sz = 0;
      }
      n += i;
    }
    if ( sz == 0 )
      break;
    if ( wild.sub_cnt != 0 && ! wild.match( subj ) )
      continue;
    /* skip msgs filtered by argv[ 2 ]
    if ( 2 < argc && ::strstr( subj, argv[ 2 ] ) == NULL )
      continue; */
    /* try to unpack it */
    MDMsg * m = MDMsg::unpack( buf, 0, sz, 0, dict, mem );
    void * msg = NULL;
    size_t msg_sz = 0;
    int fldcnt = 0;
    if ( m == NULL )
      err_cnt++;
    /* print it */
    else if ( ! quiet ) {
      mout.printf( "\n## %s (fmt %s)\n", subj, m->get_proto_string() );
      /*m->print( &mout );*/
    }
    if ( rm.fld_cnt != 0 || kp.fld_cnt != 0 ) {
      msg_sz = sz + 1024;
      if ( m->get_type_id() == TIBMSG_TYPE_ID ) {
        TibMsgWriter w( msg = mem.make( msg_sz ), msg_sz );
        fldcnt = filter<TibMsgWriter>( w, m, rm, kp, msg_sz );
      }
      else if ( m->get_type_id() == RVMSG_TYPE_ID ) {
        RvMsgWriter w( msg = mem.make( msg_sz ), msg_sz );
        fldcnt = filter<RvMsgWriter>( w, m, rm, kp, msg_sz );
      }
      else if ( m->get_type_id() == TIB_SASS_TYPE_ID ) {
        TibSassMsgWriter w( dict, msg = mem.make( msg_sz ), msg_sz );
        fldcnt = filter<TibSassMsgWriter>( w, m, rm, kp, msg_sz );
      }
      else {
        msg = NULL;
        msg_sz = 0;
        fldcnt = 0;
      }
      if ( msg_sz == 0 ) {
        err_cnt++;
      }
      else if ( ! quiet ) {
        MDMsg * m2 = MDMsg::unpack( msg, 0, msg_sz, 0, dict, mem );
        m2->print( &mout );
      }
    }
    else if ( ! quiet ) {
      m->print( &mout );
    }
    /*cmp_msg( mout, m, dict, msg, msg_sz );*/
    if ( binout ) {
      if ( fldcnt > 0 ) {
        bout.printf( "%s\n%" PRIu64 "\n", subj, msg_sz );
        if ( bout.write( msg, msg_sz ) != msg_sz )
          err_cnt++;
      }
      else {
        discard_cnt++;
      }
    }
    msg_count++;
    mem.reuse(); /* reset mem for next msg */
  }
  /*if ( ! quiet )*/ {
    fprintf( stderr, "found %" PRIu64 " messages, %" PRIu64 " erorrs\n",
             msg_count, err_cnt );
    if ( discard_cnt > 0 )
      fprintf( stderr, "discarded %" PRIu64 "\n", discard_cnt );
  }
  if ( fn != NULL && fp != NULL )
    fclose( fp );
  if ( bout.close() != 0 ) {
    perror( out );
    err_cnt++;
  }
  return err_cnt == 0 ? 0 : 2;
}

