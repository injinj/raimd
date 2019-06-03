#include <stdio.h>
#include <math.h>
#include <raimd/rwf_msg.h>
#include <raimd/md_dict.h>

using namespace rai;
using namespace md;

const char *
RwfMsg::get_proto_string( void )
{
  return "RWF";
}

static MDMatch rwf_match = {
  .off         = 0,
  .len         = 0, /* cnt of buf[] */
  .hint_size   = 1, /* cnt of hint[] */
  .ftype       = MD_MESSAGE,
  .buf         = { 0 },
  .hint        = { 0x25cdabca },
  .is_msg_type = RwfMsg::is_rwf,
  .unpack      = (md_msg_unpack_f) RwfMsg::unpack
};

void
RwfMsg::init_auto_unpack( void )
{
  MDMsg::add_match( rwf_match );
}

bool
RwfMsg::is_rwf( void *bb,  size_t off,  size_t end,  uint32_t )
{
  RwfFieldListHdr hdr;
  return off < end &&
         RwfMsg::parse_header( &((uint8_t *) bb)[ off ], end - off, hdr ) == 0;
}

int
RwfMsg::parse_header( const uint8_t *buf,  size_t buflen,
                      RwfFieldListHdr &hdr )
{
  size_t   i = 0;
  uint32_t tp;
  if ( buflen < 4 )
    return Err::BAD_HEADER;
  if ( get_u32<MD_BIG>( &buf[ i ] ) == 0x25cdabcaU ) {
    if ( buflen < 8 )
      return Err::BAD_HEADER;
    tp = get_u32<MD_BIG>( &buf[ i + 4 ] ); i += 8;
    if ( tp != 132 ) /* should be DT_FIELD_LIST */
      return Err::BAD_HEADER;
    hdr.data_type = (uint16_t) tp;
  }
  else {
    hdr.data_type = 0;
  }
  if ( buflen < i + 3 ) /* flags + field_cnt */
    return Err::BAD_HEADER;
  hdr.flags   = buf[ i++ ];
  hdr.dict_id = 1;
  hdr.flist   = 0;
  /* which dict and flist */
  if ( ( hdr.flags & HAS_FIELD_LIST_INFO ) != 0 ) {
    if ( buflen < i + 2 + (size_t) buf[ i ] + 1 )
      return Err::BAD_HEADER;
    if ( buf[ i ] == 3 ) {
      hdr.dict_id = buf[ i + 1 ];
      hdr.flist   = get_i16<MD_BIG>( &buf[ i + 2 ] );
    }
    i += ( buf[ i ] + 1 ); /* size byte + field list info */
  }
  /* no decoders for these */
  if ( ( hdr.flags & ( HAS_SET_DATA | HAS_SET_ID ) ) != 0 )
    return Err::BAD_HEADER;

  /* count of fields encoded in msg */
  hdr.field_cnt  = get_u16<MD_BIG>( &buf[ i ] ); i += 2;
  hdr.data_start = i;
  /* every field has at least a fid */
  if ( hdr.field_cnt * 2 > buflen - i )
    return Err::BAD_HEADER;

  return 0;
}


RwfMsg *
RwfMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,
                MDDict *d,  MDMsgMem *m )
{
  RwfFieldListHdr hdr;
  if ( RwfMsg::parse_header( &((uint8_t *) bb)[ off ],  end - off,
                             hdr ) != 0 )
    return NULL;
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;

  void * ptr;
  m->alloc( sizeof( RwfMsg ), &ptr );
  for ( ; d != NULL; d = d->next )
    if ( d->dict_type[ 0 ] == 'a' ) /* need app_a type */
      break;
  RwfMsg *msg = new ( ptr ) RwfMsg( bb, off, end, d, m );
  msg->hdr = hdr;
  return msg;
}

int
RwfMsg::get_field_iter( MDFieldIter *&iter )
{
  void * ptr;
  this->mem->alloc( sizeof( RwfFieldIter ), &ptr );
  iter = new ( ptr ) RwfFieldIter( *this );
  return 0;
}

inline void
RwfFieldIter::lookup_fid( void )
{
  if ( this->ftype == MD_NODATA ) {
    if ( this->iter_msg.dict != NULL )
      this->iter_msg.dict->lookup( this->fid, this->ftype, this->fsize,
                                   this->fnamelen, this->fname );
    if ( this->ftype == MD_NODATA ) { /* undefined fid or no dictionary */
      this->ftype    = MD_OPAQUE;
      this->fname    = NULL;
      this->fnamelen = 0;
    }
  }
}

int
RwfFieldIter::get_name( MDName &name )
{
  this->lookup_fid();
  name.fid      = this->fid;
  name.fnamelen = this->fnamelen;
  name.fname    = this->fname;
  return 0;
}

int
RwfFieldIter::get_hint_reference( MDReference &mref )
{
  mref.zero();
  return Err::NOT_FOUND;
}

int
RwfFieldIter::get_enum( MDReference &mref,  MDEnum &enu )
{
  if ( mref.ftype == MD_ENUM ) {
    if ( this->iter_msg.dict != NULL ) {
      enu.val = get_uint<uint16_t>( mref );
      if ( this->iter_msg.dict->get_enum_text( this->fid, enu.val,
                                               enu.disp, enu.disp_len ) )
        return 0;
    }
  }
  enu.zero();
  return Err::NO_ENUM;
}

static inline int8_t
rwf_to_md_decimal_hint( uint8_t hint )
{
  static int8_t to_md[] = {
    MD_DEC_LOGn10_10-4,  /* ^10-14 */
    MD_DEC_LOGn10_10-3,
    MD_DEC_LOGn10_10-2,
    MD_DEC_LOGn10_10-1,
    MD_DEC_LOGn10_10,
    MD_DEC_LOGn10_9,
    MD_DEC_LOGn10_8,
    MD_DEC_LOGn10_7,
    MD_DEC_LOGn10_6,
    MD_DEC_LOGn10_5,
    MD_DEC_LOGn10_4,
    MD_DEC_LOGn10_3,
    MD_DEC_LOGn10_2,
    MD_DEC_LOGn10_1,
    MD_DEC_INTEGER,   /* 14 */
    MD_DEC_LOGp10_1,
    MD_DEC_LOGp10_2,
    MD_DEC_LOGp10_3,
    MD_DEC_LOGp10_4,
    MD_DEC_LOGp10_5,
    MD_DEC_LOGp10_6,
    MD_DEC_LOGp10_7,  /* ^10+7 */
    MD_DEC_INTEGER,   /* 22 */
    MD_DEC_FRAC_2,
    MD_DEC_FRAC_4,
    MD_DEC_FRAC_8,
    MD_DEC_FRAC_16,
    MD_DEC_FRAC_32,
    MD_DEC_FRAC_64,
    MD_DEC_FRAC_128,
    MD_DEC_FRAC_256,
    MD_DEC_INF,       /* 33 */
    MD_DEC_NINF,      /* 34 */
    MD_DEC_NAN        /* 35 */
  };
  return to_md[ hint < 36 ? hint : 35 ];
}

static inline uint8_t
md_to_rwf_decimal_hint( int8_t hint )
{
  if ( hint >= 0 ) {
    if ( hint <= MD_DEC_INTEGER )
      return 14;
    if ( hint <= MD_DEC_FRAC_512 )
      return hint - MD_DEC_FRAC_2 + 23;
    return hint - MD_DEC_LOGp10_1 + 15;
  }
  /* hint < 0 */
  if ( hint > MD_DEC_LOGn10_1 ) {
    if ( hint == MD_DEC_INF )
      return 33;
    if ( hint == MD_DEC_NINF )
      return 34;
    return 35;
  }
  return hint - MD_DEC_LOGn10_1 + 13;
}

static inline bool
mref_to_value( MDReference &mref,  MDValue &val )
{
  if ( mref.fsize > 0 ) {
    bool sign_extend = false;
    size_t i = 1;
    uint64_t ival = mref.fptr[ 0 ];
    if ( ( ival & 0x80 ) != 0 ) {
      if ( mref.ftype == MD_DECIMAL || mref.ftype == MD_INT )
        sign_extend = true;
    }
    for ( ; i < mref.fsize; i++ )
      ival = ( ival << 8 ) | (uint64_t) mref.fptr[ i ];
    if ( sign_extend && i < 8 )
      ival |= ( ~(uint64_t) 0 ) << ( i * 8 );
    val.u64 = ival;
    return true;
  }
  return false;
}

static inline bool
to_digit( uint8_t c,  uint8_t &d )
{
  return (d = (c - '0')) <= 9;
}

static inline void
get_short( const uint8_t *buf,  size_t &i,  const uint8_t *eos,
           uint16_t &ival )
{
  uint8_t d;
  ival = 0;
  for ( i++; &buf[ i ] < eos && to_digit( buf[ i ], d ); i++ )
    ival = ( ival * 10 ) + d;
}

int
RwfFieldIter::get_reference( MDReference &mref )
{
  uint8_t * buf = &((uint8_t *) this->iter_msg.msg_buf)[ this->data_off ];
  mref.fendian  = MD_BIG;
  mref.fentrysz = 0;
  mref.fentrytp = MD_NODATA;

  this->lookup_fid(); /* determine type from dictionary, if loaded */
  mref.ftype    = this->ftype;
  mref.fsize    = this->field_end - this->data_off;
  mref.fptr     = buf;

  switch ( this->ftype ) {
    case MD_OPAQUE:
      break;
    case MD_INT:
    case MD_UINT:
    case MD_ENUM:
    case MD_BOOLEAN:
      if ( ( mref.fsize & ( mref.fsize - 1 ) ) != 0 ) { /* not a power of 2 */
        if ( mref_to_value( mref, this->val ) ) {
          mref.fptr    = (uint8_t *) (void *) &this->val;
          mref.fendian = md_endian;
          do {
            mref.fsize++;
          } while ( ( mref.fsize & ( mref.fsize - 1 ) ) != 0 );
        }
      }
      break;

    case MD_TIME:
      if ( mref.fsize >= 2 ) {
        this->time.hour = buf[ 0 ];
        this->time.min  = buf[ 1 ];
        if ( mref.fsize >= 3 ) {
          this->time.sec  = buf[ 2 ];
          this->time.resolution = MD_RES_SECONDS;
        }
        else {
          this->time.resolution = MD_RES_MINUTES;
        }
        this->time.fraction = 0;
      }
      else {
        this->time.zero();
        this->time.resolution = MD_RES_NULL;
      }
      mref.fptr    = (uint8_t *) (void *) &this->time;
      mref.fsize   = sizeof( this->time );
      mref.fendian = md_endian;
      break;

    case MD_DATE:
      if ( mref.fsize == 4 ) {
        this->date.day = buf[ 0 ];
        this->date.mon = buf[ 1 ];
        this->date.year = get_u16<MD_BIG>( &buf[ 2 ] );
      }
      else {
        this->date.zero();
      }
      mref.fptr    = (uint8_t *) (void *) &this->date;
      mref.fsize   = sizeof( this->date );
      mref.fendian = md_endian;
      break;

    case MD_DECIMAL:
      if ( mref.fsize >= 1 ) {
        this->dec.hint = rwf_to_md_decimal_hint( buf[ 0 ] );
        mref.fptr = &buf[ 1 ];
        mref.fsize -= 1;
        if ( mref_to_value( mref, this->val ) )
          this->dec.ival = this->val.i64;
        else
          this->dec.ival = 0;
      }
      else {
        this->dec.ival = 0;
        this->dec.hint = MD_DEC_NULL;
      }
      mref.fptr    = (uint8_t *) (void *) &this->dec;
      mref.fsize   = sizeof( this->dec );
      mref.fendian = md_endian;
      break;

    case MD_PARTIAL:
      if ( mref.fsize > 3 ) {
        enum {
          CSI1 = 0x5bU, /* '[' control sequence inducer part */
          HPA  = 0x60U, /* '`' -- the horizontal position adjust char */
          REP  = 0x62U, /* 'b' -- repitition char */
          ESC  = 0x1bU, /* <ESC> <CSI1> n ` -- for partials */
          CSI2 = 0x1bU | 0x80U /* <CSI2> n ` -- second form of partials */
        };

        if ( ( buf[ 0 ] & 0x7fU ) == ESC ) {
          uint8_t *eos = &buf[ mref.fsize ];
          size_t off = 0;
          if ( buf[ 0 ] == CSI2 ) { /* 0x80 | ESC n ` */
            get_short( buf, off, eos, this->position );
          }
          else if ( buf[ 1 ] == CSI1 ) { /* ESC [ n ` */
            get_short( &buf[ 1 ], off, eos, this->position );
            off++;
          }
          if ( off > 0 && &buf[ off ] < eos && buf[ off ] == HPA ) {
            mref.fptr     = &buf[ off + 1 ];
            mref.fsize   -= off + 1;
            mref.fentrysz = this->position;
            return 0;
          }
        }
      }
      /* FALLTHRU */
    case MD_STRING:
      if ( mref.fsize == 0 ) {
        static uint8_t nullchar[ 1 ] = { 0 };
        mref.fsize = 1;
        mref.fptr  = nullchar;
      }
      break;

    default:
      mref.ftype = MD_OPAQUE;
      break;
  }
  return 0;
}

int
RwfFieldIter::find( const char *name,  size_t name_len,  MDReference &mref )
{
  if ( this->iter_msg.dict == NULL )
    return Err::NO_DICTIONARY;

  int status = Err::NOT_FOUND;
  if ( name != NULL ) {
    MDFid    fid;
    MDType   ftype;
    uint32_t fsize;
    if ( this->iter_msg.dict->get( name, name_len, fid, ftype, fsize ) ) {
      if ( (status = this->first()) == 0 ) {
        do {
          if ( this->fid == fid )
            return this->get_reference( mref );
        } while ( (status = this->next()) == 0 );
      }
    }
  }
  return status;
}

int
RwfFieldIter::first( void )
{
  size_t fcnt  = (size_t) ((RwfMsg &) this->iter_msg).hdr.field_cnt,
         dstrt = ((RwfMsg &) this->iter_msg).hdr.data_start;
  this->field_start = this->iter_msg.msg_off + dstrt;
  this->field_end   = this->iter_msg.msg_end;
  this->field_idx   = 0;
  if ( fcnt == 0 || this->field_start >= this->field_end ) {
    this->field_end = this->field_start;
    return Err::NOT_FOUND;
  }
  return this->unpack();
}

int
RwfFieldIter::next( void )
{
  size_t fcnt = (size_t) ((RwfMsg &) this->iter_msg).hdr.field_cnt;
  if ( ++this->field_idx >= fcnt )
    return Err::NOT_FOUND;
  this->field_start = this->field_end;
  this->field_end   = this->iter_msg.msg_end;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  return this->unpack();
}

int
RwfFieldIter::unpack( void )
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  size_t    i   = this->field_start;
  /* <fid><fsize><fdata> */
  this->ftype = MD_NODATA;
  this->fid = get_i16<MD_BIG>( &buf[ i ] ); i += 2;
  if ( (this->fsize = buf[ i ]) < 0xfe ) {
    i++;
  }
  else if ( this->fsize == 0xfe ) {
    if ( i + 3 > this->field_end )
      goto bad_bounds;
    this->fsize = get_u16<MD_BIG>( &buf[ i + 1 ] );
    i += 3;
  }
  else {
    if ( i + 5 > this->field_end )
      goto bad_bounds;
    this->fsize = get_u32<MD_BIG>( &buf[ i + 1 ] );
    i += 5;
  }
  if ( i + this->fsize > this->field_end )
    goto bad_bounds;
  this->data_off  = i;
  this->field_end = i + this->fsize;
  return 0;
bad_bounds:;
  return Err::BAD_FIELD_BOUNDS;
}

RwfMsgWriter::RwfMsgWriter( MDDict *d,  void *bb,  size_t len )
    : dict( d ), buf( (uint8_t *) bb ), off( 15 ), buflen( len ),
      nflds( 0 ), flist( 0 )
{
  for ( ; d != NULL; d = d->next ) {
    if ( d->dict_type[ 0 ] == 'a' ) { /* look for app_a type */
      this->dict = d;
      break;
    }
  }
}

int
RwfMsgWriter::append_ival( const char *fname,  size_t fname_len,
                           const void *ival,  size_t ilen,  MDType t )
{
  uint32_t fsize;
  MDType   ftype;
  MDFid    fid;

  if ( ! this->dict->get( fname, fname_len, fid, ftype, fsize ) )
    return Err::UNKNOWN_FID;
  if ( ftype == MD_UINT || ftype == MD_INT ||
       ftype == MD_ENUM || ftype == MD_BOOLEAN )
    return this->pack_ival( fid, (const uint8_t *) ival, ilen );

  MDReference mref;
  mref.fptr    = (uint8_t *) (void *) ival;
  mref.fsize   = ilen;
  mref.ftype   = t;
  mref.fendian = md_endian;
  return this->append_ref( fid, ftype, fsize, mref );
}

int
RwfMsgWriter::append_ival( MDFid fid,  const void *ival, size_t ilen, MDType t )
{
  const char * fname;
  uint8_t      fname_len;
  uint32_t     fsize;
  MDType       ftype;

  if ( ! this->dict->lookup( fid, ftype, fsize, fname_len, fname ) )
    return Err::UNKNOWN_FID;
  if ( ftype == MD_UINT || ftype == MD_ENUM || ftype == MD_BOOLEAN )
    return this->pack_uval( fid, (const uint8_t *) ival, ilen );
  if ( ftype == MD_INT )
    return this->pack_ival( fid, (const uint8_t *) ival, ilen );

  MDReference mref;
  mref.fptr    = (uint8_t *) (void *) ival;
  mref.fsize   = ilen;
  mref.ftype   = t;
  mref.fendian = md_endian;
  return this->append_ref( fid, ftype, fsize, mref );
}

static inline void
copy_rwf_int_val( const uint8_t *ival,  uint8_t *ptr,  size_t ilen )
{
  ptr[ 0 ] = ival[ --ilen ];
  if ( ilen > 0 ) {
    ptr[ 1 ] = ival[ --ilen ];
    if ( ilen > 0 ) {
      ptr[ 2 ] = ival[ --ilen ];
      ptr[ 3 ] = ival[ --ilen ];
      if ( ilen > 0 ) {
        ptr[ 4 ] = ival[ --ilen ];
        ptr[ 5 ] = ival[ --ilen ];
        ptr[ 6 ] = ival[ --ilen ];
        ptr[ 7 ] = ival[ --ilen ];
      }
    }
  }
}

int
RwfMsgWriter::pack_uval( MDFid fid,  const uint8_t *ival,  size_t ilen )
{
  uint8_t * ptr = &this->buf[ this->off ];
  MDValue   val;

  ::memcpy( &val.u64, ival, ilen );
  switch ( ilen ) {
    case 8: if ( ( val.u64 >> 32 ) != 0 ) break;
      /* FALLTHRU */
    case 4: if ( ( val.u32 >> 16 ) != 0 ) { ilen = 4; break; }
      /* FALLTHRU */
    case 2: if ( ( val.u16 >> 8 ) != 0 ) { ilen = 2; break; }
      /* FALLTHRU */
    default: ilen = 1; break;
  }

  size_t len = ilen + 3;
  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  this->off += len;
  this->nflds++;

  ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
  ptr[ 1 ] = fid & 0xffU;
  ptr[ 2 ] = (uint8_t) ilen;

  copy_rwf_int_val( ival, &ptr[ 3 ], ilen );

  return 0;
}

static inline size_t
get_rwf_int_len( const uint8_t *ival,  size_t ilen )
{
  MDValue val;

  ::memcpy( &val.u64, ival, ilen );
  switch ( ilen ) {
    case 8: {
      uint32_t u32 = ( val.u64 >> 32 );
      if ( u32 != 0 && u32 != 0xffffffffU )
        break;
    }
    /* FALLTHRU */
    case 4: {
      uint16_t u16 = ( val.u32 >> 16 );
      if ( u16 != 0 && u16 != 0xffffU ) {
        ilen = 4;
        break;
      }
    }
    /* FALLTHRU */
    case 2: {
      uint8_t u8 = ( val.u16 >> 8 );
      if ( u8 != 0 && u8 != 0xff ) {
        ilen = 2;
        break;
      }
    }
    /* FALLTHRU */
    default:
      ilen = 1;
      break;
  }
  return ilen;
}

static inline size_t
rwf_pack_size( uint32_t fsize )
{
  if ( fsize < 0xfeU )
    return 1 /* 1 byte len */ + 2 /* fid */ + fsize;
  if ( fsize <= 0xffffU )
    return 3 /* 3 byte len */ + 2 /* fid */ + fsize;
  return 5 /* 5 byte len */ + 2 /* fid */ + fsize;
}

static inline size_t
pack_rwf_size( uint8_t *ptr,  size_t fsize )
{
  if ( fsize < 0xfeU ) {
    ptr[ 0 ] = (uint8_t) fsize;
    return 1;
  }
  if ( fsize <= 0xffffU ) {
    ptr[ 0 ] = 0xfeU;
    ptr[ 1 ] = ( fsize >> 8 ) & 0xffU;
    ptr[ 2 ] = fsize & 0xffU;
    return 2;
  }
  ptr[ 0 ] = 0xffU;
  ptr[ 1 ] = ( fsize >> 24 ) & 0xffU;
  ptr[ 2 ] = ( fsize >> 16 ) & 0xffU;
  ptr[ 3 ] = ( fsize >> 8 ) & 0xffU;
  ptr[ 4 ] = fsize & 0xffU;
  return 5;
}

int
RwfMsgWriter::pack_ival( MDFid fid,  const uint8_t *ival,  size_t ilen )
{
  uint8_t * ptr = &this->buf[ this->off ];
  ilen = get_rwf_int_len( ival, ilen );
  size_t len = ilen + 3;

  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  this->off += len;
  this->nflds++;

  ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
  ptr[ 1 ] = fid & 0xffU;
  ptr[ 2 ] = (uint8_t) ilen;

  copy_rwf_int_val( ival, &ptr[ 3 ], ilen );

  return 0;
}

int
RwfMsgWriter::pack_partial( MDFid fid,  const uint8_t *fptr,  size_t fsize,
                            size_t foffset )
{
  uint8_t * ptr = &this->buf[ this->off ];
  size_t partial_len = ( foffset > 100 ? 3 : foffset > 10 ? 2 : 1 );
  size_t len = rwf_pack_size( fsize + partial_len + 3 );
  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  this->off += len;
  this->nflds++;

  ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
  ptr[ 1 ] = fid & 0xffU;

  size_t n = 2 + pack_rwf_size( &ptr[ 2 ], fsize + partial_len + 3 );
  ptr[ n++ ] = 0x1b;
  ptr[ n++ ] = '[';
  if ( partial_len == 3 )
    ptr[ n++ ] = ( ( foffset / 100 ) % 10 ) + '0';
  else if ( partial_len == 2 )
    ptr[ n++ ] = ( ( foffset / 10 ) % 10 ) + '0';
  ptr[ n++ ] = ( foffset % 10 ) + '0';
  ptr[ n++ ] = '`';

  ::memcpy( &ptr[ n ], fptr, fsize );

  return 0;
}

int
RwfMsgWriter::append_ref( MDFid fid,  MDReference &mref )
{
  const char * fname;
  uint8_t      fname_len;
  uint32_t     fsize;
  MDType       ftype;

  if ( ! this->dict->lookup( fid, ftype, fsize, fname_len, fname ) )
    return Err::UNKNOWN_FID;
  return this->append_ref( fid, ftype, fsize, mref );
}

int
RwfMsgWriter::append_ref( const char *fname,  size_t fname_len,
                          MDReference &mref )
{
  uint32_t fsize;
  MDType   ftype;
  MDFid    fid;

  if ( ! this->dict->get( fname, fname_len, fid, ftype, fsize ) )
    return Err::UNKNOWN_FID;
  return this->append_ref( fid, ftype, fsize, mref );
}

int
RwfMsgWriter::append_ref( MDFid fid,  MDType ftype,  uint32_t fsize,
                          MDReference &mref )
{
  char      str_buf[ 64 ];
  uint8_t * ptr  = &this->buf[ this->off ],
          * fptr = mref.fptr;
  size_t    len, slen;
  MDValue   val;
  MDEndian  fendian = mref.fendian;

  switch ( ftype ) {
    case MD_UINT:
      if ( cvt_number<uint64_t>( mref, val.u64 ) != 0 )
        return Err::BAD_CVT_NUMBER;
      fptr = (uint8_t *) (void *) &val.u64;
      return this->pack_uval( fid, fptr, 8 );

    case MD_INT:
      if ( cvt_number<int64_t>( mref, val.i64 ) != 0 )
        return Err::BAD_CVT_NUMBER;
      fptr = (uint8_t *) (void *) &val.i64;
      return this->pack_ival( fid, fptr, 8 );

    case MD_REAL:
      if ( cvt_number<double>( mref, val.f64 ) != 0 )
        return Err::BAD_CVT_NUMBER;
      if ( fsize == 4 )
        val.f32 = val.f64;
      fptr = (uint8_t *) (void *) &val;
      fendian = md_endian;
      break;

    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      return this->append_time( fid, ftype, fsize, time );
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      return this->append_date( fid, ftype, fsize, date );
    }
    case MD_DECIMAL: {
      MDDecimal dec;
      dec.get_decimal( mref );
      return this->append_decimal( fid, ftype, fsize, dec );
    }

    case MD_PARTIAL:
      slen = mref.fsize;
      if ( slen > fsize )
        slen = fsize;
      if ( mref.ftype == MD_PARTIAL && mref.fentrysz != 0 )
        return this->pack_partial( fid, fptr, slen, mref.fentrysz );
      /* FALLTHRU */
    case MD_OPAQUE:
    case MD_STRING: {
      if ( mref.ftype == MD_STRING || mref.ftype == MD_PARTIAL ||
           mref.ftype == MD_OPAQUE )
        slen = mref.fsize;
      else {
        slen = sizeof( str_buf );
        if ( to_string( mref, str_buf, slen ) != 0 )
          return Err::BAD_CVT_STRING;
        fptr = (uint8_t *) (void *) str_buf;
      }
      if ( slen < fsize )
        fsize = slen;
      break;
    }
    default:
      return Err::BAD_CVT_NUMBER;
  }

  len = rwf_pack_size( fsize );
  if ( ! this->has_space( len ) )
    return Err::NO_SPACE;
  this->off += len;
  this->nflds++;

  ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
  ptr[ 1 ] = fid & 0xffU;
  size_t n = 2 + pack_rwf_size( &ptr[ 2 ], fsize );
  /* invert endian, for little -> big */
  if ( fendian != MD_BIG && fsize > 1 &&
       ( ftype == MD_UINT || ftype == MD_INT || ftype == MD_REAL ) ) {
    copy_rwf_int_val( fptr, &ptr[ n ], fsize );
  }
  else {
    ::memcpy( &ptr[ n ], fptr, fsize );
  }
  return 0;
}

int
RwfMsgWriter::append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                              MDDecimal &dec )
{
  MDReference mref;

  if ( ftype == MD_DECIMAL ) {
    uint8_t * ptr = &this->buf[ this->off ];
    const uint8_t *ival = (uint8_t *) (void *) &dec.ival;
    size_t ilen = get_rwf_int_len( ival, 8 );

    if ( ! this->has_space( ilen + 4 ) )
      return Err::NO_SPACE;
    this->off += ilen + 4;
    this->nflds++;

    ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
    ptr[ 1 ] = fid & 0xffU;
    ptr[ 2 ] = ilen + 1;
    ptr[ 3 ] = md_to_rwf_decimal_hint( dec.hint );
    copy_rwf_int_val( ival, &ptr[ 4 ], ilen );

    return 0;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    mref.fsize   = dec.get_string( sbuf, sizeof( sbuf ) );
    mref.fptr    = (uint8_t *) sbuf;
    mref.ftype   = MD_STRING;
    mref.fendian = MD_BIG;
    return this->append_ref( fid, ftype, fsize, mref );
  }
  if ( ftype == MD_REAL ) {
    double fval;
    if ( dec.get_real( fval ) == 0 ) {
      mref.fsize   = sizeof( double );
      mref.fptr    = (uint8_t *) (void *) &fval;
      mref.ftype   = MD_REAL;
      mref.fendian = md_endian;
      return this->append_ref( fid, ftype, fsize, mref );
    }
  }
  return Err::BAD_CVT_NUMBER;
}

int
RwfMsgWriter::append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDTime &time )
{
  MDReference mref;

  if ( ftype == MD_TIME ) {
    uint8_t * ptr = &this->buf[ this->off ];

    if ( time.resolution == MD_RES_MINUTES ) {
      if ( ! this->has_space( 5 ) )
        return Err::NO_SPACE;
      this->off += 5;
      this->nflds++;

      ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
      ptr[ 1 ] = fid & 0xffU;
      ptr[ 2 ] = 2;
      ptr[ 3 ] = time.hour;
      ptr[ 4 ] = time.min;
    }
    else {
      if ( ! this->has_space( 6 ) )
        return Err::NO_SPACE;
      this->off += 6;
      this->nflds++;

      ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
      ptr[ 1 ] = fid & 0xffU;
      ptr[ 2 ] = 3;
      ptr[ 3 ] = time.hour;
      ptr[ 4 ] = time.min;
      ptr[ 5 ] = time.sec;
    }

    return 0;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    mref.fptr    = (uint8_t *) sbuf;
    mref.fsize   = time.get_string( sbuf, sizeof( sbuf ) );
    mref.ftype   = MD_STRING;
    mref.fendian = MD_BIG;
    return this->append_ref( fid, ftype, fsize, mref );
  }
  return Err::BAD_TIME;
}

int
RwfMsgWriter::append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDDate &date )
{
  MDReference mref;
  
  if ( ftype == MD_DATE ) {
    uint8_t * ptr = &this->buf[ this->off ];
    if ( ! this->has_space( 7 ) )
      return Err::NO_SPACE;
    this->off += 7;
    this->nflds++;

    ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
    ptr[ 1 ] = fid & 0xffU;
    ptr[ 2 ] = 4;
    ptr[ 3 ] = date.day;
    ptr[ 4 ] = date.mon;
    ptr[ 5 ] = ( date.year >> 8 ) & 0xffU;
    ptr[ 6 ] = date.year & 0xffU;

    return 0;
  }
  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    mref.fptr    = (uint8_t *) sbuf;
    mref.fsize   = date.get_string( sbuf, sizeof( sbuf ) );
    mref.ftype   = MD_STRING;
    mref.fendian = MD_BIG;
    return this->append_ref( fid, ftype, fsize, mref );
  }
  return Err::BAD_DATE;
}

int
RwfMsgWriter::append_decimal( MDFid fid,  MDDecimal &dec )
{
  const char * fname;
  uint8_t      fname_len;
  uint32_t     fsize;
  MDType       ftype;

  if ( ! this->dict->lookup( fid, ftype, fsize, fname_len, fname ) )
    return Err::UNKNOWN_FID;
  return this->append_decimal( fid, ftype, fsize, dec );
}

int
RwfMsgWriter::append_time( MDFid fid,  MDTime &time )
{
  const char * fname;
  uint8_t      fname_len;
  uint32_t     fsize;
  MDType       ftype;

  if ( ! this->dict->lookup( fid, ftype, fsize, fname_len, fname ) )
    return Err::UNKNOWN_FID;
  return this->append_time( fid, ftype, fsize, time );
}

int
RwfMsgWriter::append_date( MDFid fid,  MDDate &date )
{
  const char * fname;
  uint8_t      fname_len;
  uint32_t     fsize;
  MDType       ftype;

  if ( ! this->dict->lookup( fid, ftype, fsize, fname_len, fname ) )
    return Err::UNKNOWN_FID;
  return this->append_date( fid, ftype, fsize, date );
}

int
RwfMsgWriter::append_decimal( const char *fname,  size_t fname_len,
                              MDDecimal &dec )
{
  uint32_t fsize;
  MDType   ftype;
  MDFid    fid;

  if ( ! this->dict->get( fname, fname_len, fid, ftype, fsize ) )
    return Err::UNKNOWN_FID;
  return this->append_decimal( fid, ftype, fsize, dec );
}

int
RwfMsgWriter::append_time( const char *fname,  size_t fname_len,
                           MDTime &time )
{
  uint32_t fsize;
  MDType   ftype;
  MDFid    fid;

  if ( ! this->dict->get( fname, fname_len, fid, ftype, fsize ) )
    return Err::UNKNOWN_FID;
  return this->append_time( fid, ftype, fsize, time );
}

int
RwfMsgWriter::append_date( const char *fname,  size_t fname_len,
                           MDDate &date )
{
  uint32_t fsize;
  MDType   ftype;
  MDFid    fid;

  if ( ! this->dict->get( fname, fname_len, fid, ftype, fsize ) )
    return Err::UNKNOWN_FID;
  return this->append_date( fid, ftype, fsize, date );
}

