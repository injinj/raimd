#include <stdio.h>
#include <string.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/dict_load.h>
#include <raimd/md_replay.h>
#include <raimd/json_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/sass.h>
#include <raimd/mf_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_hash_tab.h>

using namespace rai;
using namespace md;

struct MsgDict {
  MDDict * dict,
         * cfile_dict,
         * rdm_dict,
         * flist_dict;
  MsgDict() : dict( 0 ), cfile_dict( 0 ), rdm_dict( 0 ), flist_dict( 0 ) {}
};

struct CacheData : public MDSubjectKey {
  uint32_t type_id;
  void   * data;
  size_t   datalen;

  void * operator new( size_t, void *ptr ) { return ptr; }
  CacheData( char *s,  size_t len )
    : MDSubjectKey( s, len ), type_id( 0 ), data( NULL ), datalen( 0 ) {}

  static CacheData * make( const MDSubjectKey &k,  MDMsg &m,  MsgDict &dict ) {
    void * d = ::malloc( sizeof( CacheData ) + k.len + 1 );
    char * p = &((char *) d)[ sizeof( CacheData ) ];
    ::memcpy( p, k.subj, k.len );
    p[ k.len ] = '\0';
    CacheData * data = new ( d ) CacheData( p, k.len );
    data->update( m, dict );
    return data;
  }
  void update( MDMsg &m,  MsgDict &dict ) {
    if ( this->datalen != 0 ) {
      MDMsg * c = MDMsg::unpack( this->data, 0, this->datalen, this->type_id,
                                 dict.dict, *m.mem );
      if ( c != NULL ) {
        this->merge( m, *c );
        return;
      }
    }
    this->type_id = m.get_type_id();
    this->datalen = m.msg_end - m.msg_off;
    this->data = ::realloc( this->data, this->datalen );
    ::memcpy( this->data, &((char *) m.msg_buf)[ m.msg_off ], this->datalen );
  }
  uint16_t get_msg_type( MsgDict &dict ) noexcept;
  uint16_t get_rec_type( MsgDict &dict ) noexcept;
  void merge( MDMsg &m,  MDMsg &c ) noexcept;
};

uint16_t
CacheData::get_msg_type( MsgDict &dict ) noexcept
{
  MDMsgMem mem;
  MDMsg * m = MDMsg::unpack( this->data, 0, this->datalen, this->type_id,
                             dict.dict, mem );
  uint16_t msg_type = UPDATE_TYPE;
  switch ( this->type_id ) {
    case MARKETFEED_TYPE_ID: {
      MktfdMsg & mf = *(MktfdMsg *) m;
      msg_type = mf_func_to_sass_msg_type( mf.func );
      break;
    }
    case RWF_MSG_TYPE_ID: {
      RwfMsg & rwf = *(RwfMsg *) m;
      msg_type = rwf_to_sass_msg_type( rwf );
      break;
    }
    default: {
      MDFieldReader rd( *m );
      if ( rd.find( SASS_MSG_TYPE, SASS_MSG_TYPE_LEN ) )
        rd.get_uint( msg_type );
      break;
    }
  }
  return msg_type;
}

uint16_t
CacheData::get_rec_type( MsgDict &dict ) noexcept
{
  MDMsgMem mem;
  MDMsg * m = MDMsg::unpack( this->data, 0, this->datalen, this->type_id,
                             dict.dict, mem );
  uint16_t rec_type = 0,
           flist    = 0;
  switch ( this->type_id ) {
    case MARKETFEED_TYPE_ID: {
      MktfdMsg & mf = *(MktfdMsg *) m;
      flist = mf.flist;
      break;
    }
    case RWF_MSG_TYPE_ID: {
      RwfMsg & rwf = *(RwfMsg *) m;
      RwfMsg * fl = rwf.get_container_msg();
      if ( fl != NULL )
        flist = fl->fields.flist;
      break;
    }
    case RWF_FIELD_LIST_TYPE_ID: {
      RwfMsg & rwf = *(RwfMsg *) m;
      flist = rwf.fields.flist;
      break;
    }
    default: {
      MDFieldReader rd( *m );
      if ( rd.find( SASS_REC_TYPE, SASS_REC_TYPE_LEN ) )
        rd.get_uint( rec_type );
      break;
    }
  }
  if ( flist != 0 && dict.flist_dict != NULL && dict.cfile_dict != NULL ) {
    MDLookup by( flist );
    if ( dict.flist_dict->lookup( by ) ) {
      MDLookup fc( by.fname, by.fname_len );
      if ( dict.cfile_dict->get( fc ) && fc.ftype == MD_MESSAGE )
        rec_type = fc.fid;
    }
  }
  return rec_type;
}

void
CacheData::merge( MDMsg &m,  MDMsg &c ) noexcept
{
  MDFieldReader delta( m ),
                cached( c );
  MDName nm;
  for ( bool b = delta.first( nm ); b; b = delta.next( nm ) ) {
    if ( cached.find( nm.fname, nm.fnamelen ) && delta.type() == cached.type() )
      cached.iter->update( delta.mref );
  }
}

typedef MDHashTabT<MDSubjectKey, CacheData> SubHT;

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
main( int argc, char *argv[] )
{
  MsgDict dict; /* dictinonaries, cfile and RDM/appendix_a */
  const char * fn     = get_arg( argc, argv, 1, "-f", NULL ),
             * path   = get_arg( argc, argv, 1, "-p", ::getenv( "cfile_path" ));

  if ( get_arg( argc, argv, 0, "-h", NULL ) != NULL ) {
    fprintf( stderr,
      "Usage: %s [-f file] [-p cfile_path] [-q]\n"
      "Test reading messages from files on command line\n"
      "  -f file           = replay file to read\n"
      "  -o file           = output file to write\n"
      "  -p cfile_path     = load dictionary files from path\n"
      "A file is a binary dump of messages, each with these lines:\n"
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
    dict.dict = load_dict_files( path );
  if ( dict.dict != NULL ) {
    for ( MDDict *d = dict.dict; d != NULL; d = d->next ) {
      if ( d->dict_type[ 0 ] == 'c' )
        dict.cfile_dict = d;
      else if ( d->dict_type[ 0 ] == 'a' )
        dict.rdm_dict = d;
      else if ( d->dict_type[ 0 ] == 'f' )
        dict.flist_dict = d;
    }
  }
  FILE *filep = ( fn == NULL ? stdin : fopen( fn, "rb" ) );
  if ( filep == NULL ) {
    perror( fn );
    return 1;
  }
  MDReplay replay( filep );
  SubHT sub_ht( 128 );
  MDOutput mout;
  size_t count = 0;
  for ( bool b = replay.first(); b; b = replay.next() ) {
    MDMsgMem mem;
    MDMsg * m =
      MDMsg::unpack( replay.msgbuf, 0, replay.msglen, 0, dict.dict, mem );

    if ( m != NULL ) {
      MDSubjectKey k( replay.subj, replay.subjlen );
      CacheData * data;
      size_t pos;
      if ( (data = sub_ht.find( k, pos )) == NULL ) {
        data = CacheData::make( k, *m, dict );
        sub_ht.insert( pos, data );
      }
      else {
        data->update( *m, dict );
      }
      printf( "\n## %.*s (fmt %s) mt=%u rt=%u\n",
              (int) replay.subjlen, replay.subj,
              m->get_proto_string(), data->get_msg_type( dict ),
              data->get_rec_type( dict ) );
      m->print( &mout );
      count++;
    }
  }
  printf( "%ld messages\n", (long) count );

  return 0;
}
