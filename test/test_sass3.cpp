/* round trip the sass3 packed dictionary: load dict files, pack_sass3,
 * unpack_sass3, dump both dict lists and compare, then repack and compare
 * the bytes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/app_a.h>
#include <raimd/dict_load.h>
#include <raimd/tib_msg.h>

using namespace rai;
using namespace md;

static void
dump_dict( MDDict *d,  FILE *fp ) noexcept
{
  fprintf( fp, "dict %s min_fid %d max_fid %d entries %u maps %u forms %u\n",
           d->dict_type, d->min_fid, d->max_fid, d->entry_count,
           d->map_count, d->form_count );
  for ( MDFid fid = d->min_fid; fid <= d->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( ! d->lookup( by ) )
      continue;
    const char * name, * ripple;
    uint8_t namelen, ripplelen, mft, rwft;
    uint32_t mfl, enuml, rwfl;
    by.get_name_ripple( name, namelen, ripple, ripplelen );
    by.mf_type( mft, mfl, enuml );
    by.rwf_type( rwft, rwfl );
    fprintf( fp, "%d %.*s type %u size %u flags %x map %u", fid,
             (int) by.fname_len, by.fname, by.ftype, by.fsize, by.flags,
             by.map_num );
    if ( namelen != 0 )
      fprintf( fp, " name %.*s", (int) namelen, name );
    if ( ripplelen != 0 )
      fprintf( fp, " ripple %.*s", (int) ripplelen, ripple );
    if ( d->dict_type[ 0 ] == 'a' )
      fprintf( fp, " mf %u/%u/%u rwf %u/%u", mft, mfl, enuml, rwft, rwfl );
    /* name lookup must find the same fid */
    MDLookup nm( by.fname, by.fname_len );
    if ( ! d->get( nm ) || nm.fid != fid )
      fprintf( fp, " NAME-LOOKUP-FAIL" );
    if ( by.ftype == MD_MESSAGE && by.map_num != 0 &&
         d->dict_type[ 0 ] == 'c' ) {
      MDFormClass * fc = d->get_form_class( fid );
      if ( fc == NULL )
        fprintf( fp, " form NULL" );
      else {
        fprintf( fp, " form size %u fields %u:", fc->form_size, fc->nentries );
        for ( uint32_t i = 0; i < fc->nentries; i++ )
          fprintf( fp, " %u@%u", fc->entries[ i ].fid, fc->entries[ i ].foffset );
      }
    }
    fprintf( fp, "\n" );
  }
  for ( uint32_t m = 1; m < d->map_count; m++ ) {
    MDEnumMap * map = d->get_enum_map( m );
    if ( map == NULL )
      continue;
    fprintf( fp, "enum %u cnt %u max %u len %u:", m, map->value_cnt,
             map->max_value, map->max_len );
    for ( uint32_t v = 0; v <= map->max_value; v++ ) {
      const char * disp; size_t disp_len;
      if ( d->get_enum_map_text( m, (uint16_t) v, disp, disp_len ) )
        fprintf( fp, " %u=\"%.*s\"", v, (int) disp_len, disp );
    }
    fprintf( fp, "\n" );
  }
}

static void
dump_list( MDDict *d,  const char *fn ) noexcept
{
  FILE * fp = fopen( fn, "w" );
  for ( ; d != NULL; d = d->get_next() )
    dump_dict( d, fp );
  fclose( fp );
}

static size_t
pack( MDDict *d,  MDMsgMem &mem,  void *&buf ) noexcept
{
  size_t sz = 4 * 1024 * 1024;
  buf = mem.make( sz );
  TibMsgWriter w( mem, buf, sz );
  int status = CFile::pack_sass3( d, w );
  if ( status != 0 ) {
    fprintf( stderr, "pack_sass3 failed %d\n", status );
    exit( 2 );
  }
  return w.off + w.hdrlen;
}

int
main( int argc,  char **argv )
{
  const char * path = ( argc > 1 ? argv[ 1 ] : ::getenv( "cfile_path" ) );
  const char * out  = ( argc > 2 ? argv[ 2 ] : "/tmp/sass3" );
  if ( path == NULL ) {
    fprintf( stderr, "usage: %s cfile_path [out_prefix]\n", argv[ 0 ] );
    return 1;
  }
  MDDict * d1 = load_dict_files( path, true );
  if ( d1 == NULL )
    return 2;
  MDMsgMem mem;
  void   * buf1, * buf2;
  size_t   len1 = pack( d1, mem, buf1 );
  fprintf( stderr, "packed %u bytes\n", (uint32_t) len1 );

  TibMsg::init_auto_unpack();
  MDMsgMem mem2;
  MDMsg * m = MDMsg::unpack( buf1, 0, len1, 0, NULL, mem2 );
  if ( m == NULL ) {
    fprintf( stderr, "unpack msg failed\n" );
    return 3;
  }
  MDDictBuild build;
  MDDict * d2 = NULL;
  int status = CFile::unpack_sass3( build, m, d2 );
  if ( status != 0 || d2 == NULL ) {
    fprintf( stderr, "unpack_sass3 failed %d\n", status );
    return 4;
  }
  char fn1[ 256 ], fn2[ 256 ];
  snprintf( fn1, sizeof( fn1 ), "%s.1.txt", out );
  snprintf( fn2, sizeof( fn2 ), "%s.2.txt", out );
  dump_list( d1, fn1 );
  dump_list( d2, fn2 );
  size_t len2 = pack( d2, mem, buf2 );
  int rc = 0;
  if ( len1 != len2 || ::memcmp( buf1, buf2, len1 ) != 0 ) {
    fprintf( stderr, "repack differs: %u vs %u bytes\n", (uint32_t) len1,
             (uint32_t) len2 );
    rc = 5;
  }
  else
    fprintf( stderr, "repack identical (%u bytes)\n", (uint32_t) len2 );
  fprintf( stderr, "dumps: %s %s\n", fn1, fn2 );
  return rc;
}

