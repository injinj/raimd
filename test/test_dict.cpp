#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/enum_def.h>

using namespace rai;
using namespace md;

static void test_lookup( MDDictBuild &dict_build,  MDDict *dict ) noexcept;
static void gen_sass_fields( MDDictBuild &dict_build,  MDDict *dict ) noexcept;
static void gen_rdm_fields( MDDict *dict ) noexcept;
static void gen_enum_defs( MDDict *dict ) noexcept;

static int
get_arg( const char *arg,  int argc,  char **argv ) noexcept
{
  for ( int i = 1; i < argc; i++ )
    if ( ::strcmp( argv[ i ], arg ) == 0 )
      return i;
  return -1;
}

int
main( int argc, char **argv )
{
  MDDictBuild dict_build;
  MDDict * dict = NULL;
  const char * path = ::getenv( "cfile_path" );
  bool gen_fields   = ( get_arg( "-g", argc, argv ) > 0 );
  bool gen_app_a    = ( get_arg( "-a", argc, argv ) > 0 );
  bool gen_enumdefs = ( get_arg( "-e", argc, argv ) > 0 );
  int  path_arg     = get_arg( "-p", argc, argv );

  if ( get_arg( "-h", argc, argv ) > 0 ) {
    fprintf( stderr,
      "Usage: %s [-g] [-p cfile_path]\n"
      "Test loading dictionaries or generate SASS Qform fields\n"
      "Use -g to generate tss_fields.cf from RDMFieldDictionary\n"
      "Use -a to generate RDMFieldDictionary\n"
      "Set $cfile_path to the path that has dictionary files\n"
      "or pass path to the -p arg\n", argv[ 0 ] );
    return 1;
  }
  if ( path_arg > 0 && path_arg + 1 < argc )
    path = argv[ path_arg + 1 ];
  /* load RDM dictionary */
  dict_build.debug_flags = MD_DICT_PRINT_FILES;
  if ( AppA::parse_path( dict_build, path, "RDMFieldDictionary" ) == 0 ) {
    EnumDef::parse_path( dict_build, path, "enumtype.def" );
    dict_build.index_dict( "app_a", dict );
    if ( ! gen_fields && ! gen_app_a && ! gen_enumdefs )
      test_lookup( dict_build, dict );
  }
  /* if generating, don't load cfiles */
  if ( gen_fields ) {
    if ( dict == NULL )
      fprintf( stderr, "No RDMFieldDictionary loaded\n" );
    else
      gen_sass_fields( dict_build, dict );
  }
  if ( gen_app_a ) {
    if ( dict == NULL )
      fprintf( stderr, "No RDMFieldDictionary loaded\n" );
    else
      gen_rdm_fields( dict );
  }
  if ( gen_enumdefs ) {
    if ( dict == NULL )
      fprintf( stderr, "No RDMFieldDictionary loaded\n" );
    else
      gen_enum_defs( dict );
  }
  /* load Tib cfiles */
  if ( ! gen_fields && ! gen_app_a && ! gen_enumdefs ) {
    dict_build.clear_build();
    if ( CFile::parse_path( dict_build, path, "tss_fields.cf" ) == 0 ) {
      CFile::parse_path( dict_build, path, "tss_records.cf" );
      dict_build.index_dict( "cfile", dict );
      test_lookup( dict_build, dict );
    }
    MDDict * str_dict = NULL;
    dict_build.clear_build();
const char *str =
"test        { CLASS_ID    0; DATA_SIZE 4; DATA_TYPE 20; }\n"
"session     { CLASS_ID    1; DATA_SIZE 128; IS_FIXED false; DATA_TYPE 2; }\n"
"digest      { CLASS_ID    2; DATA_SIZE 16; DATA_TYPE 11; }\n"
"auth_digest { CLASS_ID    3; DATA_SIZE 16; DATA_TYPE 11; }\n"
"cnonce      { CLASS_ID    4; DATA_SIZE 16; DATA_TYPE 11; }\n"

"seqno       { CLASS_ID    5; DATA_SIZE 4; DATA_TYPE 20; }\n"
"seqno_8     { CLASS_ID  261; DATA_SIZE 8; DATA_TYPE 21; }\n"

"sub_seqno   { CLASS_ID    6; DATA_SIZE 4; DATA_TYPE 20; }\n"
"sub_seqno_8 { CLASS_ID  262; DATA_SIZE 8; DATA_TYPE 21; }\n"

"time        { CLASS_ID    7; DATA_SIZE 4; DATA_TYPE 20; }\n"
"time_8      { CLASS_ID  263; DATA_SIZE 8; DATA_TYPE 21; }\n"

"interval    { CLASS_ID    8; DATA_SIZE 2; DATA_TYPE 19; }\n"
"interval_4  { CLASS_ID  264; DATA_SIZE 4; DATA_TYPE 20; }\n"

"type        { CLASS_ID    9; DATA_SIZE 2; DATA_TYPE 19; }\n"
"type_4      { CLASS_ID  265; DATA_SIZE 4; DATA_TYPE 20; }\n"

"status      { CLASS_ID   10; DATA_SIZE 2; DATA_TYPE 19; }\n"
"status_4    { CLASS_ID  266; DATA_SIZE 4; DATA_TYPE 20; }\n"

"ret         { CLASS_ID   11; DATA_SIZE 2; DATA_TYPE 19; }\n"
"ret_4       { CLASS_ID  267; DATA_SIZE 4; DATA_TYPE 20; }\n";

    CFile::parse_string( dict_build, str, ::strlen( str ) );
    dict_build.index_dict( "cfile", str_dict );
    test_lookup( dict_build, str_dict );
  }

  return 0;
}

static void
test_lookup( MDDictBuild &dict_build,  MDDict *dict ) noexcept
{
  uint32_t fidcnt;
  MDFid fid;
#if 0
  for ( uint16_t fid = 0; fid < 0x4000; fid++ ) {
    if ( dict->lookup( fid, ftype, fsize, fnamelen, fname ) ) {
      printf( "%u: fname %s ftype %u fsize %u\n", fid, fname, ftype, fsize );
    }
  }
#endif
  if ( dict == NULL ) {
    fprintf( stderr, "no dict loaded\n" );
    return;
  }
  printf( "\n" );
  printf( "dict %s\n", dict->dict_type ); 
  printf( "build size %" PRIu64 "\n", dict_build.idx->total_size() );
  printf( "fname %u\n", dict->ht_off - dict->fname_off );
  printf( "shift %u align %u\n", dict->fname_shft, dict->fname_algn );
  printf( "entry count %" PRIu64 "\n", dict_build.idx->entry_count );
  printf( "fid min %u max %u\n", dict_build.idx->min_fid,
                                 dict_build.idx->max_fid );
  printf( "type count %u/%u\n", dict_build.idx->type_hash->htcnt,
                             dict_build.idx->type_hash->htsize() );
  printf( "index size %u\n", dict->dict_size );
  const char * val;
  size_t val_sz;
  if ( dict->find_tag( "Version", val, val_sz ) )
    printf( "version \"%.*s\"\n", (int) val_sz, val );
  if ( dict->find_tag( "DictionaryId", val, val_sz ) )
    printf( "dictionary_id \"%.*s\"\n", (int) val_sz, val );
  if ( dict->find_tag( "DT_Version", val, val_sz ) )
    printf( "dt_version \"%.*s\"\n", (int) val_sz, val );
  if ( dict->find_tag( "RT_Version", val, val_sz ) )
    printf( "rt_version \"%.*s\"\n", (int) val_sz, val );
#if 0
  const char * tag;
  size_t tag_sz;
  for ( bool b = dict->first_tag( tag, tag_sz ); b;
       b = dict->next_tag( tag, tag_sz ) ) {
    printf( "tag %.*s\n", (int) tag_sz, tag );
  }
#endif
  for ( MDDictEntry *fp = dict_build.idx->entry_q.hd; fp != NULL;
        fp = fp->next ) {
    MDLookup by( fp->fid );
    if ( ! dict->lookup( by ) ) {
      printf( "fid %u not found\n", fp->fid );
      break;
    }
    if ( fp->ftype != (MDType) by.ftype || fp->fsize != by.fsize ||
         fp->fld_flags != by.flags || fp->fnamelen != by.fname_len ||
         ::memcmp( fp->fname(), by.fname, by.fname_len ) != 0 ) {
      printf( "fid %u not equal\n", fp->fid );
      printf( "ftype %u fsize %u fnamelen %u\n",
              by.ftype, by.fsize, by.fname_len );
      printf( "ftype %u fsize %u fnamelen %u\n",
              fp->ftype, fp->fsize, fp->fnamelen );
      break;
    }
    if ( fp->ftype == MD_ENUM ) {
      for ( uint16_t val = 0; val <= 4000; val++ ) {
        const char *disp;
        size_t disp_len;
        dict->get_enum_map_text( fp->fsize, val, disp, disp_len );
        /*if ( dict->get_enum_map_text( fp->fsize, val, disp, disp_len ) ) {
          printf( "  enum[%u] = %.*s\n", val, (int) disp_len, disp );
        }*/
      }
    }
  }
  fidcnt = 0;
  for ( fid = dict_build.idx->min_fid;
        fid <= dict_build.idx->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( dict->lookup( by ) ) {
      MDLookup nm( by.fname, by.fname_len );
      if ( dict->get( nm ) ) {
        if ( by.fid == nm.fid && by.ftype == nm.ftype &&
             by.fsize == nm.fsize && by.flags == nm.flags )
          fidcnt++;
        else
          printf( "fid mismatch: fname: \"%.*s\" "
                  "fid: %d == %d ftype: %u == %u fsize: %u == %u\n",
                  (int) by.fname_len, by.fname,
                  (int) by.fid, (int) nm.fid,
                  (uint32_t) by.ftype, (uint32_t) nm.ftype,
                  (uint32_t) by.fsize, (uint32_t) nm.fsize );
      }
      else
        printf( "fname not found\n" );
    }
  }
  printf( "total fids: %u\n", fidcnt );

  for ( MDDictEntry *fp = dict_build.idx->entry_q.hd; fp != NULL;
        fp = fp->next ) {
    MDLookup by( fp->fname(), fp->fnamelen );
    if ( ! dict->get( by ) ) {
      printf( "not found: %s\n", by.fname );
    }
    else if ( fp->fid != by.fid || fp->ftype != by.ftype ||
              fp->fsize != by.fsize || fp->fld_flags != by.flags ) {
      if ( fp->fno != 0 ) { /* redecleared */
        printf( "%s: fid %u/%u ftype %u/%u fsize %u/%u\n", fp->fname(),
                by.fid, fp->fid, by.ftype, fp->ftype, by.fsize, fp->fsize );
      }
    }
  }
}

static const char *FIELDS_TYPE_HEADER =
"#   This is the type mapping used for converting fields from\n"
"# RDMFieldDictionary, except when the field exists from merging an existing\n"
"# cfiles dictionary, then the previous size, type and fid are used.\n"
"#\n"
"# DATA_TYPE            DATA_SIZE                   Derived From\n"
"# ---------            ---------                   ------------\n"
"#  1   INTEGER         SIZE 4                      RDM/INTEGER/UINT32\n"
"#    example: PROD_CATG, RDNDISPLAY, REF_COUNT\n"
"#\n"
"#  2   STRING          LENGTH + 1, roundup even    RDM/ALPHANUMERIC\n"
"#    example: DSPLY_NAME, NEWS, OFFCL_CODE\n"
"#\n"
"#  2   STRING          LENGTH>?(ENUMLEN) + 1,even  RDM/ENUMERATED\n"
"#    example: DSPLY_NAME, NEWS, OFFCL_CODE\n"
"#\n"
"#  2   STRING/PARTIAL  LENGTH * 2 + 2              RDM/ROWxx_y\n"
"#    example: ROW64_1, ROW80_1, ROW99_1, ROW66_1, ROW74_1\n"
"#\n"
"#  9   SHORT_INT       SIZE 2                      SASS specific codes\n"
"#    example: MSG_TYPE, REC_TYPE, SEQ_NO, REC_STATUS\n"
"#\n"
"#  14  DOUBLE_INT      SIZE 8                      RDM/INTEGER/UINT64|REAL64\n"
"#    example: BIDSIZE, ASKSIZE, LOT_SIZE, NUM_MOVES, TOT_VOLUME, TRDTIM_MS\n"
"#\n"
"#  15  GROCERY         SIZE 9                      RDM/PRICE\n"
"#    example: TRDPRC_1, NETCHNG_1, HIGH_1, LOW_1, BID, ASK\n"
"#\n"
"#  16  SDATE           SIZE 12                     RDM/DATE\n"
"#    example: TRADE_DATE, ACTIV_DATE\n"
"#\n"
"#  17  STIME           SIZE 6                      RDM/TIME\n"
"#    example: TIMACT, TRDTIM_1, NEWS_TIME\n"
"#\n"
"#  17  STIME           SIZE 10                     RDM/TIME_SECONDS\n"
"#    example: SALTIM, QUOTIM\n"
"#\n";

static const char *SYS_TSS_FIELDS =
"# SASS CONTROL fields\n"
"MSG_TYPE\n{\n"
"\tCLASS_ID\t4001;\n"
"\tIS_PRIMITIVE\ttrue;\n"
"\tIS_FIXED\ttrue;\n"
"\tDATA_SIZE\t2;\n"
"\tDATA_TYPE\t9;\n"
"}\n\n"
"REC_TYPE\n{\n"
"\tCLASS_ID\t4002;\n"
"\tIS_PRIMITIVE\ttrue;\n"
"\tIS_FIXED\ttrue;\n"
"\tDATA_SIZE\t2;\n"
"\tDATA_TYPE\t9;\n"
"}\n\n"
"SEQ_NO\n{\n"
"\tCLASS_ID\t4003;\n"
"\tIS_PRIMITIVE\ttrue;\n"
"\tIS_FIXED\ttrue;\n"
"\tDATA_SIZE\t2;\n"
"\tDATA_TYPE\t9;\n"
"}\n\n"
"REC_STATUS\n{\n"
"\tCLASS_ID\t4005;\n"
"\tIS_PRIMITIVE\ttrue;\n"
"\tIS_FIXED\ttrue;\n"
"\tDATA_SIZE\t2;\n"
"\tDATA_TYPE\t9;\n"
"}\n";
#if 0
static const char *SYS_TSS_FORMS =
"# SASS CONTROL form\n"
"CONTROL\n{\n"
"    CLASS_ID\t\t4081;\n"
"    IS_PRIMITIVE\tfalse;\n"
"    IS_FIXED\t\ttrue;\n"
"\n"
"    FIELDS\n"
"    {\n"
"\t\"MSG_TYPE\"\t{ FIELD_CLASS_NAME  \"MSG_TYPE\";\t\t}\n"
"\t\"REC_TYPE\"\t{ FIELD_CLASS_NAME  \"REC_TYPE\";\t\t}\n"
"\t\"SEQ_NO\"\t{ FIELD_CLASS_NAME  \"SEQ_NO\";\t\t}\n"
"\t\"REC_STATUS\"\t{ FIELD_CLASS_NAME  \"REC_STATUS\";\t}\n"
"    }\n"
"}\n";
#endif
template<size_t NBITS>
struct BitSet {
  uint64_t bits[ ( NBITS + 63 ) / 64 ];

  void zero( void ) {
    ::memset( this->bits, 0, sizeof( bits ) );
  }
  void set( size_t j ) {
    size_t   off  = j >> 6;
    uint64_t mask = (uint64_t) 1 << ( j & 63 );
    this->bits[ off ] |= mask;
  }
  void unset( size_t j ) {
    size_t   off  = j >> 6;
    uint64_t mask = (uint64_t) 1 << ( j & 63 );
    this->bits[ off ] &= ~mask;
  }
  bool isset( size_t j ) {
    size_t   off  = j >> 6;
    uint64_t mask = (uint64_t) 1 << ( j & 63 );
    return ( this->bits[ off ] & mask ) != 0;
  }
  bool testset( size_t j ) {
    size_t   off  = j >> 6;
    uint64_t mask = (uint64_t) 1 << ( j & 63 );
    if ( ( this->bits[ off ] & mask ) != 0 )
      return true;
    this->bits[ off ] |= mask;
    return false;
  }
};

static uint32_t
hash_fid( MDFid fid ) noexcept
{
  uint32_t a = (uint32_t) fid;
  a -= (a<<6);
  a ^= (a>>17);
  a -= (a<<9);
  a ^= (a<<4);
  a -= (a<<3);
  a ^= (a<<10);
  a ^= (a>>15);
  return a;
}

static const MDFid MSG_TYPE_FID   = 4001,
                   REC_TYPE_FID   = 4002,
                   SEQ_NO_FID     = 4003,
                   REC_STATUS_FID = 4005,
                   CONTROL_FID    = 4081,
                   PROD_PERM_FID  = 1;

static const char *
rdm_type_str( const MDDictEntry &entry,  char *buf ) noexcept
{
  char *p = buf;
  ::strcpy( buf, "MF=" ); buf = &buf[ 3 ];
  ::strcpy( buf, mf_type_str( (MF_type) entry.mf_type ) );
  buf = &buf[ ::strlen( buf ) ];
  ::strcpy( buf, ", RWF=" ); buf = &buf[ 6 ];
  ::strcpy( buf, rwf_type_str( (RWF_type) entry.rwf_type ) );
  buf = &buf[ ::strlen( buf ) ];
  ::strcpy( buf, ", MD=" ); buf = &buf[ 5 ];
  ::strcpy( buf, md_type_str( entry.ftype ) );
#if 0
  if ( entry->rwftype != RWF_NONE ) {
    ::strcat( buf, "/" );
    ::strcat( buf, RaiMfeed_dict::getRWFTypeString(
                      (RaiRWF_type) entry->rwftype, entry->rwfbits ) );
  }
#endif
  return p;
}

static bool
isreserved( const char *fname ) noexcept
{
  switch ( fname[ 0 ] ) {
    case 'M': return ::strcmp( fname, "MSG_TYPE" ) == 0;
    case 'R': return ( ::strcmp( fname, "REC_STATUS" ) == 0 ||
                       ::strcmp( fname, "REC_TYPE" ) == 0 );
    case 'S': return ::strcmp( fname, "SEQ_NO" ) == 0;
    default: return false;
  }
}

struct SassCounts {
  uint32_t num_fields, new_fields, existing_fields, skipped,
           reserved;
  SassCounts() : num_fields( 0 ), new_fields( 0 ),
                 existing_fields( 0 ), skipped( 0 ), reserved( 0 ) {}
};

static bool gen_sass_entry( MDFid new_fid,  const MDDictEntry &mentry,
                            SassCounts &cnt,  bool is_collision ) noexcept;

static void
gen_sass_fields( MDDictBuild &dict_build,  MDDict * ) noexcept
{
  static const uint16_t MAX_SASS_FID = 16 * 1024 - 1;
  static const MDFid base_fid = 10700 - 1; /* + fid(non-zero) >= 10700 */
  BitSet<MAX_SASS_FID + 1> used;
  const MDDictEntry * mentry = NULL, ** collisions = NULL;
  uint32_t ht[ MAX_SASS_FID + 1 ], ht_size = 0, num_collisions = 0, i;
  SassCounts cnt;
  MDFid new_fid;
  char tbuf[ 64 ];

  printf( "# Generated tss_fields.cf\n\n" );
  fputs( FIELDS_TYPE_HEADER, stdout );
  fputs( "{\n", stdout );
  fputs( SYS_TSS_FIELDS, stdout );
  used.zero();
  used.set( 0 ); /* prevent zero fid */
  used.set( MSG_TYPE_FID );   /* MSG_TYPE(4001) */
  used.set( REC_TYPE_FID );   /* REC_TYPE(4002) */
  used.set( SEQ_NO_FID );     /* SEQ_NO(4003) */
  used.set( REC_STATUS_FID ); /* REC_STATUS(4005) */
  used.set( CONTROL_FID );    /* CONTROL(4081) formclass */

  for ( mentry = dict_build.idx->entry_q.hd; mentry != NULL;
        mentry = mentry->next ) {
    if ( isreserved( mentry->fname() ) ) {
      printf( "\n# %s(%d) reserved, %s\n",
               mentry->fname(), mentry->fid, rdm_type_str( *mentry, tbuf ) );
      cnt.reserved++;
      continue;
    }
    if ( mentry->fid < 0 )
      new_fid = -mentry->fid;
    else
      new_fid = base_fid + mentry->fid;
    new_fid = new_fid & 0x3fff; /* wrap around 0 */
    /* if fid already used, save it for second pass */
    if ( used.testset( new_fid ) ) {
      if ( ( num_collisions & MAX_SASS_FID ) == 0 ) {
        void * p;
        p = realloc( collisions, sizeof( collisions[ 0 ] ) *
                     ( num_collisions + MAX_SASS_FID + 1 ) );
        collisions = (const MDDictEntry **) p;
      }
      collisions[ num_collisions++ ] = mentry;
      continue;
    }
    if ( ! gen_sass_entry( new_fid, *mentry, cnt, false ) )
      used.unset( new_fid );
  }

  /* create a table of available fids */
  for ( i = 0; i < MAX_SASS_FID; i++ )
    if ( ! used.isset( i ) )
      ht[ ht_size++ ] = i;

  for ( i = 0; i < num_collisions; i++ ) {
    mentry = collisions[ i ];
    uint32_t tries, off = hash_fid( mentry->fid ) % ht_size;
    /* find an unused fid */
    for ( tries = 0; tries < ht_size; tries++ ) {
      if ( ! used.testset( ht[ off ] ) ) {
        new_fid = ht[ off ];
        break;
      }
      off = ( off + 1 ) % ht_size;
    }
    if ( tries == ht_size ) {
      fprintf( stderr, "Too many fids, stopped with %u fids unmapped",
               num_collisions - i + 1 );
      break;
    }
    if ( ! gen_sass_entry( new_fid, *mentry, cnt, true ) )
      used.unset( new_fid );
  }

  fputs( "\n}\n", stdout );
#if 0
  if ( recOut != NULL ) {
    recOut->printf( "# Rai generated tss_records.cf %s\n\n",
                    Time::timestamp( tbuf, sizeof( tbuf ) ) );
    recOut->puts( "{\n" );
    recOut->puts( SYS_TSS_FORMS );
    recOut->puts( "\n}\n" );
  }
#endif
  fprintf( stderr, "Total fields generated: %u, New fields: %u, "
                   "Existing fields: %u "
                   "(%u fids available, %u skipped; %u reserved; "
                   "fid 0 not used)\n",
            cnt.num_fields, cnt.new_fields, cnt.existing_fields,
            MAX_SASS_FID + 1 - ( cnt.num_fields + 5 + 1 ), cnt.skipped,
            cnt.reserved );
  if ( num_collisions > 0 ) {
    fprintf( stderr, "Number of fid collisions: %u\n", num_collisions );
  }
}

static bool
gen_sass_entry( MDFid new_fid,  const MDDictEntry &mentry,  SassCounts &cnt,
                bool is_collision ) noexcept
{
  char tbuf[ 128 ];
  uint32_t size;
  TSS_type tp;
  bool partial = false;

  if ( mentry.fid == PROD_PERM_FID ) {
    printf( "\n# Entitlement field from PROD_PERM\n"
                 "PROD_CATG\n"
                 "{\n"
                 "\tCLASS_ID\t%d; # fid=1 (PROD_PERM)\n"
                 "\tIS_PRIMITIVE\ttrue;\n"
                 "\tIS_FIXED\ttrue;\n"
                 "\tDATA_SIZE\t4;\n"
                 "\tDATA_TYPE\t%u; # TSS=INTEGER, RDM=INTEGER/UINT64\n"
                 "}\n",
                 new_fid, TSS_INTEGER );
    cnt.num_fields++;
    return true;
  }
  fputs( "\n", stdout );
  switch ( mentry.ftype ) {
    case MD_PARTIAL:
      tp   = TSS_STRING;
      size = mentry.fsize;
      partial = true;
      break;
    case MD_STRING:
      tp   = TSS_STRING;
      size = mentry.fsize;
      break;
    case MD_TIME:
      tp   = TSS_STIME;
      size = mentry.fsize;
      break;
    case MD_DATE:
      tp   = TSS_SDATE;
      size = 12;
      break;
    case MD_ENUM:
      tp   = TSS_STRING;
      if ( mentry.mf_len > mentry.enum_len )
        size = mentry.mf_len;
      else
        size = mentry.enum_len;
      if ( ( ++size % 2 ) != 0 )
        size++;
      break;
    case MD_INT:
    case MD_UINT:
      /*if ( ( mentry.rwftype == RWF_UINT && mentry.rwfbits == 32 ) ||
           ( mentry.rwftype == RWF_NONE && mentry.flen < 4 ) ) {*/
      tp   = TSS_INTEGER;
      size = mentry.fsize;
      /*}
      else {
        tp   = TSS_DOUBLE_INT;
        size = 8;
      }*/
      break;
    case MD_DECIMAL:
      if ( mentry.mf_type == MF_INTEGER ) {
        tp   = TSS_DOUBLE_INT;
        size = 8;
      }
      else {
        tp   = TSS_GROCERY;
        size = 9;
      }
      break;
    case MD_OPAQUE:
      tp   = TSS_STRING;
      size = mentry.fsize;
      if ( size == 0 )
        size = mentry.rwf_len;
      break;
    default:
      tp   = TSS_NODATA;
      size = 0;
      break;
  }
  if ( tp != TSS_NODATA && size != 0 ) {
    cnt.new_fields++;
    /*new_str = " (new)";*/
    printf( "%s\n"
            "{\n"
            "\tCLASS_ID\t%d; # fid=%d%s\n"
            "\tIS_PRIMITIVE\ttrue;\n"
            "\tIS_FIXED\ttrue;\n",
            mentry.fname(),
            new_fid, mentry.fid,
            is_collision ? " collision" : ""/*, new_str*/ );

    if ( partial )
      fputs( "\tIS_PARTIAL\ttrue;\n", stdout );
    if ( mentry.enum_len > 0 )
      printf( "\tDATA_SIZE\t%u; # MKTFD LEN=%u(%u)",
              size, mentry.mf_len, mentry.enum_len );
    else
      printf( "\tDATA_SIZE\t%u; # MKTFD LEN=%u", size, mentry.mf_len );
    if ( mentry.rwf_len > 0 )
      printf( ", RWF LEN=%u\n", mentry.rwf_len );
    else
      fputs( "\n", stdout );
    printf( "\tDATA_TYPE\t%u; # TSS=%s, %s", (uint32_t) tp,
            tss_type_str( tp ), rdm_type_str( mentry, tbuf ) );
    fputs( "\n}\n", stdout );
    cnt.num_fields++;
    return true;
  }
  else {
    printf( "# %s(%d) skipped, %s\n",
            mentry.fname(), mentry.fid, rdm_type_str( mentry, tbuf ) );
    cnt.skipped++;
    return false;
  }
}

static void
gen_enum_defs( MDDict *dict ) noexcept
{
  for ( uint32_t mapnum = 1; mapnum < dict->map_count; mapnum++ ) {
    bool first = true;
    for ( MDFid fid = dict->min_fid; fid <= dict->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( dict->lookup( by ) && by.enummap == mapnum ) {
        if ( first ) {
printf(
"! ACRONYM    FID\n"
"! -------    ---\n"
"!\n" );

          first = false;
        }
        printf( "%-10s %5u\n", by.fname, by.fid );
      }
    }
    if ( ! first ) {
      MDEnumMap *map = dict->get_enum_map( mapnum );
      if ( map != NULL ) {
        uint16_t * vidx = map->value();
        uint8_t  * vmap = map->map();
        uint32_t   len  = map->max_len;
        first = true;
        for ( uint32_t i = 0; i < map->value_cnt; i++ ) {
          char  * disp = (char *) &vmap[ i * len ];
          uint16_t val = ( vidx == NULL ? i : vidx[ i ] );
          if ( first ) {
printf(
"!\n"
"! VALUE      DISPLAY   MEANING\n"
"! -----      -------   -------\n" );
            first = false;
          }
          uint32_t k, algn = 0;
          for ( k = 0; k < len; k++ ) {
            if ( (uint8_t) disp[ k ] > '~' )
              break;
          }
          if ( k != len ) {
            if ( len * 2 < 10 )
              algn = 10 - ( len * 2 );
            printf( "%7u %*s#", val, algn, "" );
            for ( k = 0; k < len; k++ )
              printf( "%02X", (uint8_t) disp[ k ] );
            printf( "#  %5s\n", "none" );
          }
          else {
            if ( len < 10 )
              algn = 10 - len;
            printf( "%7u %*s\"%.*s\"  %5s\n", val, algn, "",
                    len, disp, "none" );
          }
        }
      }
    }
  }
}

static void
gen_rdm_fields( MDDict *dict ) noexcept
{
  MDFid fid;

printf(
"!ACRONYM    DDE ACRONYM          FID  RIPPLES TO  FIELD TYPE     LENGTH  RWF TYPE   RWF LEN\n"
"!-------    -----------          ---  ----------  ----------     ------  --------   -------\n"
"!\n" );
  for ( fid = dict->min_fid; fid <= dict->max_fid; fid++ ) {
    MDLookup by( fid );
    uint8_t rwf_type, mf_type;
    const char * rwf_str, * mf_str;
    uint32_t mf_len, rwf_len, enum_len;

    if ( ! dict->lookup( by ) )
      continue;

    by.mf_type( mf_type, mf_len, enum_len );
    by.rwf_type( rwf_type, rwf_len );
    mf_str  = mf_type_str( (MF_type) mf_type );
    if ( rwf_type == RWF_UINT )
      rwf_str = "UINT64";
    else if ( rwf_type == RWF_INT )
      rwf_str = "INT64";
    else if ( rwf_type == RWF_REAL )
      rwf_str = "REAL64";
    else
      rwf_str = rwf_type_str( (RWF_type) rwf_type );

    const char *name, *ripple;
    uint8_t namelen, ripplelen;
    char name_buf[ 256 + 3 ];

    by.get_name_ripple( name, namelen, ripple, ripplelen );
    if ( ripple == NULL )
      ripple = "NULL";
    ::snprintf( name_buf, sizeof( name_buf ), "\"%s\"", namelen != 0 ? name : by.fname );

    if ( enum_len != 0 )
      printf( "%-10s %-20s %5d  %-11s %-13s %u ( %u )  %-13s %4u\n", by.fname,
              name_buf, by.fid, ripple, mf_str, mf_len, enum_len, rwf_str, rwf_len );
    else
      printf( "%-10s %-20s %5d  %-11s %-15s %5u  %-13s %4u\n", by.fname,
              name_buf, by.fid, ripple, mf_str, mf_len, rwf_str, rwf_len );
  }
}

