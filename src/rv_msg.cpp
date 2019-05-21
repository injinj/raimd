#include <raimd/rv_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/tib_sass_msg.h>

using namespace rai;
using namespace md;

namespace rai {
namespace md {
  static const uint8_t RV_TINY_SIZE  = 120; /* 78 */
  static const uint8_t RV_SHORT_SIZE = 121; /* 79 */
  static const uint8_t RV_LONG_SIZE  = 122; /* 7a */
  static const size_t MAX_RV_SHORT_SIZE = 0x7530; /* 30000 */

  enum {
    RV_BADDATA   = 0,
    RV_RVMSG     = 1,
    RV_SUBJECT   = 2,
    RV_DATETIME  = 3,
    RV_OPAQUE    = 7,
    RV_STRING    = 8,
    RV_BOOLEAN   = 9,
    RV_IPDATA    = 10,  /* 0a */
    RV_INT       = 11,  /* 0b */
    RV_UINT      = 12,  /* 0c */
    RV_REAL      = 13,  /* 0d */
    RV_ENCRYPTED = 32,  /* 20 */
    RV_ARRAY_I8  = 34,  /* 22 */
    RV_ARRAY_U8  = 35,  /* 23 */
    RV_ARRAY_I16 = 36,  /* 24 */
    RV_ARRAY_U16 = 37,  /* 25 */
    RV_ARRAY_I32 = 38,  /* 26 */
    RV_ARRAY_U32 = 39,  /* 27 */
    RV_ARRAY_I64 = 40,  /* 28 */
    RV_ARRAY_U64 = 41,  /* 29 */
    RV_ARRAY_F32 = 44,  /* 2c */
    RV_ARRAY_F64 = 45   /* 2d */
  };
}
}

const char *
RvMsg::get_proto_string( void )
{
  return "RVMSG";
}

static MDMatch rvmsg_match = {
  .off         = 4,
  .len         = 4, /* cnt of buf[] */
  .hint_size   = 1, /* cnt of hint[] */
  .ftype       = MD_MESSAGE,
  .buf         = { 0x99, 0x55, 0xee, 0xaa },
  .hint        = { 0xebf946be, 0 },
  .is_msg_type = RvMsg::is_rvmsg,
  .unpack      = RvMsg::unpack
};

bool
RvMsg::is_rvmsg( void *bb,  size_t off,  size_t end,  uint32_t h )
{
  uint32_t magic = 0;
  if ( off + 8 <= end )
    magic = get_u32<MD_BIG>( &((uint8_t *) bb)[ off + 4 ] );
  return magic == 0x9955eeaaU;
}

MDMsg *
RvMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
               MDMsgMem *m )
{
  uint32_t magic    = get_u32<MD_BIG>( &((uint8_t *) bb)[ off + 4 ] );
  size_t   msg_size = get_u32<MD_BIG>( &((uint8_t *) bb)[ off ] );

  if ( magic != 0x9955eeaaU || msg_size < 8 )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;

  /* check if another message is the first opaque field of the RvMsg */
  size_t off2 = off + 8,
         end2 = off + msg_size;
  MDMsg *msg = RvMsg::opaque_extract( (uint8_t *) bb, off2, end2, d, m );
  if ( msg == NULL ) {
    void * ptr;
    m->alloc( sizeof( RvMsg ), &ptr );
    msg = new ( ptr ) RvMsg( bb, off, end2, d, m );
  }
  return msg;
}

static inline bool
cmp_field( uint8_t *x,  size_t off,  uint8_t *y,  size_t ylen )
{
  for ( size_t i = 0; i < ylen; i++ )
    if ( x[ off + i ] != y[ i ] )
      return false;
  return true;
}

MDMsg *
RvMsg::opaque_extract( uint8_t *bb,  size_t off,  size_t end,  MDDict *d,
                       MDMsgMem *m )
{
  /* quick filter of below fields */
  if ( off+19 > end || bb[ off ] < 7 || bb[ off ] > 8 || bb[ off + 1 ] != '_' )
    return NULL;

  static uint8_t tibrv_encap_TIBMSG[]= { 8, '_','T','I','B','M','S','G', 0 };
  static uint8_t tibrv_encap_QFORM[] = { 7, '_','Q','F','O','R','M', 0 };
  static uint8_t tibrv_encap_data[]  = { 7, '_','d','a','t','a','_', 0 };
  static uint8_t tibrv_encap_RAIMSG[]= { 8, '_','R','A','I','M','S','G', 0 };
  size_t i, fsize, szbytes = 0;
  bool is_tibmsg = false, is_qform = false, is_data = false;

  i = sizeof( tibrv_encap_TIBMSG );
  if ( ! (is_tibmsg = cmp_field( bb, off, tibrv_encap_TIBMSG, i )) ) {
    i = sizeof( tibrv_encap_QFORM );
    if ( ! (is_qform = cmp_field( bb, off, tibrv_encap_QFORM, i )) ) {
      i = sizeof( tibrv_encap_data );
      if ( ! (is_data = cmp_field( bb, off, tibrv_encap_data, i )) ) {
        i = sizeof( tibrv_encap_RAIMSG );
        if ( ! (is_tibmsg = cmp_field( bb, off, tibrv_encap_RAIMSG, i )) ) {
          return NULL;
        }
      }
    }
  }

  off += i;
  if ( bb[ off++ ] != RV_OPAQUE )
    return NULL;

  fsize = bb[ off++ ];
  switch ( fsize ) {
    case RV_LONG_SIZE:
      fsize = get_u32<MD_BIG>( &bb[ off ] );
      szbytes = 4;
      break;
    case RV_SHORT_SIZE:
      fsize = get_u16<MD_BIG>( &bb[ off ] );
      szbytes = 2;
      break;
/*    case RV_TINY_SIZE:
      fsize = bb[ off ];
      szbytes = 1;
      break;*/
    default:
      break;
  }
  if ( fsize <= szbytes )
    return NULL;
  fsize -= szbytes;
  off   += szbytes;
  if ( off + fsize > end )
    return NULL;
  if ( ( is_tibmsg || is_data ) && TibMsg::is_tibmsg( bb, off, fsize, 0 ) )
    return TibMsg::unpack( bb, off, end, 0, d, m );
  if ( ( is_qform || is_data ) && TibSassMsg::is_tibsassmsg( bb, off, fsize, 0))
    return TibSassMsg::unpack( bb, off, end, 0, d, m );
  return NULL;
}

void
RvMsg::init_auto_unpack( void )
{
  MDMsg::add_match( rvmsg_match );
}

static const int rv_type_to_md_type[ 64 ] = {
    /*RV_BADDATA   =  0 */ 0, /* same as NODATA */
    /*RV_RVMSG     =  1 */ MD_MESSAGE,
    /*RV_SUBJECT   =  2 */ MD_SUBJECT,
    /*RV_DATETIME  =  3 */ MD_UINT,
                  /*  4 */ 0,
                  /*  5 */ 0,
                  /*  6 */ 0,
    /*RV_OPAQUE    =  7 */ MD_OPAQUE,
    /*RV_STRING    =  8 */ MD_STRING,
    /*RV_BOOLEAN   =  9 */ MD_BOOLEAN,
    /*RV_IPDATA    = 10 */ MD_IPDATA,
    /*RV_INT       = 11 */ MD_INT,
    /*RV_UINT      = 12 */ MD_UINT,
    /*RV_REAL      = 13 */ MD_REAL,
                  /* 14 */ 0,
                  /* 15 */ 0,
                  /* 16 */ 0,
    /* 17 */ 0, /* 18 */ 0, /* 19 */ 0, /* 20 */ 0, /* 21 */ 0,
    /* 22 */ 0, /* 23 */ 0, /* 24 */ 0, /* 25 */ 0, /* 26 */ 0,
    /* 27 */ 0, /* 28 */ 0, /* 29 */ 0, /* 30 */ 0, /* 31 */ 0,
    /*RV_ENCRYPTED = 32 */ MD_OPAQUE,
                  /* 33 */ 0,
    /*RV_ARRAY_I8  = 34 */ MD_ARRAY,
    /*RV_ARRAY_U8  = 35 */ MD_ARRAY,
    /*RV_ARRAY_I16 = 36 */ MD_ARRAY,
    /*RV_ARRAY_U16 = 37 */ MD_ARRAY,
    /*RV_ARRAY_I32 = 38 */ MD_ARRAY,
    /*RV_ARRAY_U32 = 39 */ MD_ARRAY,
    /*RV_ARRAY_I64 = 40 */ MD_ARRAY,
    /*RV_ARRAY_U64 = 41 */ MD_ARRAY,
                  /* 42 */ 0,
                  /* 43 */ 0,
    /*RV_ARRAY_F32 = 44 */ MD_ARRAY,
    /*RV_ARRAY_F64 = 45 */ MD_ARRAY
};


int
RvMsg::get_sub_msg( MDReference &mref, MDMsg *&msg )
{
  uint8_t * bb    = (uint8_t *) this->msg_buf;
  size_t    start = (size_t) ( mref.fptr - bb );
  void    * ptr;

  this->mem->alloc( sizeof( RvMsg ), &ptr );
  msg = new ( ptr ) RvMsg( bb, start, start + mref.fsize, this->dict,
                           this->mem );
  return 0;
}

int
RvMsg::get_field_iter( MDFieldIter *&iter )
{
  void * ptr;
  this->mem->alloc( sizeof( RvFieldIter ), &ptr );
  iter = new ( ptr ) RvFieldIter( *this );
  return 0;
}

int
RvFieldIter::get_name( MDName &name )
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  name.fid      = 0;
  name.fnamelen = this->name_len;
  if ( name.fnamelen > 0 )
    name.fname = (char *) &buf[ this->field_start + 1 ];
  else
    name.fname = NULL;
  return 0;
}

int
RvFieldIter::get_reference( MDReference &mref )
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  mref.fendian = MD_BIG;
  mref.ftype   = (MDType) rv_type_to_md_type[ this->type ];
  mref.fsize   = this->size;
  mref.fptr    = &buf[ this->field_end - this->size ];
  if ( mref.ftype == MD_ARRAY ) {
    switch ( this->type ) {
      default: break;
      case RV_ARRAY_I8:
      case RV_ARRAY_U8:  mref.fentrysz = 1; break;
      case RV_ARRAY_I16:
      case RV_ARRAY_U16: mref.fentrysz = 2; break;
      case RV_ARRAY_I32:
      case RV_ARRAY_U32:
      case RV_ARRAY_F32: mref.fentrysz = 4; break;
      case RV_ARRAY_I64:
      case RV_ARRAY_U64:
      case RV_ARRAY_F64: mref.fentrysz = 8; break;
    }
    switch ( this->type ) {
      default: break;
      case RV_ARRAY_U8:
      case RV_ARRAY_U16: 
      case RV_ARRAY_U32:
      case RV_ARRAY_U64: mref.fentrytp = MD_UINT; break;
      case RV_ARRAY_I8:
      case RV_ARRAY_I16:
      case RV_ARRAY_I32:
      case RV_ARRAY_I64: mref.fentrytp = MD_INT; break;
      case RV_ARRAY_F32: 
      case RV_ARRAY_F64: mref.fentrytp = MD_REAL; break;
    }
  }
  return 0;
}

int
RvFieldIter::find( const char *name )
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  size_t len = ( name != NULL ? ::strlen( name ) + 1 : 0 );
  int status;
  if ( (status = this->first()) == 0 ) {
    do {
      if ( (uint8_t) len == this->name_len ) {
        if ( ::memcmp( &buf[ this->field_start + 1 ], name, len ) == 0 )
          return 0;
      }
    } while ( (status = this->next()) == 0 );
  }
  return status;
}

int
RvFieldIter::first( void )
{
  this->field_start = this->iter_msg.msg_off + 8;
  this->field_end   = this->iter_msg.msg_end;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  return this->unpack();
}

int
RvFieldIter::next( void )
{
  this->field_start = this->field_end;
  this->field_end   = this->iter_msg.msg_end;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  return this->unpack();
}

int
RvFieldIter::unpack( void )
{
  const uint8_t * buf     = (uint8_t *) this->iter_msg.msg_buf;
  size_t          i       = this->field_start;
  uint8_t         szbytes = 0;

  if ( i >= this->field_end )
    goto bad_bounds;
  this->name_len = buf[ i++ ];
  i += this->name_len;
  if ( i >= this->field_end )
    goto bad_bounds;
  this->type = buf[ i++ ];

  switch ( this->type ) {
    case RV_RVMSG:
      if ( i + 5 > this->field_end )
        goto bad_bounds;
      if ( buf[ i++ ] != RV_LONG_SIZE )
        return Err::BAD_FIELD_SIZE;
      this->size = get_u32<MD_BIG>( &buf[ i ] );
      break;

    case RV_STRING:
    case RV_SUBJECT:
    case RV_ENCRYPTED:
    case RV_OPAQUE:
    case RV_ARRAY_I8:
    case RV_ARRAY_U8:
    case RV_ARRAY_I16:
    case RV_ARRAY_U16:
    case RV_ARRAY_I32:
    case RV_ARRAY_U32:
    case RV_ARRAY_I64:
    case RV_ARRAY_U64:
    case RV_ARRAY_F32:
    case RV_ARRAY_F64:
      this->size = buf[ i++ ];
      switch ( this->size ) {
        case RV_LONG_SIZE:
          if ( i + 4 > this->field_end )
            goto bad_bounds;
          this->size = get_u32<MD_BIG>( &buf[ i ] );
          szbytes = 4;
          break;
        case RV_SHORT_SIZE:
          if ( i + 2 > this->field_end )
            goto bad_bounds;
          this->size = get_u16<MD_BIG>( &buf[ i ] );
          szbytes = 2;
          break;
/*        case RV_TINY_SIZE:
          if ( i + 1 > this->field_end )
            goto bad_bounds;
          this->size = buf[ i ];
          szbytes = 1;
          break;*/
        default:
          break;
      }
      break;

    case RV_DATETIME:
    case RV_BOOLEAN:
    case RV_IPDATA:
    case RV_INT:
    case RV_UINT:
    case RV_REAL:
      this->size = buf[ i++ ];
      break;

    default:
      return Err::BAD_FIELD_TYPE;
  }

  i += this->size;
  if ( this->size < szbytes )
    goto bad_bounds;
  this->size -= szbytes;
  if ( i > this->field_end ) {
  bad_bounds:;
    return Err::BAD_FIELD_BOUNDS;
  }
  this->field_end = i;
  return 0;
}

int
RvMsgWriter::append_ref( const char *fname,  size_t fname_len,
                          MDReference &mref )
{
  uint8_t * ptr = &this->buf[ this->off ];
  size_t    len = 1 + fname_len + 1 + mref.fsize,
            szbytes;

  if ( mref.fsize < RV_TINY_SIZE )
    szbytes = 1;
  else if ( mref.fsize < MAX_RV_SHORT_SIZE )
    szbytes = 3;
  else
    szbytes = 5;

  len += szbytes;
  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  if ( fname_len > 0xff )
    return Err::BAD_NAME;
  ptr[ 0 ] = (uint8_t) fname_len;
  ::memcpy( &ptr[ 1 ], fname, fname_len );
  ptr = &ptr[ fname_len + 1 ];

  switch ( mref.ftype ) {
    default:
    case MD_OPAQUE:  ptr[ 0 ] = RV_OPAQUE; break;
    case MD_PARTIAL:
    case MD_STRING:  ptr[ 0 ] = RV_STRING; break;
    case MD_BOOLEAN: ptr[ 0 ] = RV_BOOLEAN; break;
    case MD_IPDATA:  ptr[ 0 ] = RV_IPDATA; break;
    case MD_INT:     ptr[ 0 ] = RV_INT; break;
    case MD_UINT:    ptr[ 0 ] = RV_UINT; break;
    case MD_REAL:    ptr[ 0 ] = RV_REAL; break;
  }

  if ( szbytes == 1 ) {
    ptr[ 1 ] = mref.fsize;
    ptr = &ptr[ 2 ];
  }
  else if ( szbytes == 3 ) {
    ptr[ 1 ] = RV_SHORT_SIZE;
    ptr[ 2 ] = ( ( mref.fsize + 2 ) >> 8 ) & 0xffU;
    ptr[ 3 ] = ( mref.fsize + 2 ) & 0xffU;
    ptr = &ptr[ 4 ];
  }
  else {
    ptr[ 1 ] = RV_LONG_SIZE;
    ptr[ 2 ] = ( ( mref.fsize + 4 ) >> 24 ) & 0xffU;
    ptr[ 3 ] = ( ( mref.fsize + 4 ) >> 16 ) & 0xffU;
    ptr[ 4 ] = ( ( mref.fsize + 4 ) >> 8 ) & 0xffU;
    ptr[ 5 ] = ( mref.fsize + 4 ) & 0xffU;
    ptr = &ptr[ 6 ];
  }
  /* invert endian, for little -> big */
  if ( mref.fendian != MD_BIG && mref.fsize > 1 &&
       ( mref.ftype == MD_UINT || mref.ftype == MD_INT ||
         mref.ftype == MD_REAL ) ) {
    size_t off = mref.fsize;
    ptr[ 0 ] = mref.fptr[ --off ];
    ptr[ 1 ] = mref.fptr[ --off ];
    if ( off > 0 ) {
      ptr[ 2 ] = mref.fptr[ --off ];
      ptr[ 3 ] = mref.fptr[ --off ];
      if ( off > 0 ) {
        ptr[ 4 ] = mref.fptr[ --off ];
        ptr[ 5 ] = mref.fptr[ --off ];
        ptr[ 6 ] = mref.fptr[ --off ];
        ptr[ 7 ] = mref.fptr[ --off ];
      }
    }
  }
  else {
    ::memcpy( ptr, mref.fptr, mref.fsize );
  }
  this->off += len;
  return 0;
}

int
RvMsgWriter::append_decimal( const char *fname,  size_t fname_len,
                              MDDecimal &dec )
{
  uint8_t * ptr = &this->buf[ this->off ];
  size_t    len = 1 + fname_len + 1 + 1 + 8;
  double    val;
  int       status;

  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  if ( fname_len > 0xff )
    return Err::BAD_NAME;
  if ( (status = dec.get_real( val )) != 0 )
    return status;
  ptr[ 0 ] = (uint8_t) fname_len;
  ::memcpy( &ptr[ 1 ], fname, fname_len );
  ptr = &ptr[ fname_len + 1 ];
  ptr[ 0 ] = (uint8_t) RV_REAL;
  ptr[ 1 ] = (uint8_t) 8;
  ptr = &ptr[ 2 ];

  if ( ! is_little_endian )
    ::memcpy( ptr, &val, 8 );
  else {
    uint8_t * fptr = (uint8_t *) (void *) &val;
    for ( size_t i = 0; i < 8; i++ )
      ptr[ i ] = fptr[ 7 - i ];
  }
  this->off += len;
  return 0;
}

int
RvMsgWriter::append_time( const char *fname,  size_t fname_len,  MDTime &time )
{
  uint8_t * ptr = &this->buf[ this->off ];
  char      sbuf[ 32 ];
  size_t    n   = time.get_string( sbuf, sizeof( sbuf ) );
  size_t    len = 1 + fname_len + 1 + 1 + n + 1;

  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  if ( fname_len > 0xff )
    return Err::BAD_NAME;

  ptr[ 0 ] = (uint8_t) fname_len;
  ::memcpy( &ptr[ 1 ], fname, fname_len );
  ptr = &ptr[ fname_len + 1 ];
  ptr[ 0 ] = (uint8_t) RV_STRING;
  ptr[ 1 ] = (uint8_t) ( n + 1 );
  ptr = &ptr[ 2 ];
  ::memcpy( ptr, sbuf, n + 1 );
  this->off += len;
  return 0;
}

int
RvMsgWriter::append_date( const char *fname,  size_t fname_len,  MDDate &date )
{
  uint8_t * ptr = &this->buf[ this->off ];
  char      sbuf[ 32 ];
  size_t    n   = date.get_string( sbuf, sizeof( sbuf ) );
  size_t    len = 1 + fname_len + 1 + 1 + n + 1;

  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  if ( fname_len > 0xff )
    return Err::BAD_NAME;

  ptr[ 0 ] = (uint8_t) fname_len;
  ::memcpy( &ptr[ 1 ], fname, fname_len );
  ptr = &ptr[ fname_len + 1 ];
  ptr[ 0 ] = (uint8_t) RV_STRING;
  ptr[ 1 ] = (uint8_t) ( n + 1 );
  ptr = &ptr[ 2 ];
  ::memcpy( ptr, sbuf, n + 1 );
  this->off += len;
  return 0;
}

