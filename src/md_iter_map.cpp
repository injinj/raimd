#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <raimd/md_field_iter.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

extern "C" {

MDFieldReader_t *
md_msg_get_field_reader( MDMsg_t *m )
{
  void *p = ((MDMsgMem *) static_cast<MDMsg *>( m )->mem)->make( sizeof( MDFieldReader ) );
  return (MDFieldReader_t *) new ( p ) MDFieldReader( *static_cast<MDMsg *>( m ) );
}

bool md_field_reader_find( MDFieldReader_t *rd, const char *fn,  size_t flen ) { return ((MDFieldReader *) rd)->find( fn, flen ); }
bool md_field_reader_first( MDFieldReader_t *rd,  MDName_t *n ) { return ((MDFieldReader *) rd)->first( *(MDName *) n ); }
bool md_field_reader_next( MDFieldReader_t *rd,  MDName_t *n ) { return ((MDFieldReader *) rd)->next( *(MDName *) n ); }
MDType md_field_reader_type( MDFieldReader_t *rd ) { return ((MDFieldReader *) rd)->type(); }
bool md_field_reader_get_uint( MDFieldReader_t *rd, void *u, size_t ulen ) { return ((MDFieldReader *) rd)->get_value( u, ulen, MD_UINT ); }
bool md_field_reader_get_int( MDFieldReader_t *rd, void *u, size_t ulen ) { return ((MDFieldReader *) rd)->get_value( u, ulen, MD_INT ); }
bool md_field_reader_get_real( MDFieldReader_t *rd, void *r, size_t rlen ) { return ((MDFieldReader *) rd)->get_value( r, rlen, MD_REAL ); }
bool md_field_reader_get_string( MDFieldReader_t *rd, char **buf,  size_t *len ) { return ((MDFieldReader *) rd)->get_string( *buf, *len ); }
bool md_field_reader_get_opaque( MDFieldReader_t *rd, void **buf,  size_t *len ) { return ((MDFieldReader *) rd)->get_opaque( *buf, *len ); }
bool md_field_reader_get_string_buf( MDFieldReader_t *rd, char *buf, size_t len, size_t *blen ) { return ((MDFieldReader *) rd)->get_string( buf, len, *blen ); }
bool md_field_reader_get_time( MDFieldReader_t *rd, MDTime_t *time ) { return ((MDFieldReader *) rd)->get_value( time, sizeof( MDTime ), MD_TIME ); }
bool md_field_reader_get_date( MDFieldReader_t *rd, MDDate_t *date ) { return ((MDFieldReader *) rd)->get_value( date, sizeof( MDDate ), MD_DATE ); }
bool md_field_reader_get_decimal( MDFieldReader_t *rd, MDDecimal_t *dec ) { return ((MDFieldReader *) rd)->get_value( dec, sizeof( MDDecimal ), MD_DECIMAL ); }
bool md_field_reader_get_sub_msg( MDFieldReader_t *rd,  MDMsg_t **msg ) { MDMsg *m;  bool b = ((MDFieldReader *) rd)->get_sub_msg( m ); *msg = (MDMsg_t *) m; return b; }

}

bool
MDIterMap::index_array( size_t i,  void *&ptr,  size_t &sz ) noexcept
{
  sz  = this->elem_fsize;
  ptr = this->fptr;

  if ( sz == 0 )
    sz = this->fsize;
  else {
    if ( i * sz >= this->fsize )
      return false;
    ptr = &((uint8_t *) ptr)[ i * sz ];
  }
  return true;
}

bool
MDIterMap::copy_string( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  if ( mref.ftype == MD_STRING || mref.ftype == MD_OPAQUE ) {
    size_t len = sz;
    if ( len > mref.fsize )
      len = mref.fsize;
    ::memcpy( ptr, mref.fptr, len );
    /* null term strings */
    if ( this->ftype == MD_STRING || this->elem_ftype == MD_STRING ) {
      size_t end = sz - 1;
      if ( len < end )
        end = len;
      ((char *) ptr)[ end ] = '\0';
    }
    else if ( len < sz ) /* zero out opaque bufs */
      ::memset( &((char *) ptr)[ len ], 0, sz - len );
  }
  else {
    to_string( mref, (char *) ptr, sz );
  }
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

static inline bool get_uint_size( void *ptr,  size_t sz,  MDReference &mref ) {
  if ( is_integer( mref.ftype ) ) {
    switch ( sz ) {
      case 1: ((uint8_t *)  ptr)[ 0 ] = get_uint<uint8_t>( mref );  break;
      case 2: ((uint16_t *) ptr)[ 0 ] = get_uint<uint16_t>( mref ); break;
      case 4: ((uint32_t *) ptr)[ 0 ] = get_uint<uint32_t>( mref ); break;
      case 8: ((uint64_t *) ptr)[ 0 ] = get_uint<uint64_t>( mref ); break;
      default: return false;
    }
    return true;
  }
  int x = 0;
  switch ( sz ) {
    case 1: x = cvt_number<uint8_t> ( mref , ((uint8_t *)  ptr)[ 0 ] ); break;
    case 2: x = cvt_number<uint16_t>( mref , ((uint16_t *) ptr)[ 0 ] ); break;
    case 4: x = cvt_number<uint32_t>( mref , ((uint32_t *) ptr)[ 0 ] ); break;
    case 8: x = cvt_number<uint64_t>( mref , ((uint64_t *) ptr)[ 0 ] ); break;
    default: return false;
  }
  return x == 0;
}

static inline bool get_sint_size( void *ptr,  size_t sz,  MDReference &mref ) {
  if ( is_integer( mref.ftype ) ) {
    switch ( sz ) {
      case 1: ((int8_t *)  ptr)[ 0 ] = get_int<int8_t>( mref );  break;
      case 2: ((int16_t *) ptr)[ 0 ] = get_int<int16_t>( mref ); break;
      case 4: ((int32_t *) ptr)[ 0 ] = get_int<int32_t>( mref ); break;
      case 8: ((int64_t *) ptr)[ 0 ] = get_int<int64_t>( mref ); break;
      default: return false;
    }
    return true;
  }
  int x = 0;
  switch ( sz ) {
    case 1: x = cvt_number<int8_t> ( mref , ((int8_t *)  ptr)[ 0 ] ); break;
    case 2: x = cvt_number<int16_t>( mref , ((int16_t *) ptr)[ 0 ] ); break;
    case 4: x = cvt_number<int32_t>( mref , ((int32_t *) ptr)[ 0 ] ); break;
    case 8: x = cvt_number<int64_t>( mref , ((int64_t *) ptr)[ 0 ] ); break;
    default: return false;
  }
  return x == 0;
}

static inline bool get_real_size( void *ptr,  size_t sz,  MDReference &mref ) {
  if ( mref.ftype == MD_REAL ) {
    switch ( sz ) {
      case 4: ((float *) ptr)[ 0 ]  = get_float<float>( mref );  break;
      case 8: ((double *) ptr)[ 0 ] = get_float<double>( mref ); break;
      default: return false;
    }
    return true;
  }
  int x = 0;
  switch ( sz ) {
    case 4: x = cvt_number<float> ( mref , ((float *) ptr)[ 0 ] ); break;
    case 8: x = cvt_number<double>( mref , ((double *) ptr)[ 0 ] ); break;
    default: return false;
  }
  return x == 0;
}

static inline bool get_ipdata_size( void *ptr,  size_t sz,  MDReference &mref ) {
  if ( is_integer( mref.ftype ) ) {
    switch ( sz ) {
      case 1: ((uint8_t *)  ptr)[ 0 ] = get_uint<uint8_t>( mref );  break;
      case 2: ((uint16_t *) ptr)[ 0 ] = get_u16<MD_LITTLE>( mref.fptr ); break;
      case 4: ((uint32_t *) ptr)[ 0 ] = get_u32<MD_LITTLE>( mref.fptr ); break;
      case 8: ((uint64_t *) ptr)[ 0 ] = get_u64<MD_LITTLE>( mref.fptr ); break;
      default: return false;
    }
    return true;
  }
  return false;
}

bool
MDIterMap::copy_uint( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  if ( ! get_uint_size( ptr, sz, mref ) )
    return false;
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_sint( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  if ( ! get_sint_size( ptr, sz, mref ) )
    return false;
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_decimal( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  if ( sz != sizeof( MDDecimal ) ||
       ((MDDecimal *) ptr)->get_decimal( mref ) != 0 )
    return false;
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_array( MDMsg &msg,  MDReference &mref ) noexcept
{
  MDReference aref;
  size_t i, num_entries = mref.fsize;
  bool b = false;
  if ( mref.fentrysz > 0 )
    num_entries /= mref.fentrysz;

  if ( mref.fentrysz != 0 ) {
    aref.zero();
    aref.ftype   = mref.fentrytp;
    aref.fsize   = mref.fentrysz;
    aref.fendian = mref.fendian;
    for ( i = 0; i < num_entries; i++ ) {
      aref.fptr = &mref.fptr[ i * (size_t) mref.fentrysz ];
      if ( this->elem_ftype == MD_STRING || this->elem_ftype == MD_OPAQUE )
        b |= this->copy_string( i, aref );
      else if ( this->elem_ftype == MD_UINT )
        b |= this->copy_uint( i, aref );
      else if ( this->elem_ftype == MD_INT )
        b |= this->copy_sint( i, aref );
      else if ( this->elem_ftype == MD_DECIMAL )
        b |= this->copy_decimal( i, mref );
    }
    return b;
  }
  for ( i = 0; i < num_entries; i++ ) {
    if ( msg.get_array_ref( mref, i, aref ) == 0 ) {
      if ( this->elem_ftype == MD_STRING || this->elem_ftype == MD_OPAQUE )
        b |= this->copy_string( i, aref );
      else if ( this->elem_ftype == MD_UINT )
        b |= this->copy_uint( i, aref );
      else if ( this->elem_ftype == MD_INT )
        b |= this->copy_sint( i, aref );
      else if ( this->elem_ftype == MD_DECIMAL )
        b |= this->copy_decimal( i, mref );
    }
  }
  return b;
}

size_t
MDIterMap::get_map( MDMsg &msg,  MDIterMap *map,  size_t n,
                    MDFieldIter *iter ) noexcept
{
  MDReference mref;
  size_t count = 0;
  int    status;
  if ( iter == NULL ) {
    if ( msg.get_field_iter( iter ) != 0 )
      return 0;
  }
  for ( size_t i = 0; i < n; i++ ) {
    if ( (status = iter->find( map[ i ].fname, map[ i ].fname_len,
                               mref )) == 0 ) {
      bool b = false;
      if ( map[ i ].ftype == MD_STRING || map[ i ].ftype == MD_OPAQUE )
        b = map[ i ].copy_string( 0, mref );
      else if ( map[ i ].ftype == MD_UINT )
        b = map[ i ].copy_uint( 0, mref );
      else if ( map[ i ].ftype == MD_INT )
        b = map[ i ].copy_sint( 0, mref );
      else if ( map[ i ].ftype == MD_DECIMAL )
        b = map[ i ].copy_decimal( 0, mref );
      else if ( map[ i ].ftype == MD_ARRAY ) {
        if ( mref.ftype == MD_ARRAY )
          b = map[ i ].copy_array( msg, mref );
      }
      if ( b )
        count++;
    }
  }
  return count;
}

MDFieldReader::MDFieldReader( MDMsg &m ) noexcept
             : iter( 0 ), err( 0 )
{
  this->mref.ftype = MD_NODATA;
  this->err = m.get_field_iter( this->iter );
}
bool
MDFieldReader::first( void ) noexcept
{
  this->mref.ftype = MD_NODATA;
  if ( this->iter != NULL )
    this->err = this->iter->first();
  return this->err == 0;
}
bool
MDFieldReader::next( void ) noexcept
{
  this->mref.ftype = MD_NODATA;
  if ( this->iter != NULL )
    this->err = this->iter->next();
  return this->err == 0;
}
bool
MDFieldReader::find( const char *fname,  size_t fnamelen ) noexcept
{
  this->mref.ftype = MD_NODATA;
  if ( this->iter != NULL )
    this->err = this->iter->find( fname, fnamelen, this->mref );
  return this->err == 0;
}
MDType
MDFieldReader::type( void ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
  }
  if ( this->err != 0 )
    this->mref.ftype = MD_NODATA;
  return this->mref.ftype;
}

bool
MDFieldReader::get_value( void *val,  size_t len,  MDType t ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
  }
  if ( this->err == 0 ) {
    switch ( t ) {
      case MD_UINT:
      case MD_BOOLEAN:
        if ( ! get_uint_size( val, len, this->mref ) )
          this->err = Err::BAD_CVT_NUMBER;
        break;
      case MD_INT:
        if ( ! get_sint_size( val, len, this->mref ) )
          this->err = Err::BAD_CVT_NUMBER;
        break;
      case MD_REAL:
        if ( ! get_real_size( val, len, this->mref ) )
          this->err = Err::BAD_CVT_NUMBER;
        break;
      case MD_TIME:
        this->err = ((MDTime *) val)->get_time( this->mref );
        break;
      case MD_DATE:
        this->err = ((MDDate *) val)->get_date( this->mref );
        break;
      case MD_DECIMAL:
        this->err = ((MDDecimal *) val)->get_decimal( this->mref );
        break;
      case MD_IPDATA:
        if ( ! get_ipdata_size( val, len, this->mref ) )
          this->err = Err::BAD_CVT_NUMBER;
        break;
      case MD_DATETIME:
        this->err = ((MDStamp *) val)->get_stamp( this->mref );
        break;
      default:
        this->err = Err::BAD_FIELD_TYPE;
        break;
    }
  }
  if ( this->err != 0 ) {
    ::memset( val, 0, len );
    return false;
  }
  return this->err == 0;
}

bool
MDFieldReader::get_array_count( size_t &cnt ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
  }
  if ( this->err == 0 ) {
    cnt = this->mref.fsize;
    if ( this->mref.fentrysz != 0 )
      cnt /= this->mref.fentrysz;
  }
  else {
    cnt = 0;
  }
  return this->err == 0;
}

bool
MDFieldReader::get_array_value( void *val,  size_t cnt,  size_t elsz,
                                MDType t ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
  }
  if ( this->err == 0 ) {
    size_t num = this->mref.fsize;
    if ( this->mref.fentrysz != 0 )
      num /= this->mref.fentrysz;
    if ( num > cnt )
      num = cnt;
    for ( size_t i = 0; i < num; i++ ) {
      MDReference aref;
      this->err = this->iter->iter_msg().get_array_ref( this->mref, i, aref );
      if ( this->err != 0 )
        break;
      switch ( t ) {
        case MD_UINT:
        case MD_BOOLEAN:
          if ( ! get_uint_size( val, elsz, aref ) )
            this->err = Err::BAD_CVT_NUMBER;
          break;
        case MD_INT:
          if ( ! get_sint_size( val, elsz, aref ) )
            this->err = Err::BAD_CVT_NUMBER;
          break;
        case MD_REAL:
          if ( ! get_real_size( val, elsz, aref ) )
            this->err = Err::BAD_CVT_NUMBER;
          break;
        case MD_STRING:
        case MD_OPAQUE:
          if ( aref.ftype == MD_STRING || aref.ftype == MD_OPAQUE ) {
            char * ptr = NULL;
            if ( aref.fsize > 0 ) {
              ptr = (char *) aref.fptr;
              if ( ptr[ aref.fsize - 1 ] != '\0' )
                ptr = this->iter->iter_msg().mem->stralloc( aref.fsize, ptr );
            }
            ((char **) val)[ 0 ] = ptr;
            break;
          }
          /* FALLTHRU */
        default:
          this->err = Err::BAD_FIELD_TYPE;
          break;
      }
      if ( this->err != 0 )
        break;
      val = &((uint8_t *) val)[ elsz ];
    }
    if ( this->err != 0 )
      return false;
  }
  return this->err == 0;
}

bool
MDFieldReader::name( MDName &n ) noexcept
{
  if ( this->iter != NULL )
    this->err = this->iter->get_name( n );
  if ( this->err != 0 )
    n.zero();
  return this->err == 0;
}
bool
MDFieldReader::get_string( char *&buf,  size_t &len ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
    if ( this->err == 0 )
      this->err = this->iter->iter_msg().get_string( this->mref, buf, len );
  }
  if ( this->err != 0 )
    len = 0;
  return this->err == 0;
}
bool
MDFieldReader::get_opaque( void *&buf,  size_t &len ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
    if ( this->err == 0 ) {
      buf = this->mref.fptr;
      len = this->mref.fsize;
    }
  }
  if ( this->err != 0 )
    len = 0;
  return this->err == 0;
}
bool
MDFieldReader::get_string( char *buf, size_t buflen, size_t &len ) noexcept
{
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
    if ( this->err == 0 ) {
      char * ptr;
      if ( this->get_string( ptr, len ) ) {
        if ( len > buflen - 1 )
          len = buflen - 1;
        ::memcpy( buf, ptr, len );
        buf[ len ] = '\0';
      }
    }
  }
  if ( this->err != 0 )
    len = 0;
  return this->err == 0;
}
bool
MDFieldReader::get_sub_msg( MDMsg *&msg ) noexcept
{
  msg = NULL;
  if ( this->err == 0 ) {
    if ( this->mref.ftype == MD_NODATA )
      this->err = this->iter->get_reference( this->mref );
    if ( this->err == 0 )
      this->err = this->iter->iter_msg().get_sub_msg( this->mref, msg,
                                                      this->iter );
  }
  return this->err == 0;
}

