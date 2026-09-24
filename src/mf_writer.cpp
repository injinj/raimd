#include <stdio.h>
#include <raimd/mf_msg.h>
#include <raimd/md_dict.h>
#include <raimd/md_int.h>
#include <raimd/app_a.h>
#include <raimd/sass.h>

using namespace rai;
using namespace md;

namespace {
enum {
  FS_ = 0x1cU, /* <FS>..msg..<FS> */
  GS_ = 0x1dU, /* <GS>ric */
  RS_ = 0x1eU, /* <RS>fid<US>val */
  US_ = 0x1fU
};
/* decimal digits of an unsigned */
static inline size_t
udigs( uint64_t v ) {
  size_t n = 1;
  while ( v >= 10 ) { v /= 10; n++; }
  return n;
}
static inline size_t
put_uint( char *p,  uint64_t v ) {
  size_t n = udigs( v );
  for ( size_t i = n; i > 0; ) {
    p[ --i ] = (char) ( '0' + ( v % 10 ) );
    v /= 10;
  }
  return n;
}
static inline size_t
put_int( char *p,  int64_t v ) {
  if ( v < 0 ) {
    p[ 0 ] = '-';
    return 1 + put_uint( &p[ 1 ], (uint64_t) -v );
  }
  return put_uint( p, (uint64_t) v );
}
}

extern "C" {
MDMsgWriter_t *
mf_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,  void *buf_ptr,
                      size_t buf_sz )
{
  void * p = static_cast<MDMsgMem *>( mem )->make( sizeof( MktfdMsgWriter ) );
  return p == NULL ? 0 :
    new ( p ) MktfdMsgWriter( *static_cast<MDMsgMem *>( mem ),
                              static_cast<MDDict *>( d ), buf_ptr, buf_sz );
}
}

int
MktfdMsg::create_writer( MDMsgWriterBase *&wr,  MDMsgMem &mem,  MDDict *d,
                         void *bb,  size_t len ) noexcept
{
  wr = static_cast<MDMsgWriterBase *>( mf_msg_writer_create( &mem, d, bb, len ) );
  return wr ? 0 : Err::ALLOC_FAIL;
}

MktfdMsgWriter::MktfdMsgWriter( MDMsgMem &m,  MDDict *d,  void *bb,
                                size_t len ) noexcept
{
  this->msg_mem = &m;
  this->buf     = (uint8_t *) bb;
  this->off     = 0;
  this->buflen  = len;
  this->wr_type = MARKETFEED_TYPE_ID;
  this->err     = 0;
  this->dict    = NULL;
  this->hdr_off = 0;
  this->func    = 0;
  this->in_hdr  = false;
  this->closed  = false;
  /* prefer the app_a (RDM) dictionary: it carries the MF type and length;
   * a cfile dictionary works for fids/types without the MF lengths */
  for ( MDDict *p = d; p != NULL; p = p->get_next() ) {
    if ( p->dict_type[ 0 ] == 'a' ) {
      this->dict = p;
      break;
    }
  }
  if ( this->dict == NULL )
    this->dict = d;
}

bool
MktfdMsgWriter::resize( size_t len ) noexcept
{
  static const size_t max_size = 0x3fffffff;
  if ( this->err != 0 )
    return false;
  size_t old_len = this->buflen,
         new_len = this->off + len + 1;
  if ( new_len > max_size )
    return false;
  if ( new_len < old_len * 2 )
    new_len = old_len * 2;
  else
    new_len += 1024;
  if ( new_len > max_size )
    new_len = max_size;
  uint8_t * new_buf = this->buf;
  this->mem().extend( old_len, new_len, &new_buf );
  this->buf    = new_buf;
  this->buflen = new_len;
  return this->off + len + 1 <= this->buflen;
}

MktfdMsgWriter &
MktfdMsgWriter::start( uint16_t fn,  const char *ric,  size_t riclen,
                       const char *tag,  size_t taglen ) noexcept
{
  this->off    = 0;
  this->closed = false;
  this->func   = fn;
  if ( ! this->has_space( 1 + 5 + 1 + taglen + 1 + riclen ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  size_t n = 0;
  p[ n++ ] = FS_;
  n += put_uint( &p[ n ], fn );
  if ( fn != AggregateUpd_350 ) { /* 350 has no tag */
    p[ n++ ] = US_;
    ::memcpy( &p[ n ], tag, taglen );
    n += taglen;
  }
  p[ n++ ] = GS_;
  ::memcpy( &p[ n ], ric, riclen );
  n += riclen;
  this->off   += n;
  this->in_hdr = true;
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::add_rstatus( int32_t rstatus ) noexcept
{
  if ( ! this->in_hdr )
    return this->error( Err::INVALID_MSG );
  if ( ! this->has_space( 1 + 11 ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  p[ 0 ] = RS_;
  this->off += 1 + put_int( &p[ 1 ], rstatus );
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::add_flist( uint16_t flist ) noexcept
{
  if ( ! this->in_hdr )
    return this->error( Err::INVALID_MSG );
  if ( ! this->has_space( 1 + 5 ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  p[ 0 ] = US_;
  this->off += 1 + put_uint( &p[ 1 ], flist );
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::add_rtl( uint32_t rtl ) noexcept
{
  if ( ! this->in_hdr )
    return this->error( Err::INVALID_MSG );
  if ( ! this->has_space( 1 + 10 ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  p[ 0 ] = US_;
  this->off += 1 + put_uint( &p[ 1 ], rtl );
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::add_rtl( void ) noexcept
{
  if ( ! this->in_hdr )
    return this->error( Err::INVALID_MSG );
  if ( ! this->has_space( 1 ) )
    return this->error( Err::NO_SPACE );
  this->buf[ this->off++ ] = US_;
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::add_status( int32_t status,  const char *text,
                            size_t textlen ) noexcept
{
  if ( ! this->in_hdr )
    return this->error( Err::INVALID_MSG );
  if ( ! this->has_space( 1 + 11 + 1 + textlen ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  size_t n = 0;
  p[ n++ ] = GS_;
  n += put_int( &p[ n ], status );
  if ( text != NULL && textlen > 0 ) {
    p[ n++ ] = RS_;
    ::memcpy( &p[ n ], text, textlen );
    n += textlen;
  }
  this->off += n;
  this->hdr_off = this->off;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::append_raw( MDFid fid,  const char *val,
                            size_t vallen ) noexcept
{
  if ( this->closed )
    return this->error( Err::INVALID_MSG );
  if ( this->in_hdr )
    this->end_hdr();
  if ( ! this->has_space( 1 + 11 + 1 + vallen ) )
    return this->error( Err::NO_SPACE );
  char * p = (char *) &this->buf[ this->off ];
  size_t n = 0;
  p[ n++ ] = RS_;
  n += put_int( &p[ n ], fid );
  p[ n++ ] = US_;
  ::memcpy( &p[ n ], val, vallen );
  n += vallen;
  this->off += n;
  return *this;
}

MktfdMsgWriter &
MktfdMsgWriter::append_ref( MDFid fid,  MDReference &mref ) noexcept
{
  MDLookup by( fid );
  if ( this->dict == NULL || ! this->dict->lookup( by ) ) {
    by.ftype = mref.ftype; /* no dictionary: format from the reference */
    by.fsize = mref.fsize;
    by.mflen = 0;
    by.flags = 0;
  }
  return this->append_ref( by, mref );
}

MktfdMsgWriter &
MktfdMsgWriter::append_ref( const char *fname,  size_t fname_len,
                            MDReference &mref ) noexcept
{
  MDLookup by( fname, fname_len );
  if ( this->dict == NULL || ! this->dict->get( by ) )
    return this->error( Err::UNKNOWN_FID );
  return this->append_ref( by, mref );
}

/* format one value the way the ADS does for SSL sinks, then append */
MktfdMsgWriter &
MktfdMsgWriter::append_ref( MDLookup &by,  MDReference &mref ) noexcept
{
  char     tmp[ 256 ];
  size_t   n = 0;
  uint8_t  mf_type = MF_NONE;
  uint32_t mf_len  = 0,
           enum_len = 0;
  MDType   ftype = by.ftype;
  int      status;

  if ( by.mflen != 0 || by.fsize != 0 )
    by.mf_type( mf_type, mf_len, enum_len );

  switch ( ftype ) {
    case MD_DECIMAL:
    case MD_REAL:
    case MD_INT:
    case MD_UINT:
    case MD_BOOLEAN: {
      MDDecimal dec;
      if ( ftype == MD_DECIMAL || ftype == MD_REAL ) {
        if ( (status = dec.get_decimal( mref )) != 0 )
          return this->error( status );
      }
      else if ( ftype == MD_INT ) {
        int64_t iv;
        if ( cvt_number<int64_t>( mref, iv ) != 0 )
          return this->error( Err::BAD_CVT_NUMBER );
        dec.set( iv, MD_DEC_INTEGER );
      }
      else {
        uint64_t uv;
        if ( cvt_number<uint64_t>( mref, uv ) != 0 )
          return this->error( Err::BAD_CVT_NUMBER );
        dec.set( (int64_t) uv, MD_DEC_INTEGER );
      }
      /* MF INTEGER/PRICE from a REAL64 field carry an explicit sign;
       * MF INTEGER from a UINT64 field (RDNDISPLAY, PROD_PERM) and
       * enumerations do not */
      bool signed_fmt = ( ftype == MD_DECIMAL || ftype == MD_REAL ||
                          ftype == MD_INT );
      if ( mf_type == MF_ENUMERATED || mf_type == MF_BINARY )
        signed_fmt = false;
      if ( dec.hint == MD_DEC_NULL ) {
        n = 0; /* blank field */
      }
      else {
        if ( signed_fmt && dec.ival >= 0 && dec.hint != MD_DEC_NAN &&
             dec.hint != MD_DEC_INF )
          tmp[ n++ ] = '+';
        /* fractions as "64 4/8", decimals as "0.20" */
        n += dec.get_string( &tmp[ n ], sizeof( tmp ) - n, false );
      }
      break;
    }
    case MD_ENUM: {
      /* rv / tib sources carry enums as their display string: map back
       * to the value through enumtype.def, as RwfFieldListWriter does */
      uint64_t uval = 0;
      if ( mref.ftype == MD_STRING ) {
        uint16_t ev = 0;
        size_t   slen = mref.fsize;
        if ( slen > 0 && ((const char *) mref.fptr)[ slen - 1 ] == '\0' )
          slen--;
        if ( this->dict != NULL &&
             this->dict->get_enum_val( by.fid, (const char *) mref.fptr,
                                       slen, ev ) )
          uval = ev;
        else if ( cvt_number<uint64_t>( mref, uval ) != 0 )
          return this->error( Err::BAD_CVT_NUMBER );
      }
      else if ( cvt_number<uint64_t>( mref, uval ) != 0 )
        return this->error( Err::BAD_CVT_NUMBER );
      n = put_uint( tmp, uval );
      break;
    }
    case MD_TIME: {
      MDTime time;
      if ( (status = time.get_time( mref )) != 0 )
        return this->error( status );
      /* dictionary TIME length 5 -> HH:MM, 8 (TIME_SECONDS) -> HH:MM:SS */
      if ( mf_type == MF_TIME || ( mf_type == MF_NONE && mf_len == 0 &&
                                   time.res() == MD_RES_MINUTES ) ) {
        if ( mf_len == 5 || mf_type == MF_TIME )
          time.resolution = MD_RES_MINUTES | ( time.resolution & MD_RES_NULL );
      }
      else if ( mf_type == MF_TIME_SECONDS || mf_len >= 8 ) {
        if ( time.res() == MD_RES_MINUTES )
          time.resolution = MD_RES_SECONDS | ( time.resolution & MD_RES_NULL );
        else if ( time.res() != MD_RES_SECONDS )
          time.resolution = MD_RES_SECONDS | ( time.resolution & MD_RES_NULL );
      }
      n = time.get_string( tmp, sizeof( tmp ) );
      break;
    }
    case MD_DATE: {
      MDDate date;
      if ( (status = date.get_date( mref )) != 0 )
        return this->error( status );
      n = date.get_string( tmp, sizeof( tmp ), MD_DATE_FMT_default );
      break;
    }
    case MD_STRING:
    case MD_PARTIAL:
    case MD_OPAQUE:
    default: {
      /* strings: as is, space padded to the MF length when known */
      const char * s = (const char *) mref.fptr;
      size_t       slen = mref.fsize;
      if ( ftype == MD_STRING && slen > 0 && s[ slen - 1 ] == '\0' )
        slen--;
      if ( mf_type == MF_ALPHANUMERIC && mf_len > 0 && slen < mf_len &&
           mf_len <= 255 ) {
        n = mf_len;
        if ( ! this->has_space( 1 + 11 + 1 + n ) )
          return this->error( Err::NO_SPACE );
        ::memcpy( tmp, s, slen );
        ::memset( &tmp[ slen ], ' ', n - slen );
        return this->append_raw( by.fid, tmp, n );
      }
      return this->append_raw( by.fid, s, slen );
    }
  }
  return this->append_raw( by.fid, tmp, n );
}

/* a fid from the source message can be written as is only when the
 * source was decoded with the same dictionary this writer encodes with:
 * a cfile fid is a different numbering from an RDM fid, and two RDM
 * dictionaries from different domains can disagree too.  The message's
 * dictionary may be a list (cfile, app_a, ...); match any member by
 * identity or content hash.  Otherwise go by name. */
static inline bool
same_dict( MDDict *wr,  MDDict *msg_dict ) noexcept
{
  if ( wr == NULL )
    return false;
  for ( MDDict *d = msg_dict; d != NULL; d = d->get_next() ) {
    if ( d == wr || ( d->dict_hash_id != 0 &&
                      d->dict_hash_id == wr->dict_hash_id ) )
      return true;
  }
  return false;
}

int
MktfdMsgWriter::append_iter( MDFieldIter *iter ) noexcept
{
  MDName      n;
  MDReference mref;
  int status;
  if ( (status = iter->get_name( n )) != 0 ||
       (status = iter->get_reference( mref )) != 0 )
    return status;
  if ( n.fid != 0 && same_dict( this->dict, iter->iter_msg().dict ) )
    this->append_ref( n.fid, mref );
  else if ( n.fname != NULL )
    this->append_ref( n.fname, n.fnamelen, mref );
  else if ( n.fid != 0 )
    this->append_ref( n.fid, mref );
  return this->err;
}

int
MktfdMsgWriter::convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept
{
  MDFieldIter *iter;
  int  status;
  bool use_fid = same_dict( this->dict, msg.dict );
  if ( (status = msg.get_field_iter( iter )) == 0 ) {
    if ( (status = iter->first()) == 0 ) {
      do {
        MDName      n;
        MDReference mref;
        if ( (status = iter->get_name( n )) == 0 &&
             (status = iter->get_reference( mref )) == 0 ) {
          if ( skip_hdr && is_sass_hdr( n ) )
            continue;
          if ( n.fid != 0 && ( use_fid || n.fname == NULL ) )
            this->append_ref( n.fid, mref );
          else
            this->append_ref( n.fname, n.fnamelen, mref );
          status = this->err;
        }
        if ( status != 0 )
          break;
      } while ( (status = iter->next()) == 0 );
    }
  }
  if ( status != Err::NOT_FOUND )
    return status;
  return 0;
}

int
MktfdMsgWriter::append_sass_hdr( MDFormClass *form, uint16_t msg_type,
                                 uint16_t rec_type, uint16_t seqno,
                                 uint16_t status, const char *subj,
                                 size_t sublen ) noexcept
{
  rai::md::append_sass_hdr( *this, form, msg_type, rec_type, seqno, status,
                            subj, sublen );
  return this->err;
}

size_t
MktfdMsgWriter::update_hdr( void ) noexcept
{
  if ( this->err != 0 )
    return 0;
  if ( this->in_hdr )
    this->end_hdr();
  if ( ! this->closed ) {
    if ( ! this->has_space( 1 ) ) {
      this->error( Err::NO_SPACE );
      return 0;
    }
    this->buf[ this->off++ ] = FS_;
    this->closed = true;
  }
  return this->off;
}
