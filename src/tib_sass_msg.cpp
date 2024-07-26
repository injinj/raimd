#include <stdio.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/md_dict.h>
#include <raimd/sass.h>

using namespace rai;
using namespace md;

static const char TibSassMsg_proto_string[] = "TIB_QFORM";
const char *
TibSassMsg::get_proto_string( void ) noexcept
{
  return TibSassMsg_proto_string;
}

uint32_t
TibSassMsg::get_type_id( void ) noexcept
{
  return TIB_SASS_TYPE_ID;
}

static MDMatch tibsassmsg_match = {
  .off         = 0,
  .len         = 4, /* cnt of buf[] */
  .hint_size   = 2, /* cnt of hint[] */
  .ftype       = (uint8_t) TIB_SASS_TYPE_ID,
  .buf         = { 0x11, 0x11, 0x11, 0x12 },
  .hint        = { TIB_SASS_TYPE_ID, 0xa08b0040 },
  .is_msg_type = TibSassMsg::is_tibsassmsg,
  .unpack      = (md_msg_unpack_f) TibSassMsg::unpack,
  .name        = TibSassMsg_proto_string
};

bool
TibSassMsg::is_tibsassmsg( void *bb,  size_t off,  size_t end,
                           uint32_t ) noexcept
{
  uint32_t magic = 0;
  if ( off + 8 <= end )
    magic = get_u32<MD_BIG>( &((uint8_t *) bb)[ off ] );
  return magic == 0x11111112U;
}

TibSassMsg *
TibSassMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,  MDDict *d,
                    MDMsgMem &m ) noexcept
{
  if ( off + 8 > end )
    return NULL;
  uint32_t magic    = get_u32<MD_BIG>( &((uint8_t *) bb)[ off ] );
  size_t   msg_size = get_u32<MD_BIG>( &((uint8_t *) bb)[ off + 4 ] );

  if ( magic != 0x11111112U )
    return NULL;
  if ( off + msg_size + 8 > end )
    return NULL;
  void * ptr;
  m.incr_ref();
  m.alloc( sizeof( TibSassMsg ), &ptr );
  for ( ; d != NULL; d = d->next )
    if ( d->dict_type[ 0 ] == 'c' ) /* need cfile type */
      break;
  return new ( ptr ) TibSassMsg( bb, off, off + msg_size + 8, d, m );
}

void
TibSassMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( tibsassmsg_match );
}

int
TibSassMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( TibSassFieldIter ), &ptr );
  iter = new ( ptr ) TibSassFieldIter( *this );
  return 0;
}

MDFieldIter *
TibSassFieldIter::copy( void ) noexcept
{
  void * ptr;
  TibSassFieldIter *iter;
  this->iter_msg.mem->alloc( sizeof( TibSassFieldIter ), &ptr );
  iter = new ( ptr ) TibSassFieldIter( this->iter_msg );
  this->dup_sass( *iter );
  return iter;
}

int
TibSassFieldIter::get_name( MDName &name ) noexcept
{
  name.fid      = this->fid;
  name.fnamelen = this->fnamelen;
  name.fname    = this->fname;
  return 0;
}

int
TibSassFieldIter::get_reference( MDReference &mref ) noexcept
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;

  mref.ftype    = this->ftype;
  mref.fendian  = MD_BIG;
  mref.fentrytp = MD_NODATA;
  mref.fentrysz = 0;

  if ( this->ftype != MD_PARTIAL && ( this->flags & MD_FIXED ) != 0 ) {
    mref.fsize = this->fsize;
    mref.fptr  = &buf[ this->field_start + 2 ];

    if ( this->ftype == MD_DECIMAL ) {
      MDReference dref = mref;
      dref.fsize = ( mref.fsize >= 8 ? 8 : 4 );
      dref.ftype = MD_REAL;
      double    f         = get_float<double>( dref );
      double    denom     = 0;
      uint32_t  precision = 256;
      uint32_t  hint      = dref.fptr[ dref.fsize ];

      mref.fptr    = (uint8_t *) (void *) &this->dec;
      mref.fsize   = sizeof( this->dec );
      mref.fendian = md_endian;

      switch ( hint ) {
        case TSS_HINT_BLANK_VALUE: precision = 0; this->dec.hint = MD_DEC_NULL;
                                   break;
        case TSS_HINT_NONE:      break;
        case TSS_HINT_DENOM_2:   denom = 2;   this->dec.hint = MD_DEC_FRAC_2;
                                 break;
        case TSS_HINT_DENOM_4:   denom = 4;   this->dec.hint = MD_DEC_FRAC_4;
                                 break;
        case TSS_HINT_DENOM_8:   denom = 8;   this->dec.hint = MD_DEC_FRAC_8;
                                 break;
        case TSS_HINT_DENOM_16:  denom = 16;  this->dec.hint = MD_DEC_FRAC_16;
                                 break;
        case TSS_HINT_DENOM_32:  denom = 32;  this->dec.hint = MD_DEC_FRAC_32;
                                 break;
        case TSS_HINT_DENOM_64:  denom = 64;  this->dec.hint = MD_DEC_FRAC_64;
                                 break;
        case TSS_HINT_DENOM_128: denom = 128; this->dec.hint = MD_DEC_FRAC_128;
                                 break;
        case TSS_HINT_DENOM_256: denom = 256; this->dec.hint = MD_DEC_FRAC_256;
                                 break;
        case TSS_HINT_PRECISION_1: case TSS_HINT_PRECISION_2:
        case TSS_HINT_PRECISION_3: case TSS_HINT_PRECISION_4:
        case TSS_HINT_PRECISION_5: case TSS_HINT_PRECISION_6:
        case TSS_HINT_PRECISION_7: case TSS_HINT_PRECISION_8:
        case TSS_HINT_PRECISION_9:
        default:
          if ( hint >= 16 && hint < 32 )
            precision = hint - 16;
          break;
      }
      if ( denom != 0 )
        this->dec.ival = (int64_t) ( f * denom );
      else if ( precision != 256 ) {
        if ( precision == 0 )
          this->dec.ival = (int64_t) f;
        else {
          static const double dec_powers_f[] = MD_DECIMAL_POWERS;
          size_t n = sizeof( dec_powers_f ) / sizeof( dec_powers_f[ 0 ] ) - 1;
          double p;
          if ( precision <= n ) {
            p = dec_powers_f[ precision ];
          }
          else {
            p = dec_powers_f[ n ];
            while ( n < precision ) {
              p *= 10;
              n++;
            }
          }
          this->dec.hint = -10 - (int8_t) precision;
          this->dec.ival = (int64_t) ( f * p );
        }
      }
      else {
        this->dec.set_real( f ); /* determine the hint, NONE is ambiguous */
      }
    }
    else if ( this->ftype == MD_TIME ) {
      if ( this->time.parse( (char *) mref.fptr, mref.fsize ) == 0 ) {
        mref.fendian = md_endian;
        mref.fptr    = (uint8_t *) (void *) &this->time;
        mref.fsize   = sizeof( this->time );
      }
      else {
        mref.ftype = MD_STRING;
      }
    }
    else if ( this->ftype == MD_DATE ) {
      if ( this->date.parse( (char *) mref.fptr, mref.fsize ) == 0 ) {
        mref.fendian = md_endian;
        mref.fptr    = (uint8_t *) (void *) &this->date;
        mref.fsize   = sizeof( this->date );
      }
      else {
        mref.ftype = MD_STRING;
      }
    }
  }
  else if ( this->ftype == MD_PARTIAL ) {
    mref.fentrysz = (uint8_t)
                    get_u16<MD_BIG>( &buf[ this->field_start + 2 ] ); /* off */
    mref.fsize    = get_u16<MD_BIG>( &buf[ this->field_start + 4 ] ); /* len */
    mref.fptr     = &buf[ this->field_start + 6 ];                   /* data */
  }
  else if ( this->fsize <= 0xffffU ) {
    mref.fsize    = get_u16<MD_BIG>( &buf[ this->field_start + 2 ] ); /* len */
    mref.fptr     = &buf[ this->field_start + 4 ];                   /* data */
  }
  else {
    mref.fsize    = get_u32<MD_BIG>( &buf[ this->field_start + 2 ] ); /* len */
    mref.fptr     = &buf[ this->field_start + 6 ];                   /* data */
  }
  return 0;
}

int
TibSassFieldIter::get_hint_reference( MDReference &mref ) noexcept
{
  mref.fentrytp = MD_NODATA;
  mref.fentrysz = 0;
  mref.fendian  = md_endian;
  switch ( this->ftype ) {
    case MD_DATE: {
      static uint16_t sdate_hint = 257;
      mref.fsize = 2;
      mref.ftype = MD_UINT;
      mref.fptr  = (uint8_t *) (void *) &sdate_hint;
      break;
    }
    case MD_TIME: {
      static uint16_t stime_hint = 256;
      mref.fsize = 2;
      mref.ftype = MD_UINT;
      mref.fptr  = (uint8_t *) (void *) &stime_hint;
      break;
    }
    case MD_DECIMAL: {
      uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
      mref.fsize = 1;
      mref.ftype = MD_UINT;
      mref.fptr  = &buf[ this->field_start + 2 + this->fsize - 1 ];
      break;
    }
    case MD_REAL: {
      static uint16_t price_hint = 0;
      mref.fsize = 2;
      mref.ftype = MD_UINT;
      mref.fptr  = (uint8_t *) (void *) &price_hint;
      break;
    }
    /*case MD_PARTIAL: {
      uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
      mref.fsize   = 2;
      mref.ftype   = MD_UINT;
      mref.fptr    = &buf[ this->field_start + 2 ];
      mref.fendian = MD_BIG;
      break;
    }*/
    default:
      mref.zero();
      return Err::NOT_FOUND;
  }
  return 0;
}

int
TibSassFieldIter::find( const char *name,  size_t name_len,
                        MDReference &mref ) noexcept
{
  if ( this->iter_msg.dict == NULL )
    return Err::NO_DICTIONARY;

  int status = Err::NOT_FOUND;
  if ( name != NULL ) {
    MDLookup by( name, name_len );
    if ( this->iter_msg.dict->get( by ) ) {
      if ( (status = this->first()) == 0 ) {
        do {
          if ( this->fid == by.fid )
            return this->get_reference( mref );
        } while ( (status = this->next()) == 0 );
      }
    }
  }
  return status;
}

int
TibSassFieldIter::first( void ) noexcept
{
  int x;
  this->field_start = this->iter_msg.msg_off + 8;
  this->field_end   = this->iter_msg.msg_end;
  this->field_index = 0;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  if ( (x = this->unpack()) == Err::NULL_FID ) {
    if ( this->field_start + 2 == this->field_end )
      return Err::NOT_FOUND;
  }
  return x;
}

int
TibSassFieldIter::next( void ) noexcept
{
  int x;
  this->field_start = this->field_end;
  this->field_end   = this->iter_msg.msg_end;
  this->field_index++;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  if ( (x = this->unpack()) == Err::NULL_FID ) {
    if ( this->field_start + 2 == this->field_end )
      return Err::NOT_FOUND;
  }
  return x;
}

int
TibSassFieldIter::unpack( void ) noexcept
{
  const uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  size_t          i   = this->field_start;

  if ( i + 2 > this->field_end )
    return Err::BAD_FIELD_BOUNDS;

  this->fid = get_u16<MD_BIG>( &buf[ i ] ) & 0x3fffU;
  if ( this->iter_msg.dict == NULL )
    return Err::NO_DICTIONARY;
  MDLookup by( this->fid );
  if ( ! this->iter_msg.dict->lookup( by ) ) {
    if ( this->fid == 0 )
      return Err::NULL_FID;
    return Err::UNKNOWN_FID;
  }
  this->ftype    = by.ftype;
  this->fsize    = by.fsize;
  this->flags    = by.flags;
  this->fnamelen = by.fname_len;
  this->fname    = by.fname;
  if ( this->ftype != MD_PARTIAL && ( this->flags & MD_FIXED ) != 0 ) {
    i += this->pack_size();
  }
  else if ( this->ftype == MD_PARTIAL ) {
    if ( i + 6 > this->field_end )
      return Err::BAD_FIELD_BOUNDS;
    i += this->partial_pack_size( get_u16<MD_BIG>( &buf[ i + 4 ] ) );
  }
  else if ( this->fsize <= 0xffffU ) {
    i += this->variable_pack_size_2( get_u16<MD_BIG>( &buf[ i + 2 ] ) );
  }
  else {
    i += this->variable_pack_size_4( get_u32<MD_BIG>( &buf[ i + 2 ] ) );
  }

  if ( i > this->field_end )
    return Err::BAD_FIELD_BOUNDS;
  this->field_end = i;
  return 0;
}

TibSassMsgWriter::TibSassMsgWriter( MDMsgMem &m,  MDDict *d,  void *bb,
                                    size_t len ) noexcept
    : mem( m ), dict( d ), form( NULL ), buf( (uint8_t *) bb ), off( 0 ),
      buflen( len ), err( 0 ), unk_fid( 0 ), use_form( false )
{
  for ( ; d != NULL; d = d->next ) {
    if ( d->dict_type[ 0 ] == 'c' ) { /* look for cfile type */
      this->dict = d;
      break;
    }
  }
}

TibSassMsgWriter::TibSassMsgWriter( MDMsgMem &m,  MDFormClass &f,  void *bb,
                                    size_t len ) noexcept
    : mem( m ), dict( &f.dict ), form( &f ), buf( (uint8_t *) bb ), off( 0 ),
      buflen( len ), err( 0 ), unk_fid( 0 ), use_form( false )
{
}

bool
TibSassMsgWriter::resize( size_t len ) noexcept
{
  static const size_t max_size = 0x3fffffff; /* 1 << 30 - 1 == 1073741823 */
  if ( this->err != 0 )
    return false;
  size_t old_len = this->buflen,
         new_len = this->buflen + ( len - this->off ) + 8;
  if ( new_len > max_size )
    return false;
  if ( new_len < old_len * 2 )
    new_len = old_len * 2;
  else
    new_len += 1024;
  if ( new_len > max_size )
    new_len = max_size;
  uint8_t * new_buf = this->buf;
  this->mem.extend( old_len, new_len, &new_buf );
  this->buf    = new_buf;
  this->buflen = new_len;
  return this->off + 8 + len <= this->buflen;
}

TibSassMsgWriter &
TibSassMsgWriter::append_form_record( void ) noexcept
{
  if ( this->form == NULL )
    return this->error( Err::NO_FORM );
  if ( this->off < this->form->form_size &&
       ! this->has_space( this->form->form_size - this->off ) )
    return this->error( Err::NO_SPACE );

  uint8_t * ptr = &this->buf[ 8 ];
  for ( uint32_t i = 0; i < this->form->nentries; i++ ) {
    MDFormEntry & entry = this->form->entries[ i ];
    if ( entry.foffset >= this->off ) {
      if ( entry.foffset > this->off ) {
        ::memset( &ptr[ this->off ], 0, entry.foffset - this->off );
        this->off = entry.foffset;
      }
      uint16_t fid = entry.fid;
      fid |= ( (uint16_t) (MD_FIXED | MD_PRIMITIVE) << 14 );
      ptr[ entry.foffset ]     = (uint8_t) ( ( fid >> 8 ) & 0xffU );
      ptr[ entry.foffset + 1 ] = (uint8_t) ( fid & 0xffU );
      this->off = entry.foffset + 2;
    }
  }
  if ( this->form->form_size > this->off ) {
    ::memset( &ptr[ this->off ], 0, this->form->form_size - this->off );
    this->off = this->form->form_size;
  }
  this->use_form = true;
  return *this;
}

bool
TibSassMsgWriter::lookup( MDLookup &by,  const MDFormEntry *&entry ) noexcept
{
  if ( this->form != NULL ) {
    if ( (entry = this->form->lookup( by )) == NULL )
      return false;
  }
  else {
    entry = NULL;
    if ( ! this->dict->lookup( by ) )
      return false;
  }
  return true;
}

TibSassMsgWriter &
TibSassMsgWriter::append_ref( MDFid fid,  MDReference &mref ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fid );

  if ( ! this->lookup( by, entry ) )
    return this->unknown_fid();
  return this->append_ref( fid, by.ftype, by.fsize, by.flags, mref,
                           entry );
}

bool
TibSassMsgWriter::get( MDLookup &by,  const MDFormEntry *&entry ) noexcept
{
  if ( this->form != NULL ) {
    if ( (entry = this->form->get( by )) == NULL )
      return false;
  }
  else {
    entry = NULL;
    if ( ! this->dict->get( by ) )
      return false;
  }
  return true;
}

TibSassMsgWriter &
TibSassMsgWriter::append_ref( const char *fname,  size_t fname_len,
                              MDReference &mref ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fname, fname_len );

  if ( ! this->get( by, entry ) )
    return this->unknown_fid();
  return this->append_ref( by.fid, by.ftype, by.fsize, by.flags, mref,
                           entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_ref( MDFid fid,  MDType ftype,  uint32_t fsize,
                              uint8_t flags,  MDReference &mref,
                              const MDFormEntry *entry ) noexcept
{
  char      str_buf[ 64 ];
  uint8_t * ptr,
          * fptr = mref.fptr;
  size_t    slen,
            zpad = 0,
            len;
  MDValue   val;
  MDEndian  fendian = mref.fendian;
  int       status;

  len = tib_sass_pack_size( fsize );
  if ( entry != NULL && this->use_form )
    ptr = &this->buf[ entry->foffset + 8 ];
  else {
    if ( ftype != MD_PARTIAL ) {
      if ( ! this->has_space( len ) )
        return this->error( Err::NO_SPACE );
    }
    ptr = &this->buf[ this->off + 8 ];
  }
  switch ( ftype ) {
    case MD_DECIMAL: {
      MDDecimal dec;
      status = dec.get_decimal( mref );
      if ( status != 0 )
        return this->error( status );
      return this->append_decimal( fid, ftype, fsize, dec, entry );
    }
    case MD_TIME:
      if ( mref.ftype != MD_STRING ) {
        MDTime time;
        status = time.get_time( mref );
        if ( status != 0 )
          return this->error( status );
        return this->append_time( fid, ftype, fsize, time, entry );
      }
      break;
    case MD_DATE: {
      if ( mref.ftype != MD_STRING ) {
        MDDate date;
        status = date.get_date( mref );
        if ( status != 0 )
          return this->error( status );
        return this->append_date( fid, ftype, fsize, date, entry );
      }
      break;
    }
    default:
      break;
  }
  if ( ftype != mref.ftype || fsize != mref.fsize ) {
    switch ( ftype ) {
      case MD_UINT:
        switch ( mref.ftype ) {
          case MD_UINT:   val.u64 = get_uint<uint64_t>( mref ); break;
          case MD_INT:    val.u64 = (uint64_t) get_int<int64_t>( mref ); break;
          case MD_REAL:   val.u64 = (uint64_t) get_float<double>( mref ); break;
          case MD_STRING: val.u64 = parse_u64( (char *) fptr, NULL ); break;
          case MD_DECIMAL: {
            MDDecimal dec;
            status = dec.get_decimal( mref );
            if ( status != 0 )
              return this->error( status );
            dec.get_integer( val.i64 );
            break;
          }
          default:        return this->error( Err::BAD_CVT_NUMBER );
        }
        fptr = (uint8_t *) (void *) &val;
        fendian = md_endian;
        break;
      case MD_INT:
        switch ( mref.ftype ) {
          case MD_UINT:   val.i64 = (int64_t) get_uint<uint64_t>( mref ); break;
          case MD_INT:    val.i64 = get_int<int64_t>( mref ); break;
          case MD_REAL:   val.i64 = (int64_t) get_float<double>( mref ); break;
          case MD_STRING: val.i64 = parse_i64( (char *) fptr, NULL ); break;
          case MD_DECIMAL: {
            MDDecimal dec;
            status = dec.get_decimal( mref );
            if ( status != 0 )
              return this->error( status );
            dec.get_integer( val.i64 );
            break;
          }
          default:        return this->error( Err::BAD_CVT_NUMBER );
        }
        fptr = (uint8_t *) (void *) &val;
        fendian = md_endian;
        break;
      case MD_REAL:
        switch ( mref.ftype ) {
          case MD_UINT:   val.f64 = (double) get_uint<uint64_t>( mref ); break;
          case MD_INT:    val.f64 = (double) get_int<int64_t>( mref ); break;
          case MD_REAL:   val.f64 = get_float<double>( mref ); break;
          case MD_STRING: val.f64 = parse_f64( (char *) fptr, NULL ); break;
          case MD_DECIMAL: {
            MDDecimal dec;
            status = dec.get_decimal( mref );
            if ( status != 0 )
              return this->error( status );
            dec.get_real( val.f64 );
            break;
          }
          default:        return this->error( Err::BAD_CVT_NUMBER );
        }
        if ( fsize == 4 )
          val.f32 = (float) val.f64;
        fptr = (uint8_t *) (void *) &val;
        fendian = md_endian;
        if ( fsize > 8 ) { /* for grocery */
          zpad  = fsize - 8;
          fsize = 8;
        }
        break;
      case MD_OPAQUE:
      case MD_STRING:
      case MD_TIME:
      case MD_DATE:
      case MD_PARTIAL: {
        char * sbuf = str_buf;
        size_t sz;
        switch ( mref.ftype ) {
          case MD_UINT:    slen = uint_str( get_uint<uint64_t>( mref ), sbuf );
                           break;
          case MD_INT:     slen = int_str( get_int<int64_t>( mref ), sbuf );
                           break;
          case MD_REAL:    slen = float_str( get_float<double>( mref ), sbuf );
                           break;
          case MD_STRING:
          case MD_OPAQUE:  sbuf = (char *) fptr; slen = mref.fsize;
                           break;
          case MD_PARTIAL: sbuf = (char *) fptr; fsize = (uint32_t) mref.fsize;
                           goto skip_zpad;
          default:         sz = 0;
                           status = to_string( mref, sbuf, sz );
                           if ( status != 0 ) return this->error( status );
                           slen = sz;
                           break;
        }
        fptr = (uint8_t *) (void *) sbuf;
        if ( slen < fsize ) {
          zpad  = fsize - slen;
          fsize = (uint32_t) slen;
        }
      skip_zpad:;
        break;
      }
      case MD_BOOLEAN:
        switch ( mref.ftype ) {
          case MD_UINT:   val.u64 = get_uint<uint64_t>( mref ); break;
          case MD_INT:    val.u64 = (uint64_t) get_int<int64_t>( mref ); break;
          case MD_REAL:   val.u64 = (uint64_t) get_float<double>( mref ); break;
          case MD_STRING: val.u64 = (uint64_t) parse_bool( (char *) fptr, mref.fsize ); break;
          default:        return this->error( Err::BAD_CVT_NUMBER );
        }
        fptr = (uint8_t *) (void *) &val;
        fendian = md_endian;
        break;
      default:
        return this->error( Err::BAD_CVT_NUMBER );
    }
  }

  if ( ftype == MD_PARTIAL || ( flags & MD_FIXED ) == 0 ) {
    if ( ftype == MD_PARTIAL )
      len = tib_sass_partial_pack_size( fsize );
    else
      len = tib_sass_variable_pack_size( fsize, fsize + zpad );
    if ( ! this->has_space( len ) )
      return this->error( Err::NO_SPACE );
    ptr = &this->buf[ this->off + 8 ];
    fid |= ( (uint16_t) flags << 14 ); /* FIXED = 2, PRIMITIVE = 1 */
    ptr[ 0 ] = (uint8_t) ( ( fid >> 8 ) & 0xffU );
    ptr[ 1 ] = (uint8_t) ( fid & 0xffU );
    if ( ftype == MD_PARTIAL ) {
      if ( fsize > 0xffffU )
        return this->error( Err::BAD_FIELD_SIZE );
      ptr[ 2 ] = 0; /* ( uint8_t )( ( mref.fentrysz >> 8 ) & 0xffU );*/
      ptr[ 3 ] = (uint8_t) ( mref.fentrysz & 0xffU );
      ptr[ 4 ] = (uint8_t) ( ( fsize >> 8 ) & 0xffU );
      ptr[ 5 ] = (uint8_t) ( fsize & 0xffU );
      ::memcpy( &ptr[ 6 ], fptr, fsize );
      if ( fsize + 6 < len )
        ptr[ fsize + 7 ] = 0;
    }
    else {
      if ( fsize + zpad < 0xffffU ) {
        ptr[ 2 ] = ( fsize >> 8 ) & 0xffU;
        ptr[ 3 ] = fsize & 0xffU;
        ::memcpy( &ptr[ 4 ], fptr, fsize );
        if ( fsize + 4 < len )
          ptr[ fsize + 5 ] = 0;
      }
      else {
        ptr[ 2 ] = ( fsize >> 24 ) & 0xffU;
        ptr[ 3 ] = ( fsize >> 16 ) & 0xffU;
        ptr[ 4 ] = ( fsize >> 8 ) & 0xffU;
        ptr[ 5 ] = fsize & 0xffU;
        ::memcpy( &ptr[ 6 ], fptr, fsize );
        if ( fsize + 6 < len )
          ptr[ fsize + 7 ] = 0;
      }
    }
  }
  else {
    fid |= (uint16_t) ( MD_FIXED | MD_PRIMITIVE ) << 14;
    ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
    ptr[ 1 ] = fid & 0xffU;
    /* invert endian, for little -> big */
    if ( fendian != MD_BIG &&
         ( ftype == MD_UINT || ftype == MD_INT || ftype == MD_REAL ||
           ftype == MD_BOOLEAN ) ) {
      size_t off = fsize;
      ptr[ 2 ] = fptr[ --off ];
      if ( off > 0 ) {
        ptr[ 3 ] = fptr[ --off ];
        if ( off > 0 ) {
          ptr[ 4 ] = fptr[ --off ];
          ptr[ 5 ] = fptr[ --off ];
          if ( off > 0 ) {
            ptr[ 6 ] = fptr[ --off ];
            ptr[ 7 ] = fptr[ --off ];
            ptr[ 8 ] = fptr[ --off ];
            ptr[ 9 ] = fptr[ --off ];
          }
        }
      }
    }
    else {
      ::memcpy( &ptr[ 2 ], fptr, fsize );
    }
    if ( zpad > 0 )
      ::memset( &ptr[ 2 + fsize ], 0, zpad );
  }
  this->off += len;
  return *this;
}

TibSassMsgWriter &
TibSassMsgWriter::append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                                  MDDecimal &dec,
                                  const MDFormEntry *entry ) noexcept
{
  MDReference mref;
  double      fval;
  float       f32val;

  if ( ftype == MD_STRING ) {
    char sbuf[ 64 ];
    mref.fsize    = dec.get_string( sbuf, sizeof( sbuf ) );
    mref.fptr     = (uint8_t *) sbuf;
    mref.ftype    = MD_STRING;
    mref.fendian  = MD_BIG;
    mref.fentrysz = 0;
    return this->append_ref( fid, ftype, fsize, MD_FIXED, mref, entry );
  }
  if ( dec.get_real( fval ) == 0 ) {
    if ( ftype == MD_DECIMAL ) {
      uint8_t * ptr,
              * fptr,
                h; /* translate md hint into tib hint */
      size_t    n    = ( fsize > 8 ? 8 : 4 ),
                len  = tib_sass_pack_size( fsize );

      if ( entry != NULL && this->use_form )
        ptr = &this->buf[ entry->foffset + 8 ];
      else {
        if ( ! this->has_space( len ) )
          return this->error( Err::NO_SPACE );
        ptr = &this->buf[ this->off + 8 ];
      }
      switch ( dec.hint ) {
        default:
          if ( dec.hint == MD_DEC_INTEGER ) {
            h = TSS_HINT_NONE;
            break;
          }
          else if ( dec.hint <= MD_DEC_LOGn10_1 ) {
            h = -( (int8_t) dec.hint - MD_DEC_LOGn10_1 ) + TSS_HINT_PRECISION_1;
            break;
          }
          else if ( dec.hint >= MD_DEC_FRAC_2 &&
                    dec.hint <= MD_DEC_FRAC_512 ) {
            h = ( (int8_t) dec.hint - MD_DEC_FRAC_2 + TSS_HINT_DENOM_2 );
            break;
          }
          /* FALLTHRU */
        case MD_DEC_NNAN:
        case MD_DEC_NAN:
        case MD_DEC_NINF:
        case MD_DEC_INF:  h = TSS_HINT_NONE; break;
        case MD_DEC_NULL: h = TSS_HINT_BLANK_VALUE; break;
      }
      fid |= (uint16_t) ( MD_FIXED | MD_PRIMITIVE ) << 14;
      ptr[ 0 ] = ( fid >> 8 ) & 0xffU;
      ptr[ 1 ] = fid & 0xffU;

      if ( fsize < 8 ) {
        f32val = (float) fval;
        fptr = (uint8_t *) (void *) &f32val;
      }
      else {
        fptr = (uint8_t *) (void *) &fval;
      }
      /* invert endian, for little -> big */
      if ( md_endian != MD_BIG ) {
        size_t off = n;
        ptr[ 2 ] = fptr[ --off ];
        ptr[ 3 ] = fptr[ --off ];
        ptr[ 4 ] = fptr[ --off ];
        ptr[ 5 ] = fptr[ --off ];
        if ( off > 0 ) {
          ptr[ 6 ] = fptr[ --off ];
          ptr[ 7 ] = fptr[ --off ];
          ptr[ 8 ] = fptr[ --off ];
          ptr[ 9 ] = fptr[ --off ];
        }
      }
      else {
        if ( fsize > 8 )
          ::memcpy( &ptr[ 2 ], fptr, 8 );
        else
          ::memcpy( &ptr[ 2 ], fptr, 4 );
      }
      ptr[ 2 + n ] = h;
      ptr[ 2 + n+1 ] = 0;
      this->off += len;
      return *this;
    }
    mref.fsize    = sizeof( double );
    mref.fptr     = (uint8_t *) (void *) &fval;
    mref.ftype    = MD_REAL;
    mref.fendian  = md_endian;
    mref.fentrysz = 0;
    return this->append_ref( fid, ftype, fsize, MD_FIXED, mref, entry );
  }
  return this->error( Err::BAD_CVT_NUMBER );
}

TibSassMsgWriter &
TibSassMsgWriter::append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                               MDTime &time, const MDFormEntry *entry ) noexcept
{
  char        sbuf[ 32 ];
  MDReference mref;
  
  mref.fptr = (uint8_t *) sbuf;
  if ( fsize <= 6 ) {
    MDTime tmp( time );
    tmp.resolution = MD_RES_MINUTES | ( time.is_null() ? MD_RES_NULL : 0 );
    mref.fsize = tmp.get_string( sbuf, sizeof( sbuf ) );
  }
  else if ( fsize <= 10 ) {
    MDTime tmp( time );
    tmp.resolution = MD_RES_SECONDS | ( time.is_null() ? MD_RES_NULL : 0 );
    mref.fsize = tmp.get_string( sbuf, sizeof( sbuf ) );
  }
  else {
    mref.fsize = time.get_string( sbuf, sizeof( sbuf ) );
  }
  mref.ftype    = MD_STRING;
  mref.fendian  = MD_BIG;
  mref.fentrysz = 0;
  return this->append_ref( fid, ftype, fsize, MD_FIXED, mref, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                               MDDate &date, const MDFormEntry *entry ) noexcept
{
  char        sbuf[ 32 ];
  MDReference mref;
  
  mref.fptr     = (uint8_t *) sbuf;
  mref.fsize    = date.get_string( sbuf, sizeof( sbuf ) );
  mref.ftype    = MD_STRING;
  mref.fendian  = MD_BIG;
  mref.fentrysz = 0;
  return this->append_ref( fid, ftype, fsize, MD_FIXED, mref, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_enum( MDFid fid,  MDType ftype,  uint32_t fsize,
                               MDEnum &enu,  const MDFormEntry *entry ) noexcept
{
  MDReference mref;
  if ( ftype == MD_STRING ) {
    mref.fptr   = (uint8_t *) enu.disp;
    mref.fsize  = enu.disp_len;
    mref.ftype  = MD_STRING;
  }
  else {
    mref.fptr   = (uint8_t *) &enu.val;
    mref.fsize  = 2;
    mref.ftype  = MD_UINT;
  }
  mref.fendian  = MD_BIG;
  mref.fentrysz = 0;
  return this->append_ref( fid, ftype, fsize, MD_FIXED, mref, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_decimal( MDFid fid,  MDDecimal &dec ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fid );

  if ( ! this->lookup( by, entry ) )
    return this->unknown_fid();
  return this->append_decimal( fid, by.ftype, by.fsize, dec, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_time( MDFid fid,  MDTime &time ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fid );

  if ( ! this->lookup( by, entry ) )
    return this->unknown_fid();
  return this->append_time( fid, by.ftype, by.fsize, time, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_date( MDFid fid,  MDDate &date ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fid );

  if ( ! this->lookup( by, entry ) )
    return this->unknown_fid();
  return this->append_date( fid, by.ftype, by.fsize, date, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_decimal( const char *fname,  size_t fname_len,
                                  MDDecimal &dec ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fname, fname_len );

  if ( ! this->get( by, entry ) )
    return this->unknown_fid();
  return this->append_decimal( by.fid, by.ftype, by.fsize, dec, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_time( const char *fname,  size_t fname_len,
                               MDTime &time ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fname, fname_len );

  if ( ! this->get( by, entry ) )
    return this->unknown_fid();
  return this->append_time( by.fid, by.ftype, by.fsize, time, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_date( const char *fname,  size_t fname_len,
                               MDDate &date ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fname, fname_len );

  if ( ! this->get( by, entry ) )
    return this->unknown_fid();
  return this->append_date( by.fid, by.ftype, by.fsize, date, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_enum( const char *fname,  size_t fname_len,
                               MDEnum &enu ) noexcept
{
  const MDFormEntry * entry;
  MDLookup by( fname, fname_len );

  if ( ! this->get( by, entry ) )
    return this->unknown_fid();
  return this->append_enum( by.fid, by.ftype, by.fsize, enu, entry );
}

TibSassMsgWriter &
TibSassMsgWriter::append_iter( MDFieldIter *iter ) noexcept
{
  size_t len = iter->field_end - iter->field_start;
  if ( ! this->has_space( len ) )
    return this->error( Err::NO_SPACE );
  uint8_t * ptr  = &this->buf[ this->off + 8 ],
          * iptr = &((uint8_t *) iter->iter_msg.msg_buf)[ iter->field_start ];
  ::memcpy( ptr, iptr, len );
  this->off += len;
  return *this;
}

int
TibSassMsgWriter::convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept
{
  MDFieldIter *iter;
  int status;
  if ( (status = msg.get_field_iter( iter )) == 0 &&
       (status = iter->first()) == 0 ) {
    do {
      MDName      n;
      MDReference mref/*, href*/;
      MDEnum      enu;
      if ( (status = iter->get_name( n )) == 0 &&
           (status = iter->get_reference( mref )) == 0 ) {
        if ( skip_hdr && is_sass_hdr( n ) )
          continue;
        if ( mref.ftype == MD_ENUM ) {
          if ( iter->get_enum( mref, enu ) == 0 )
            this->append_enum( n.fname, n.fnamelen, enu );
          else
            this->append_uint( n.fname, n.fnamelen,
                               get_uint<uint16_t>( mref ) );
        }
        else {
          /*iter->get_hint_reference( href );*/
          this->append_ref( n.fname, n.fnamelen, mref/*, href*/ );
        }
        status = this->err;
      }
      if ( status != 0 )
        break;
    } while ( (status = iter->next()) == 0 );
  }
  if ( status != Err::NOT_FOUND )
    return status;
  return 0;
}
