#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <raimd/json_msg.h>
#include <raimd/md_geo.h>
#include <raimd/md_hash.h>
#include <raimd/md_hll.h>
#include <raimd/md_list.h>
#include <raimd/md_set.h>
#include <raimd/md_zset.h>
#include <raimd/mf_msg.h>
#include <raimd/rv_msg.h>
#include <raimd/rwf_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/tib_sass_msg.h>

using namespace rai;
using namespace md;

static bool is_basic( void *,  size_t,  size_t,  uint32_t ) { return false; }
static MDMsg * unpack_basic( void *,  size_t,  size_t,  uint32_t,  MDDict *,
                             MDMsgMem * ) { return NULL; }

struct BasicMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }
  MDType type;
  BasicMsg( void *bb,  size_t off,  size_t end,  MDDict *d,
            MDMsgMem *m,  MDType t )
    : MDMsg( bb, off, end, d, m ), type( t ) {}
  virtual int get_reference( MDReference &mref ) final {
    mref.fptr     = &((uint8_t *) this->msg_buf)[ this->msg_off ];
    mref.fsize    = this->msg_end - this->msg_off;
    mref.ftype    = type;
    mref.fendian  = MD_LITTLE;
    mref.fentrytp = MD_NODATA;
    mref.fentrysz = 0;
    return 0;
  }
  virtual const char *get_proto_string( void ) final {
    return md_type_str( this->type, this->msg_end - this->msg_off );
  }
  virtual uint32_t get_type_id( void ) final {
    return this->type;
  }
};

static MDMsg *
make_basic_msg( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                MDMsgMem *m,  MDType type )
{
  void *p = NULL;
  m->alloc( sizeof( BasicMsg ), &p );
  if ( p == NULL )
    return NULL;
  return new ( p ) BasicMsg( bb, off, end, d, m, type );
}

static bool
is_string( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_string( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
               MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_STRING );
}

static bool
is_opaque( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_opaque( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
               MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_OPAQUE );
}

static bool
is_boolean( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_boolean( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_BOOLEAN );
}

static bool
is_int( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_int( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
            MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_INT );
}

static bool
is_uint( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_uint( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
             MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_UINT );
}

static bool
is_real( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_real( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
             MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_REAL );
}

static bool
is_ipdata( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_ipdata( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
               MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_IPDATA );
}

static bool
is_subject( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_subject( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_SUBJECT );
}

static bool
is_enum( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_enum( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
             MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_ENUM );
}

static bool
is_time( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_time( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
             MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_TIME );
}

static bool
is_date( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_date( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
             MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_DATE );
}

static bool
is_decimal( void *,  size_t,  size_t,  uint32_t ) { return true; }
static MDMsg *
unpack_decimal( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                MDMsgMem *m ) {
  return make_basic_msg( bb, off, end, h, d, m, MD_DECIMAL );
}

static MDMatch basic_match[] = {
  { 0,0,0, MD_MESSAGE, { 0 }, { 0 }, is_basic, unpack_basic },
  { 0,0,0, MD_STRING, { 0 }, { 0 }, is_string, unpack_string },
  { 0,0,0, MD_OPAQUE, { 0 }, { 0 }, is_opaque, unpack_opaque },
  { 0,0,0, MD_BOOLEAN, { 0 }, { 0 }, is_boolean, unpack_boolean },
  { 0,0,0, MD_INT, { 0 }, { 0 }, is_int, unpack_int },
  { 0,0,0, MD_UINT, { 0 }, { 0 }, is_uint, unpack_uint },
  { 0,0,0, MD_REAL, { 0 }, { 0 }, is_real, unpack_real },
  { 0,0,0, MD_ARRAY, { 0 }, { 0 }, is_basic, unpack_basic },
  { 0,0,0, MD_PARTIAL, { 0 }, { 0 }, is_basic, unpack_basic },
  { 0,0,0, MD_IPDATA, { 0 }, { 0 }, is_ipdata, unpack_ipdata },
  { 0,0,0, MD_SUBJECT, { 0 }, { 0 }, is_subject, unpack_subject },
  { 0,0,0, MD_ENUM, { 0 }, { 0 }, is_enum, unpack_enum },
  { 0,0,0, MD_TIME, { 0 }, { 0 }, is_time, unpack_time },
  { 0,0,0, MD_DATE, { 0 }, { 0 }, is_date, unpack_date },
  { 0,0,0, MD_DATETIME, { 0 }, { 0 }, is_basic, unpack_basic },
  { 0,0,0, MD_STAMP, { 0 }, { 0 }, is_basic, unpack_basic },
  { 0,0,0, MD_DECIMAL, { 0 }, { 0 }, is_decimal, unpack_decimal }
};

extern "C"
void
md_init_auto_unpack( void )
{
  static int auto_unpack;
  if ( ! auto_unpack ) {
    auto_unpack = 1;
    JsonMsg::init_auto_unpack();
    GeoMsg::init_auto_unpack();
    HashMsg::init_auto_unpack();
    HLLMsg::init_auto_unpack();
    ListMsg::init_auto_unpack();
    SetMsg::init_auto_unpack();
    ZSetMsg::init_auto_unpack();
    MktfdMsg::init_auto_unpack();
    RvMsg::init_auto_unpack();
    RwfMsg::init_auto_unpack();
    TibMsg::init_auto_unpack();
    TibSassMsg::init_auto_unpack();
    size_t i;
    for ( i = 0; i < sizeof( basic_match ) / sizeof( basic_match[ 0 ] ); i++ )
      MDMsg::add_match( basic_match[ i ] );
  }
}
