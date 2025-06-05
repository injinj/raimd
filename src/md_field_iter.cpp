#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <raimd/md_field_iter.h>
#include <raimd/md_msg.h>
#include <raimd/md_dict.h>
#include <raimd/sass.h>
#include <raimd/hex_dump.h>
#include <raimd/md_list.h>
#include <raimd/md_set.h>
#include <raimd/md_hll.h>

using namespace rai;
using namespace md;

static void
short_string( unsigned short x,  char *buf ) noexcept
{
  for ( unsigned short i = 10000; i >= 10; i /= 10 ) {
    if ( i < x )
      *buf++ = ( ( x / i ) % 10 ) + '0';
  }
  *buf++ = ( x % 10 ) + '0';
  *buf = '\0';
}

static const char *msg_type_string[] = { SASS_DEF_MSG_TYPE( SASS_DEF_ENUM_STRING ) };
const char *
rai::md::sass_msg_type_string( unsigned short msg_type,  char *buf ) noexcept
{
  if ( msg_type < MAX_MSG_TYPE ) {
    return msg_type_string[ msg_type ];
  }
  short_string( msg_type, buf );
  return buf;
}

static const struct {
  uint16_t status,
           char2;
  const char * str;
} rec_status_string[] = {
  SASS_DEF_REC_STATUS( SASS_DEF_ENUM_STRING_2 )
};
static const size_t rec_status_string_count = sizeof( rec_status_string ) /
                                              sizeof( rec_status_string[ 0 ] );
const char *
rai::md::sass_rec_status_string( unsigned short rec_status, char *buf ) noexcept
{
  for ( size_t i = 0; i < rec_status_string_count; i++ )
    if ( rec_status == rec_status_string[ i ].status )
      return rec_status_string[ i ].str;
  short_string( rec_status, buf );
  return buf;
}
uint16_t
rai::md::sass_rec_status_val( const char *str,  size_t len ) noexcept
{
  uint16_t i;
  if ( len > 0 && str[ len - 1 ] == '\0' )
    len--;
  if ( len == 0 )
    return 0;
  if ( str[ 0 ] >= '0' && str[ 0 ] <= '9' ) {
    i = (uint16_t) ( str[ 0 ] - '0' );
    while ( len > 1 && str[ 1 ] >= '0' && str[ 1 ] <= '9' ) {
      i = i * 10 + (uint16_t) ( str[ 1 ] - '0' );
      len--;
      str++;
    }
    return i;
  }
  if ( len >= 2 ) {
    uint16_t char2 = SASS_STR_CHAR2( str[ 0 ], str[ 1 ] );
    for ( i = 0; i < rec_status_string_count; i++ ) {
      if ( rec_status_string[ i ].char2 == char2 &&
           ::strncasecmp( rec_status_string[ i ].str, str, len ) == 0 &&
           rec_status_string[ i ].str[ len ] == '\0' )
        return rec_status_string[ i ].status;
    }
  }
  return 0;
}

extern "C" {
const char MD_SASS_MSG_TYPE[]   = "MSG_TYPE",
           MD_SASS_REC_TYPE[]   = "REC_TYPE",
           MD_SASS_SEQ_NO[]     = "SEQ_NO",
           MD_SASS_REC_STATUS[] = "REC_STATUS",
           MD_SASS_SYMBOL[]     = "SYMBOL";
}

const char * MDMsg::get_proto_string( void ) noexcept
{ return "NO PROTOCOL"; }
uint32_t MDMsg::get_type_id( void ) noexcept
{ return 0; }
int MDMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{ iter = NULL; return Err::INVALID_MSG; }
int MDMsg::get_sub_msg( MDReference &, MDMsg *&msg, MDFieldIter * ) noexcept
{ msg = NULL; return Err::INVALID_MSG; }
int MDMsg::get_reference( MDReference &mref ) noexcept
{ mref.zero(); return Err::INVALID_MSG; }
int MDMsg::get_array_ref( MDReference &,  size_t,  MDReference &aref ) noexcept
{ aref.zero(); return Err::INVALID_MSG; }

int MDFieldIter::get_name( MDName &name ) noexcept
{ name.zero(); return 0; }
int MDFieldIter::get_reference( MDReference &mref ) noexcept
{ mref.zero(); return Err::NOT_FOUND; }
int MDFieldIter::get_hint_reference( MDReference &mref ) noexcept
{ mref.zero(); return Err::NOT_FOUND; }
int MDFieldIter::find( const char *,  size_t,  MDReference & ) noexcept
{ return Err::NOT_FOUND; }
int MDFieldIter::first( void ) noexcept
{ return Err::NOT_FOUND; }
int MDFieldIter::next( void ) noexcept
{ return Err::NOT_FOUND; }
int MDFieldIter::update( MDReference & ) noexcept
{ return Err::NO_MSG_IMPL; }
MDFieldIter *MDFieldIter::copy( void ) noexcept
{ return NULL; }

MDMsg &
MDFieldIter::iter_msg( void ) const noexcept
{
  return *static_cast<MDMsg *>( this->field_iter_msg );
}

MDFieldIter::MDFieldIter( MDMsg &m ) noexcept
{
  this->field_iter_msg = (struct MDMsg_s *) &m;
  this->field_start    = 0;
  this->field_end      = 0;
  this->field_index    = 0;
}

void
MDFieldIter::dup_iter( MDFieldIter &i ) noexcept
{
  i.field_iter_msg = this->field_iter_msg;
  i.field_start    = this->field_start;
  i.field_end      = this->field_end;
  i.field_index    = this->field_index;
}

int
MDFieldIter::copy_name( char *name_buf, size_t &name_len, MDFid &fid ) noexcept
{
  MDName nm;
  int status = this->get_name( nm );
  if ( status == 0 ) {
    size_t off, size = name_len;
    if ( size > nm.fnamelen )
      size = nm.fnamelen;
    fid = nm.fid;
    if ( size > 0 && nm.fname[ size - 1 ] == '\0' )
      size--;
    for ( off = 0; off < size; off++ )
      name_buf[ off ] = nm.fname[ off ];
    if ( size < name_len )
      name_buf[ size ] = '\0';
    name_len = size;
  }
  else {
    fid = 0;
    if ( name_len > 0 )
      name_buf[ 0 ] = '\0';
    name_len = 0;
  }
  return status;
}

int
MDDecimal::get_decimal( const MDReference &mref ) noexcept
{
  switch ( mref.ftype ) {
    case MD_DECIMAL:
      if ( mref.fsize == sizeof( MDDecimal ) ||
           mref.fsize == sizeof( int64_t ) + sizeof( int8_t ) ) {
        this->ival = get_int<int64_t>( mref.fptr, mref.fendian );
        this->hint = mref.fptr[ sizeof( int64_t ) ];
        return 0;
      } /* FALLTHRU */
    default:
      this->ival = 0;
      this->hint = MD_DEC_NULL;
      return Err::BAD_DECIMAL;

    case MD_INT:
    case MD_UINT:
    case MD_BOOLEAN:
      this->ival = get_int<int64_t>( mref );
      this->hint = MD_DEC_INTEGER;
      return 0;

    case MD_STRING:
      return this->parse( (const char *) mref.fptr, mref.fsize );

    case MD_REAL: {
      double fval = get_float<double>( mref );
      this->set_real( fval );
      return 0;
    }
    case MD_DATE: {
      MDDate d;
      if ( d.get_date( mref ) != 0 )
        return Err::BAD_DECIMAL;
      if ( d.is_null() ) {
        this->ival = 0;
        this->hint = MD_DEC_NULL;
        return 0;
      }
      struct tm tm;
      ::memset( &tm, 0, sizeof( tm ) );
      tm.tm_year  = d.year > 1900 ? d.year - 1900 : 0;
      tm.tm_mon   = d.mon > 0 ? d.mon - 1 : 0;
      tm.tm_mday  = d.day;
      tm.tm_isdst = -1;
      this->ival = (int64_t) mktime( &tm );
      this->hint = MD_DEC_INTEGER;
      return 0;
    }
    case MD_TIME: {
      MDTime t;
      if ( t.get_time( mref ) != 0 )
        return Err::BAD_DECIMAL;
      if ( t.resolution == MD_RES_NULL ) {
        this->ival = 0;
        this->hint = MD_DEC_NULL;
        return 0;
      }
      this->ival = (int64_t) t.hour * 3600 + (int64_t) t.minute * 60 +
                   (int64_t) t.sec;
      if ( t.resolution == MD_RES_MILLISECS ) {
        this->ival *= 1000;
        this->ival += t.fraction;
        this->hint  = MD_DEC_LOGn10_3;
      } else if ( t.resolution == MD_RES_MICROSECS ) {
        this->ival *= 1000000;
        this->ival += t.fraction;
        this->hint  = MD_DEC_LOGn10_6;
      } else if ( t.resolution == MD_RES_NANOSECS ) {
        this->ival *= 1000000000;
        this->ival += t.fraction;
        this->hint  = MD_DEC_LOGn10_9;
      }
      else {
        this->hint  = MD_DEC_INTEGER;
      }
      return 0;
    }
  }
}

int
MDTime::get_time( const MDReference &mref ) noexcept
{
  if ( mref.ftype == MD_TIME ) {
    if ( mref.fsize == sizeof( MDTime ) ) {
      this->hour       = mref.fptr[ 0 ];
      this->minute     = mref.fptr[ 1 ];
      this->sec        = mref.fptr[ 2 ];
      this->resolution = mref.fptr[ 3 ];
      this->fraction   = get_int<uint32_t>( &mref.fptr[ 4 ], mref.fendian );
      return 0;
    }
    else if ( mref.fsize == sizeof( this->hour ) + sizeof( this->minute ) ) {
      this->hour       = mref.fptr[ 0 ];
      this->minute     = mref.fptr[ 1 ];
      this->sec        = 0;
      this->fraction   = 0;
      this->resolution = MD_RES_MINUTES;
      return 0;
    }
    else if ( mref.fsize == sizeof( this->hour ) + sizeof( this->minute ) +
                            sizeof( this->sec ) ) {
      this->hour       = mref.fptr[ 0 ];
      this->minute     = mref.fptr[ 1 ];
      this->sec        = mref.fptr[ 2 ];
      this->fraction   = 0;
      this->resolution = MD_RES_SECONDS;
      return 0;
    }
  }
  else if ( mref.ftype == MD_STRING ) {
    if ( this->parse( (const char *) mref.fptr, mref.fsize ) == 0 )
      return 0;
  }
  this->zero();
  return Err::BAD_TIME;
}

int
MDStamp::get_stamp( const MDReference &mref ) noexcept
{
  if ( mref.ftype == MD_STAMP ) {
    if ( mref.fsize == sizeof( MDStamp ) ||
         mref.fsize == sizeof( this->stamp ) + sizeof( this->resolution ) ) {
      this->stamp      = get_int<uint64_t>( mref.fptr, mref.fendian );
      this->resolution = mref.fptr[ 8 ];
      return 0;
    }
  }
  else if ( mref.ftype == MD_UINT || mref.ftype == MD_INT ) {
    this->stamp = get_int<uint64_t>( mref.fptr, mref.fendian );
  determine_resolution:;
    this->resolution = MD_RES_SECONDS;
    if ( this->stamp > (uint64_t) 1000000000 * 1000 ) {
      this->resolution = MD_RES_MILLISECS;
      if ( this->stamp > (uint64_t) 1000000000 * 1000 * 1000 ) {
        this->resolution = MD_RES_MICROSECS;
        if ( this->stamp > (uint64_t) 1000000000 * 1000 * 1000 * 1000 ) {
          this->resolution = MD_RES_NANOSECS;
        }
      }
    }
    return 0;
  }
  else if ( mref.ftype == MD_DECIMAL || mref.ftype == MD_REAL ) {
    MDDecimal d;
    if ( d.get_decimal( mref ) == 0 ) {
      this->stamp = d.ival;
      if ( d.hint == MD_DEC_INTEGER ) {
        goto determine_resolution;
      }
      if ( d.hint >= MD_DEC_LOGp10_1 ) {
        for ( ; d.hint >= MD_DEC_LOGp10_1; d.hint-- )
          this->stamp *= 10;
        goto determine_resolution;
      }
      if ( d.hint >= MD_DEC_LOGn10_3 && d.hint <= MD_DEC_LOGn10_1 ) {
        for ( ; d.hint > MD_DEC_LOGn10_3; d.hint-- )
          this->stamp *= 10;
        this->resolution = MD_RES_MILLISECS;
        return 0;
      }
      if ( d.hint >= MD_DEC_LOGn10_6 && d.hint <= MD_DEC_LOGn10_4 ) {
        for ( ; d.hint > MD_DEC_LOGn10_6; d.hint-- )
          this->stamp *= 10;
        this->resolution = MD_RES_MICROSECS;
        return 0;
      }
      if ( d.hint <= MD_DEC_LOGn10_7 ) {
        for ( ; d.hint > MD_DEC_LOGn10_9; d.hint-- )
          this->stamp *= 10;
        for ( ; d.hint < MD_DEC_LOGn10_9; d.hint++ )
          this->stamp /= 10;
        this->resolution = MD_RES_NANOSECS;
        return 0;
      }
    }
  }
  else if ( mref.ftype == MD_STRING ) {
    if ( this->parse( (const char *) mref.fptr, mref.fsize ) == 0 )
      return 0;
  }
  else if ( mref.ftype == MD_DATETIME ) { /* rv datetime */
    if ( mref.fsize == 8 ) {
      uint64_t x = get_int<uint64_t>( mref.fptr, mref.fendian );
      this->stamp = ( x >> 32 ) * 1000000 +
                    ( x & 0xffffffff );
      this->resolution = MD_RES_MICROSECS;
      return 0;
    }
  }
  this->zero();
  return Err::BAD_STAMP;
}

int
MDDate::get_date( const MDReference &mref ) noexcept
{
  if ( mref.ftype == MD_DATE ) {
    if ( mref.fsize == sizeof( MDDate ) ) {
      this->year = get_int<uint16_t>( &mref.fptr[ 0 ], mref.fendian );
      this->mon  = mref.fptr[ 2 ];
      this->day  = mref.fptr[ 3 ];
      return 0;
    }
  }
  else if ( mref.ftype == MD_STRING ) {
    if ( this->parse( (const char *) mref.fptr, mref.fsize ) == 0 )
      return 0;
  }
  this->zero();
  return Err::BAD_DATE;
}

int
MDFieldIter::get_enum( MDReference &,  MDEnum &enu ) noexcept
{
  /* fetch enum from dictionary */
  enu.zero();
  return Err::NO_ENUM;
}

int
MDMsg::msg_to_string( MDReference &mref,  char *&str,  size_t &len ) noexcept
{
  MDMsg * msg = NULL;
  int status = this->get_sub_msg( mref, msg, NULL );
  if ( status != 0 )
    return status;
  MDOutput bout;
  bout.filep = tmpfile();
  status = Err::INVALID_MSG;
  if ( bout.filep != NULL ) {
    bout.puts( "{" );
    msg->print( &bout, 0, NULL, NULL );
    bout.puts( "}" );
    bout.flush();
    len = ::ftell( (FILE *) bout.filep );
    ::rewind( (FILE *) bout.filep );
    str = this->mem->str_make( len + 1 );
    if ( ::fread( str, 1, len, (FILE *) bout.filep ) == len ) {
      str[ len ] = '\0';
      status = 0;
    }
  }
  return status;
}

int
MDMsg::hash_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  /* TODO */
  return Err::INVALID_MSG;
}

int
MDMsg::zset_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  /* TODO */
  return Err::INVALID_MSG;
}

int
MDMsg::geo_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  /* TODO */
  return Err::INVALID_MSG;
}

int
MDMsg::array_to_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  static char mt[] = "[]";
  MDMsgMem    tmp,
            * sav = this->mem;
  MDReference aref;
  char     ** str;
  size_t      i, j = 0,
            * k;
  int         status;

  size_t num_entries = mref.fsize;
  if ( mref.fentrysz > 0 )
    num_entries /= mref.fentrysz;
  if ( num_entries == 0 ) {
    buf = mt;
    len = 2;
    return 0;
  }
  this->mem = &tmp;
  tmp.alloc( num_entries * sizeof( str[ 0 ] ), &str );
  tmp.alloc( num_entries * sizeof( k[ 0 ] ), &k );
  if ( mref.fentrysz != 0 ) {
    aref.zero();
    aref.ftype   = mref.fentrytp;
    aref.fsize   = mref.fentrysz;
    aref.fendian = mref.fendian;
    for ( i = 0; i < num_entries; i++ ) {
      aref.fptr = &mref.fptr[ i * (size_t) mref.fentrysz ];
      if ( (status = this->get_quoted_string( aref, str[ i ], k[ i ] )) != 0 ) {
        this->mem = sav;
        return status;
      }
      j += k[ i ];
    }
  }
  else {
    for ( i = 0; i < num_entries; i++ ) {
      if ( (status = this->get_array_ref( mref, i, aref )) != 0 ||
           (status = this->get_quoted_string( aref, str[ i ], k[ i ] )) != 0 ) {
        this->mem = sav;
        return status;
      }
      j += k[ i ];
    }
  }
  this->mem = sav;
  return this->concat_array_to_string( str, k, num_entries, j, buf, len );
}

int
MDMsg::list_to_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  static char mt[] = "[]";
  MDMsgMem    tmp,
            * sav = this->mem;
  char     ** str;
  size_t      i, j = 0,
            * k;
  ListData    ldata( mref.fptr, mref.fsize );
  int         status;

  ldata.open( mref.fptr, mref.fsize );
  size_t num_entries = ldata.count();
  if ( num_entries == 0 ) {
    buf = mt;
    len = 2;
    return 0;
  }
  this->mem = &tmp;
  tmp.alloc( num_entries * sizeof( str[ 0 ] ), &str );
  tmp.alloc( num_entries * sizeof( k[ 0 ] ), &k );
  for ( i = 0; i < num_entries; i++ ) {
    MDReference aref;
    ListVal lv;
    aref.zero();
    if ( ldata.lindex( i, lv ) == LIST_OK ) {
      aref.ftype = MD_STRING;
      aref.fptr  = (uint8_t *) lv.data;
      aref.fsize = lv.sz;
      if ( lv.sz2 != 0 ) {
        aref.fsize += lv.sz2;
        tmp.alloc( aref.fsize, &aref.fptr );
        lv.copy_out( aref.fptr, 0, aref.fsize );
      }
    }
    if ( (status = this->get_quoted_string( aref, str[ i ], k[ i ] )) != 0 ) {
      this->mem = sav;
      return status;
    }
    j += k[ i ];
  }
  this->mem = sav;
  return this->concat_array_to_string( str, k, num_entries, j, buf, len );
}

int
MDMsg::set_to_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  static char mt[] = "[]";
  MDMsgMem    tmp,
            * sav = this->mem;
  char     ** str;
  size_t      i, j = 0,
            * k;
  SetData     sdata( mref.fptr, mref.fsize );
  int         status;

  sdata.open( mref.fptr, mref.fsize );
  size_t num_entries = sdata.hcount();
  if ( num_entries <= 1 ) {
    buf = mt;
    len = 2;
    return 0;
  }
  this->mem = &tmp;
  tmp.alloc( num_entries * sizeof( str[ 0 ] ), &str );
  tmp.alloc( num_entries * sizeof( k[ 0 ] ), &k );
  for ( i = 0; i < num_entries; i++ ) {
    MDReference aref;
    ListVal lv;
    aref.zero();
    if ( sdata.lindex( i+1, lv ) == LIST_OK ) {
      aref.ftype = MD_STRING;
      aref.fptr  = (uint8_t *) lv.data;
      aref.fsize = lv.sz;
      if ( lv.sz2 != 0 ) {
        aref.fsize += lv.sz2;
        tmp.alloc( aref.fsize, &aref.fptr );
        lv.copy_out( aref.fptr, 0, aref.fsize );
      }
    }
    if ( (status = this->get_quoted_string( aref, str[ i ], k[ i ] )) != 0 ) {
      this->mem = sav;
      return status;
    }
    j += k[ i ];
  }
  this->mem = sav;
  return this->concat_array_to_string( str, k, num_entries, j, buf, len );
}

int
MDMsg::concat_array_to_string( char **str,  size_t *k,  size_t num_entries,
                               size_t tot_len,  char *&buf,
                               size_t &len ) noexcept
{
  char * astr = NULL;
  size_t i, j;

  this->mem->alloc( tot_len + 3 + ( num_entries - 1 ), &astr );
  j = 0;
  astr[ j++ ] = '[';
  ::memcpy( &astr[ j ], str[ 0 ], k[ 0 ] );
  j += k[ 0 ];
  for ( i = 1; i < num_entries; i++ ) {
    astr[ j++ ] = ',';
    ::memcpy( &astr[ j ], str[ i ], k[ i ] );
    j += k[ i ];
  }
  astr[ j++ ] = ']';
  astr[ j ] = '\0';

  buf = astr;
  len = j;
  return 0;
}

int
MDMsg::hll_to_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  HyperLogLog * hll = (HyperLogLog *) (void *) mref.fptr;
  double e  = hll->estimate();
  char   num[ 64 ];
  len = float_str( ::round( e ), num );
  this->mem->alloc( len + 1, &buf );
  ::memcpy( buf, num, len );
  buf[ len ] = '\0';
  return 0;
}

int
MDMsg::stream_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  return Err::INVALID_MSG;
}

int
MDMsg::xml_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  return Err::INVALID_MSG;
}

int
MDMsg::time_to_string( MDReference &,  char *&,  size_t & ) noexcept
{
  return Err::INVALID_MSG;
}

static char   nul_string[]   = "null";
static size_t nul_string_len = 4;

int
MDMsg::get_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  char   num[ 128 ];
  char * str;
  size_t sz;

  if ( mref.fsize == 0 ) {
    buf = nul_string;
    len = nul_string_len;
    return 0;
  }
  switch ( mref.ftype ) {
    case MD_STRING: {
      char * s = (char *) (void *) mref.fptr;
      size_t i;
      if ( s[ 0 ] == '\0' ) {
        buf = s;
        len = 0;
        return 0;
      }
      for ( i = mref.fsize; i > 0; )
        if ( s[ --i ] == '\0' )
          break;
      if ( i != 0 ) {
        buf = s;
        len = i;
        return 0;
      }
      this->mem->alloc( mref.fsize + 1, &str );
      ::memcpy( str, s, mref.fsize );
      str[ mref.fsize ] = '\0';
      buf = str;
      len = mref.fsize;
      return 0;
    }
    case MD_BOOLEAN: {
      static char tr[] = "true";
      static char fa[] = "false";
      if ( get_uint<uint8_t>( mref ) ) {
        buf = tr;
        len = 4;
      }
      else {
        buf = fa;
        len = 5;
      }
      return 0;
    }
    case MD_IPDATA:
      if ( mref.fsize == 4 ) {
        const uint8_t * ptr = (const uint8_t *) (const void *) mref.fptr;
        sz = 0;
        for ( size_t i = 0; i < 4; i++ ) {
          sz += uint_str( ptr[ i ], &num[ sz ] );
          if ( i != 3 )
            num[ sz++ ] = '.';
        }
        goto return_num;
      }
      /* FALLTHRU */
    case MD_UINT:
    case MD_ENUM:
      switch ( mref.fsize ) {
        case 1: sz = uint_str( get_uint<uint8_t>( mref ), num ); break;
        case 2: sz = uint_str( get_uint<uint16_t>( mref ), num ); break;
        case 4: sz = uint_str( get_uint<uint32_t>( mref ), num ); break;
        case 8: sz = uint_str( get_uint<uint64_t>( mref ), num ); break;
        default: goto bad_num;
      }
      goto return_num;

    case MD_INT:
      switch ( mref.fsize ) {
        case 1: sz = int_str( get_int<int8_t>( mref ), num ); break;
        case 2: sz = int_str( get_int<int16_t>( mref ), num ); break;
        case 4: sz = int_str( get_int<int32_t>( mref ), num ); break;
        case 8: sz = int_str( get_int<int64_t>( mref ), num ); break;
        default: goto bad_num;
      }
      goto return_num;

    case MD_REAL:
      switch ( mref.fsize ) {
        case 4: sz = float_str( get_float<float>( mref ), num ); break;
        case 8: sz = float_str( get_float<double>( mref ), num ); break;
        default: goto bad_num;
      }
      goto return_num;

    case MD_MESSAGE:     return this->msg_to_string( mref, buf, len );
    case MD_ARRAY:       return this->array_to_string( mref, buf, len );
    case MD_LIST:        return this->list_to_string( mref, buf, len );
    case MD_HASH:        return this->hash_to_string( mref, buf, len );
    case MD_SET:         return this->set_to_string( mref, buf, len );
    case MD_ZSET:        return this->zset_to_string( mref, buf, len );
    case MD_GEO:         return this->geo_to_string( mref, buf, len );
    case MD_HYPERLOGLOG: return this->hll_to_string( mref, buf, len );
    case MD_STREAM:      return this->stream_to_string( mref, buf, len );
    case MD_XML:         return this->xml_to_string( mref, buf, len );

    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      sz = time.get_string( num, sizeof( num ) );
      goto return_num;
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      sz = date.get_string( num, sizeof( num ) );
      goto return_num;
    }
    case MD_DATETIME:
    case MD_STAMP:   return this->time_to_string( mref, buf, len );
    case MD_SUBJECT: return this->get_subject_string( mref, buf, len );

    case MD_DECIMAL: {
        MDDecimal dec;
        dec.get_decimal( mref );
        sz = dec.get_string( num, sizeof( num ) );
        goto return_num;
      }
      /* FALLTHRU */
    case MD_NODATA:
    case MD_PARTIAL:
    case MD_OPAQUE:
      break;
  }
bad_num:;
  return this->get_escaped_string( mref, "\"", buf, len );
return_num:;
    buf = this->mem->stralloc( sz, num );
    len = sz;
    return 0;
}

int
MDMsg::get_quoted_string( MDReference &mref,  char *&buf,
                          size_t &len ) noexcept
{
  switch ( mref.ftype ) {
    case MD_STRING:
    case MD_NODATA:
    case MD_OPAQUE:
    case MD_PARTIAL:
      if ( mref.fsize == 0 ) {
        buf = nul_string;
        len = nul_string_len;
        return 0;
      }
      return this->get_escaped_string( mref, "\"", buf, len );
    default:
      return this->get_string( mref, buf, len );
  }
}

size_t
MDMsg::get_escaped_string_len( MDReference &mref,  const char *quotes ) noexcept
{
  uint8_t * ptr = mref.fptr;
  size_t    j   = 0;

  if ( mref.fsize == 0 )
    return nul_string_len;

  if ( quotes != NULL )
    j += 2;
  for ( size_t i = 0; i < mref.fsize; i++ ) {
    switch ( ptr[ i ] ) {
      case '\n': case '\r': case '\t': case '\"': case '\\':
      case '\b': case '\f':
        j += 2;
        break;
      case 0:
        if ( mref.ftype == MD_STRING || mref.ftype == MD_PARTIAL )
          return j;
        /* FALLTHRU */
      default:
        j += ( ( ptr[ i ] >= ' ' && ptr[ i ] <= '~' ) ? 1 : 6 );
        break;
    }
  }
  return j;
}

size_t
MDMsg::get_escaped_string_output( MDReference &mref,  const char *quotes,
                                  char *str ) noexcept
{
  uint8_t * ptr = mref.fptr;
  size_t    j   = 0;

  if ( mref.fsize == 0 ) {
    ::memcpy( str, nul_string, nul_string_len + 1 );
    return nul_string_len;
  }
  if ( quotes != NULL )
    str[ j++ ] = quotes[ 0 ];
  for ( size_t i = 0; i < mref.fsize; i++ ) {
    switch ( ptr[ i ] ) {
      case '\b': str[ j++ ] = '\\'; str[ j++ ] = 'b'; break;
      case '\f': str[ j++ ] = '\\'; str[ j++ ] = 'f'; break;
      case '\n': str[ j++ ] = '\\'; str[ j++ ] = 'n'; break;
      case '\r': str[ j++ ] = '\\'; str[ j++ ] = 'r'; break;
      case '\t': str[ j++ ] = '\\'; str[ j++ ] = 't'; break;
      case '\"': str[ j++ ] = '\\'; str[ j++ ] = '\"'; break;
      case '\\': str[ j++ ] = '\\'; str[ j++ ] = '\\'; break;
      case 0:
        if ( mref.ftype == MD_STRING || mref.ftype == MD_PARTIAL )
          goto break_loop;
        /* FALLTHRU */
      default:
        if ( ptr[ i ] >= ' ' && ptr[ i ] <= '~' )
          str[ j++ ] = (char) ptr[ i ];
        else {
          str[ j++ ] = '\\';
          str[ j++ ] = 'u';
          str[ j++ ] = '0';
          str[ j++ ] = '0';
          str[ j++ ] = hexchar( ( ptr[ i ] >> 4 ) & 0xfU );
          str[ j++ ] = hexchar( ptr[ i ] & 0xfU );
        }
        break;
    }
  }
break_loop:;
  if ( quotes != NULL )
    str[ j++ ] = quotes[ 0 ];
  str[ j ] = '\0';
  return j;
}

int
MDMsg::get_escaped_string( MDReference &mref,  const char *quotes,
                           char *&buf,  size_t &len ) noexcept
{
  size_t j   = this->get_escaped_string_len( mref, quotes );
  char * str = NULL;
  this->mem->alloc( j + 1, &str );
  len = this->get_escaped_string_output( mref, quotes, str );
  buf = str;
  return 0;
}

int
MDMsg::get_hex_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  uint8_t * ptr = mref.fptr;
  char    * str;
  size_t    i, j;

  if ( mref.fsize == 0 ) {
    buf = nul_string;
    len = nul_string_len;
    return 0;
  }
  j = 2 + mref.fsize * 2;
  this->mem->alloc( j + 1, &str );
  j = 0;
  str[ j++ ] = '0';
  str[ j++ ] = 'x';
  for ( i = 0; i < mref.fsize; i++ ) {
    str[ j++ ] = hexchar( ( ptr[ i ] >> 4 ) & 0xfU );
    str[ j++ ] = hexchar( ptr[ i ] & 0xfU );
  }
  str[ j ] = '\0';
  buf = str;
  len = j;
  return 0;
}

static char
b64_char( uint32_t b )
{
  /* ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ */
  static const uint32_t AZ = 'Z' - 'A' + 1;
  if ( b < AZ )
    return 'A' + b;
  if ( b < AZ * 2 )
    return 'a' + ( b - AZ );
  if ( b < AZ * 2 + 10 )
    return '0' + ( b - AZ*2 );
  if ( b == AZ * 2 + 10 )
    return '+';
  return '/';
}

int
MDMsg::get_b64_string( MDReference &mref,  char *&buf,  size_t &len ) noexcept
{
  uint8_t * ptr = mref.fptr;
  char    * str;
  size_t    i, j;
  uint32_t  reg;
  uint8_t   bits;

  if ( mref.fsize == 0 ) {
    buf = nul_string;
    len = nul_string_len;
    return 0;
  }
  j = ( mref.fsize * 8 + 5 ) / 6;
  this->mem->alloc( j + 1, &str );
  j    = 0;
  bits = 0;
  reg  = 0;
  for ( i = 0; i < mref.fsize; ) {
    if ( bits < 6 ) {
      reg   = ( reg << 8 ) | ptr[ i++ ];
      bits += 8;
    }
    bits -= 6;
    str[ j++ ] = b64_char( ( reg >> bits ) & 0x3fU );
  }
  if ( bits > 0 ) {
    reg &= ( 1U << bits ) - 1;
    reg <<= ( 6 - bits );
    str[ j++ ] = b64_char( reg );
  }
  str[ j ] = '\0';
  buf = str;
  len = j;
  return 0;
}

int
MDMsg::get_subject_string( MDReference &mref,  char *&buf,
                           size_t &len ) noexcept
{
  static char bad_subject[] = "bad.subject";
  char * str = NULL;
  uint32_t i, j, k;
  uint8_t segs;

  if ( mref.fsize == 0 )
    goto bad_subject;

  segs = mref.fptr[ 0 ];
  j = 1; k = 0;
  if ( segs == 0 )
    goto bad_subject;

  for (;;) {
    i = j + 1;
    if ( j >= mref.fsize )
      goto bad_subject;
    j += mref.fptr[ j ];
    if ( i + 1 < j )
      k += j - ( i + 1 );
    if ( --segs == 0 )
      break;
    k++;
  }
  this->mem->alloc( k + 1, &str );
  segs = mref.fptr[ 0 ];
  j = 1; k = 0;
  for (;;) {
    i = j + 1;
    j += mref.fptr[ j ];
    while ( i + 1 < j )
      str[ k++ ] = (char) mref.fptr[ i++ ];
    if ( --segs == 0 )
      break;
    str[ k++ ] = '.';
  }
  str[ k ] = '\0';
  buf = str;
  len = k;
  return 0;

bad_subject:;
  buf = bad_subject;
  len = sizeof( bad_subject ) - 1;
  return Err::BAD_SUBJECT;
}

int
MDOutput::printf( const char *fmt, ... ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stdout : (FILE *) this->filep );
  va_list ap;
  int n;
  va_start( ap, fmt );
  n = vfprintf( fp, fmt, ap );
  va_end( ap );
  return n;
}

int
MDOutput::printe( const char *fmt, ... ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stderr : (FILE *) this->filep );
  va_list ap;
  int n;
  va_start( ap, fmt );
  n = vfprintf( fp, fmt, ap );
  va_end( ap );
  if ( this->filep == NULL )
    fflush( stderr );
  return n;
}

int
MDOutput::flush( void ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stdout : (FILE *) this->filep );
  return fflush( fp );
}

int
MDOutput::puts( const char *s ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stdout : (FILE *) this->filep );
  if ( s != NULL ) {
    int n = fputs( s, fp );
    if ( n > 0 )
      return (int) ::strlen( s );
  }
  return 0;
}

size_t
MDOutput::write( const void *buf,  size_t buflen ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stdout : (FILE *) this->filep );
  return fwrite( buf, 1, buflen, fp );
}

int
MDOutput::open( const char *path,  const char *mode ) noexcept
{
  FILE * fp = fopen( path, mode );
  if ( fp != NULL ) {
    this->filep = (void *) fp;
    return 0;
  }
  return -1;
}

int
MDOutput::close( void ) noexcept
{
  FILE * fp = ( this->filep == NULL ? stdout : (FILE *) this->filep );
  if ( fp != NULL ) {
    this->filep = NULL;
    return fclose( fp );
  }
  return 0;
}

int
MDOutput::print_hex( const void *buf,  size_t buflen ) noexcept
{
  MDHexDump hex;
  int n = 0;
  for ( size_t off = 0; off < buflen; ) {
    off = hex.fill_line( buf, off, buflen );
    n += this->printf( "%s\n", hex.line );
    hex.flush_line();
  }
  return n;
}

void
MDHexDump::print_hex( const void *buf,  size_t buflen ) noexcept
{
  MDOutput mout;
  mout.print_hex( buf, buflen );
}

int
MDOutput::print_hex( const MDMsg *m ) noexcept
{
  return this->print_hex( (const uint8_t *) m->msg_buf + m->msg_off, m->msg_end - m->msg_off );
}

const char *
rai::md::md_type_str( MDType type,  size_t size ) noexcept
{
  switch ( type ) {
    case MD_NODATA:    return "nodata";
    case MD_MESSAGE:   return "message";
    case MD_STRING:    return "string";
    case MD_OPAQUE:    return "opaque";
    case MD_BOOLEAN:   return "boolean";
    case MD_INT:       switch ( size ) {
                         case 1: return "int8";
                         case 2: return "int16";
                         case 4: return "int32";
                         case 8: return "int64";
                         default: return "int";
                       }
    case MD_UINT:      switch ( size ) {
                         case 1: return "uint8";
                         case 2: return "uint16";
                         case 4: return "uint32";
                         case 8: return "uint64";
                         default: return "uint";
                       }
    case MD_REAL:      switch ( size ) {
                         case 4: return "real32";
                         case 8: return "real64";
                         default: return "real";
                       }
    case MD_ARRAY:     return "array";
    case MD_PARTIAL:   return "partial";
    case MD_IPDATA:    switch ( size ) {
                         case 2: return "ipdata16";
                         case 4: return "ipdata32";
                         case 16: return "ipdata128";
                         default: return "ipdata";
                       }
    case MD_SUBJECT:   return "subject";
    case MD_ENUM:      return "enum";
    case MD_TIME:      return "time";
    case MD_DATE:      return "date";
    case MD_DATETIME:  return "datetime";
    case MD_STAMP:     return "stamp";
    case MD_DECIMAL:   return "decimal";
    case MD_LIST:      return "list";
    case MD_HASH:      return "hash";
    case MD_SET:       return "set";
    case MD_ZSET:      return "zset";
    case MD_GEO:       return "geo";
    case MD_HYPERLOGLOG: return "hyperloglog";
    case MD_STREAM:    return "stream";
    case MD_XML:       return "xml";
  }
  return "invalid";
}

const char *
rai::md::md_sass_msg_type_str( uint16_t msg_type,  char *tmp_buf ) noexcept
{
  return sass_msg_type_string( msg_type, tmp_buf );
}

const char *
rai::md::md_sass_rec_status_str( uint16_t rec_status,  char *tmp_buf ) noexcept
{
  return sass_rec_status_string( rec_status, tmp_buf );
}

size_t
MDFieldIter::fname_string( char *fname_buf,  size_t &fname_len ) noexcept
{
  char     fid_buf[ 16 ];
  size_t   fname_off, maxlen = fname_len;
  MDFid    fid;
  uint32_t ufid;

  this->copy_name( fname_buf, fname_len, fid );
  fname_off = fname_len;

  if ( fid != 0 ) {
    uint32_t j = 1, k;
    fid_buf[ 0 ] = '[';
    if ( fid < 0 ) {
      fid_buf[ j++ ] = '-';
      ufid = (uint32_t) -fid;
    }
    else {
      ufid = (uint32_t) fid;
    }
    k = 1000;
    while ( ufid >= k * 10 )
      k *= 10;
    do {
      if ( ufid >= k )
        fid_buf[ j++ ] = '0' + (char) (( ufid / k ) % 10 );
      k /= 10;
    } while ( k > 1 );
    fid_buf[ j++ ] = '0' + (char) ( ufid % 10 );
    fid_buf[ j++ ] = ']';

    if ( fname_off == 0 ||
        ( fname_buf[ fname_off - 1 ] != ' ' &&
          fname_off < maxlen - 1 ) )
      fname_buf[ fname_off++ ] = ' ';
#if 0
    if ( fname_fmt != NULL && fname_fmt[ 0 ] == '%' && fname_fmt[ 1 ] != '-' ) {
      for ( k = j; k < 6; k++ )
        if ( fname_off < maxlen - 1 )
          fname_buf[ fname_off++ ] = ' ';
    }
#endif
    for ( k = 0; k < j; k++ ) {
      if ( fname_off < maxlen - 1 )
        fname_buf[ fname_off++ ] = fid_buf[ k ];
    }
  }
  else if ( fname_len == 0 ) {
    for ( const char *s = nul_string; *s != '\0'; s++ )
      fname_buf[ fname_off++ ] = *s;
  }
  fname_buf[ fname_off ] = '\0';
  return fname_off;
}

int
MDFieldIter::print( MDOutput *out, int indent_newline,
                    const char *fname_fmt,  const char *type_fmt ) noexcept
{
  MDMsgMemSwap swap( this->iter_msg().mem );
  MDReference mref, href;

  this->get_reference( mref );

  const char * tstr  = md_type_str( mref.ftype );
  size_t       fsize = mref.fsize,
               fname_len;
  MDType       ftype = mref.ftype;
  char         fname_buf[ 256 + 16 ];
  char       * str;
  size_t       len;

  if ( indent_newline > 1 )
    out->indent( indent_newline - 1 );

  fname_len = sizeof( fname_buf );
  this->fname_string( fname_buf, fname_len );

  if ( fname_fmt == NULL )
    fname_fmt = "%s=";
  out->printf( fname_fmt, fname_buf );

  if ( type_fmt != NULL )
    out->printf( type_fmt, tstr, fsize );

  switch ( ftype ) {
    case MD_MESSAGE: {
      MDMsg * msg;
      if ( this->iter_msg().get_sub_msg( mref, msg, this ) == 0 ) {
        if ( indent_newline != 0 ) {
          out->puts( "{\n" );
          indent_newline += 4;
        }
        else {
          out->puts( "{ " );
        }
        if ( msg != NULL )
          msg->print( out, indent_newline, fname_fmt, type_fmt );
        if ( indent_newline > 5 ) {
          out->indent( indent_newline - 5 );
        }
        out->puts( "}" );
      }
      break;
    }
    case MD_SUBJECT:
      if ( this->iter_msg().get_subject_string( mref, str, len ) == 0 ) {
        out->puts( str );
        break;
      }
      /* FALLTHRU */
    case MD_NODATA:
    case MD_OPAQUE:
       if ( ( out->output_hints & MD_OUTPUT_OPAQUE_TO_HEX ) != 0 ) {
         if ( this->iter_msg().get_hex_string( mref, str, len ) == 0 )
           out->puts( str );
         break;
       }
       if ( ( out->output_hints & MD_OUTPUT_OPAQUE_TO_B64 ) != 0 ) {
         if ( this->iter_msg().get_b64_string( mref, str, len ) == 0 )
           out->puts( str );
         break;
       }
      /* FALLTHRU */
    case MD_STRING:
      if ( this->iter_msg().get_escaped_string( mref, "\"", str, len ) == 0 )
        out->puts( str );
      break;

    case MD_PARTIAL:
      if ( fsize == 0 )
        out->puts( nul_string );
      else if ( this->iter_msg().get_escaped_string( mref, "\"", str, len ) == 0 )
        out->printf( "%s <offset=%u>", str, mref.fentrysz );
      break;

    case MD_ENUM:
      if ( this->iter_msg().get_string( mref, str, len ) == 0 ) {
        MDEnum enu;
        out->puts( str );
        if ( this->get_enum( mref, enu ) == 0 )
          out->printf( "  [%.*s]", (int) enu.disp_len, enu.disp );
      }
      break;

    case MD_INT:
    case MD_UINT:
      if ( this->iter_msg().get_string( mref, str, len ) == 0 ) {
        const char * extra = NULL;
        char tmp_buf[ 32 ];
        if ( fname_buf[ 0 ] >= 'M' && fname_buf[ 1 ] >= 'E' &&
             fname_buf[ 2 ] >= 'C' && fname_buf[ 3 ] == '_' ) {
          if ( MDDict::dict_equals( fname_buf, fname_len, MD_SASS_MSG_TYPE,
                                    MD_SASS_MSG_TYPE_LEN ) )
            extra = sass_msg_type_string( get_int<int16_t>( mref ), tmp_buf );
          else if ( MDDict::dict_equals( fname_buf, fname_len, MD_SASS_REC_TYPE,
                                         MD_SASS_REC_TYPE_LEN ) ) {
            MDLookup by( get_int<uint16_t>( mref ) );
            if ( this->iter_msg().dict != NULL &&
                 this->iter_msg().dict->lookup( by ) ) {
              if ( by.ftype == MD_MESSAGE )
                extra = by.fname;
            }
          }
          else if ( MDDict::dict_equals( fname_buf, fname_len, MD_SASS_REC_STATUS,
                                         MD_SASS_REC_STATUS_LEN ) )
            extra = sass_rec_status_string( get_int<int16_t>( mref ), tmp_buf );
        }
        out->puts( str );
        if ( extra != NULL )
          out->printf( "  [%s]", extra );
      }
      break;

    case MD_BOOLEAN:
    case MD_REAL:
    case MD_IPDATA: 
    case MD_ARRAY:
    case MD_TIME:
    case MD_DATE:
    case MD_DATETIME:
    case MD_STAMP:
    case MD_DECIMAL:
    case MD_LIST:
    case MD_HASH:
    case MD_SET:
    case MD_ZSET:
    case MD_GEO:
    case MD_HYPERLOGLOG:
    case MD_STREAM:
    case MD_XML:
      if ( this->iter_msg().get_string( mref, str, len ) == 0 ) {
        out->puts( str );
      }
      break;
  }

  if ( this->get_hint_reference( href ) == 0 ) {
    if ( this->iter_msg().get_string( href, str, len ) == 0 )
      out->printf( " <%s>", str );
  }
  if ( indent_newline != 0 )
    out->puts( "\n" );
  else
    out->puts( " " );
  return 0;
}

bool 
rai::md::string_is_true( const char *s ) noexcept
{
  if ( s != NULL )
    switch ( *s ) {
      case 't':
      case 'T': return s[ 1 ] == '\0' || s[ 1 ] == ' ' || /* t, true */
               ( toupper( s[ 1 ] ) == 'R' && toupper( s[ 2 ] ) == 'U' &&
                 toupper( s[ 3 ] ) == 'E' );
      case 'y':
      case 'Y': return s[ 1 ] == '\0' || s[ 1 ] == ' ' || /* y, yes */
               ( toupper( s[ 1 ] ) == 'E' && toupper( s[ 2 ] ) == 'S' );
      case 'o':
      case 'O': return toupper( s[ 1 ] ) == 'N'; /* on */
      case '1': /* 1, + */
      case '+': return true;
    }
  return false;
}

static const double   md_dec_powers_f[] = MD_DECIMAL_POWERS;
static const uint64_t md_dec_powers_i[] = MD_DECIMAL_POWERS;

int
MDDecimal::parse( const char *s,  const size_t fsize ) noexcept
{
  const uint8_t * fptr  = (const uint8_t *) s;
  /* stk[] is up to 3 numbers separated by '.', '/', and/or 'e' */
  struct {
    uint64_t ival;
    uint32_t digit, overflow;
    bool     dot, slash, space, exp, nexp;
  } stk[ 3 ];
  size_t   i, tos;
  uint32_t e;
  bool     neg;
  static const uint64_t maxval =
    (uint64_t) 0x7fffffffU * (uint64_t) 0xffffffffU;

  this->zero();
  /* skip leading spaces, check for null value */
  for ( i = 0; i < fsize; i++ )
    if ( fptr[ i ] != ' ' )
      break;
  if ( i == fsize ) {
    this->hint = MD_DEC_NULL;
    return 0;
  }
  /* note the leading '-', skip a leading '+' */
  neg = false;
  if ( fptr[ i ] == '+' || fptr[ i ] == '-' ) {
    if ( fptr[ i ] == '-' )
      neg = true;
    i++;
  }

  tos = 0;
  ::memset( stk, 0, sizeof( stk ) );
  for ( ; i < fsize; i++ ) {
    switch ( fptr[ i ] ) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if ( stk[ tos ].ival >= maxval / 10 )
          stk[ tos ].overflow++; /* deal with overflow later */
        else {
          stk[ tos ].ival = stk[ tos ].ival * 10 + ( fptr[ i ] - '0' );
          stk[ tos ].digit++;
        }
        break;
      case '.': /* num.num */
        if ( tos >= 2 )
          goto end_of_float;
        stk[ tos++ ].dot = true;
        break;
      case '/': /* fraction */
        if ( tos >= 2 )
          goto end_of_float;
        stk[ tos++ ].slash = true;
        break;
      case ' ': /* num num/num */
        if ( tos >= 2 )
          goto end_of_float;
        stk[ tos++ ].space = true;
        break;
      case '%': /* num% (same as e-2) */
        if ( tos >= 2 )
          goto end_of_float;
        stk[ tos++ ].nexp = true;
        stk[ tos ].ival = 2;
        stk[ tos ].digit = 1;
        goto end_of_float;
      case 'i': case 'I': /* inf */
        if ( tos == 0 && i + 2 <= fsize ) {
          if ( fptr[ i + 1 ] == 'n' ||
               fptr[ i + 1 ] == 'N' ) {
            if ( fptr[ i + 2 ] == 'f' ||
                 fptr[ i + 2 ] == 'F' ) {
              if ( neg )
                this->hint = MD_DEC_NINF;
              else
                this->hint = MD_DEC_INF;
              return 0;
            }
          }
        }
        goto end_of_float;
      case 'n': case 'N': /* nan, -nan */
        if ( tos == 0 && i + 2 <= fsize ) {
          if ( fptr[ i + 1 ] == 'a' ||
               fptr[ i + 1 ] == 'A' ) {
            if ( fptr[ i + 2 ] == 'n' ||
                 fptr[ i + 2 ] == 'N' ) {
              if ( neg )
                this->hint = MD_DEC_NNAN;
              else
                this->hint = MD_DEC_NAN;
              return 0;
            }
          }
        }
        goto end_of_float;
      case 'e': case 'E': /* e+n, e-n */
        if ( i + 1 >= fsize || tos == 3 )
          goto end_of_float;
        if ( fptr[ i + 1 ] == '-' || fptr[ i + 1 ] == '+' ) {
          if ( i + 2 >= fsize )
            goto end_of_float;
          if ( fptr[ i + 2 ] >= '0' && fptr[ i + 2 ] <= '9' ) {
            if ( fptr[ i + 1 ] == '-' )
              stk[ tos ].nexp = true;
            else
              stk[ tos ].exp = true;
            tos++;
            i++;
          }
        }
        else if ( fptr[ i + 1 ] >= '0' && fptr[ i + 1 ] <= '9' ) {
          stk[ tos++ ].exp = true;
        }
        else
          goto end_of_float;
        break;
      default:
        goto end_of_float;
    }
  }
end_of_float:;
  if ( tos >= 1 && stk[ 0 ].dot && stk[ 1 ].digit != 0 ) {   /* num.num */
    if ( stk[ 0 ].overflow != 0 )
      goto overflow_big;
    static const uint32_t npow = sizeof( md_dec_powers_i ) /
                                 sizeof( md_dec_powers_i[ 0 ] ) - 1;
    uint64_t p;
    uint32_t n = stk[ 1 ].digit, overflow = 0;
    if ( n <= npow ) {
      p = md_dec_powers_i[ n ];
    }
    else {
      p = md_dec_powers_i[ npow ];
      for ( uint32_t j = npow; j < n; j++ ) {
        if ( p >= maxval / 10 ) {
          stk[ 1 ].ival /= 10;
          overflow++;
        }
        else {
          p *= 10;
        }
      }
    }
    while ( p > 0 && stk[ 0 ].ival > maxval / p ) {
      stk[ 1 ].ival /= 10;
      overflow++;
      p /= 10;
    }
    this->ival = stk[ 0 ].ival;
    if ( p > 1 ) {
      this->ival *= p;
      this->ival += stk[ 1 ].ival;
      this->hint = -10 - (int) ( n - overflow );
    }
    else {
      this->hint = MD_DEC_INTEGER;
    }
    e = 1;
    goto check_exponent;
  }
  else if ( tos >= 2 && stk[ 0 ].space && stk[ 1 ].slash &&
            stk[ 0 ].digit != 0 && stk[ 1 ].digit != 0 && 
            stk[ 2 ].digit != 0 ) { /* num num/num */
  zero_slash:;
    if ( stk[ 2 ].ival == 0 ) /* divide by zero */
      this->hint = MD_DEC_INF;
    else {
      /* only power of 2 fractions */
      switch ( stk[ 2 ].ival ) {
        case 1:   this->hint = MD_DEC_INTEGER; break;
        case 2:   this->hint = MD_DEC_FRAC_2; break;
        case 4:   this->hint = MD_DEC_FRAC_4; break;
        case 8:   this->hint = MD_DEC_FRAC_8; break;
        case 16:  this->hint = MD_DEC_FRAC_16; break;
        case 32:  this->hint = MD_DEC_FRAC_32; break;
        case 64:  this->hint = MD_DEC_FRAC_64; break;
        case 128: this->hint = MD_DEC_FRAC_128; break;
        case 256: this->hint = MD_DEC_FRAC_256; break;
        case 512: this->hint = MD_DEC_FRAC_512; break;
        default:  return Err::BAD_DECIMAL;
      }
      this->ival = stk[ 0 ].ival * stk[ 2 ].ival + stk[ 1 ].ival;
    }
  }
  else if ( tos >= 1 && stk[ 0 ].slash &&
            stk[ 0 ].digit != 0 && stk[ 1 ].digit != 0 ) { /* num/num */
    stk[ 2 ].ival = stk[ 1 ].ival;
    stk[ 1 ].ival = stk[ 0 ].ival;
    stk[ 0 ].ival = 0;
    goto zero_slash;
  }
  else if ( stk[ 0 ].digit > 0 ) { /* num */
  overflow_big:;
    this->ival = stk[ 0 ].ival;
    if ( stk[ 0 ].overflow == 0 )
      this->hint = MD_DEC_INTEGER;
    else
      this->hint = 10 + stk[ 0 ].overflow;
    e = 0;
  check_exponent:;
    if ( stk[ e+1 ].digit != 0 && stk[ e+1 ].ival != 0 ) {
      if ( stk[ e ].exp ) {
        if ( this->hint == MD_DEC_INTEGER )
          this->hint = 10 + (int8_t) stk[ e+1 ].ival;
        else if ( this->hint > 10 )
          this->hint += (int8_t) stk[ e+1 ].ival;
        else {
          this->hint += (int8_t) stk[ e+1 ].ival;
          if ( this->hint >= -10 ) {
            this->hint += 20;
            if ( this->hint == 10 )
              this->hint = MD_DEC_INTEGER;
          }
        }
      }
      else if ( stk[ e ].nexp ) {
        if ( this->hint == MD_DEC_INTEGER )
          this->hint = -10 - (int8_t) stk[ e+1 ].ival;
        else if ( this->hint < -10 )
          this->hint -= (int8_t) stk[ e+1 ].ival;
        else {
          this->hint -= (int8_t) stk[ e+1 ].ival;
          if ( this->hint <= 10 ) {
            this->hint -= 20;
            if ( this->hint == -10 )
              this->hint = MD_DEC_INTEGER;
          }
        }
      }
    }
  }
  else {
    return Err::BAD_DECIMAL;
  }
  if ( neg )
    this->ival = -this->ival;
  return 0;
}

static void
decimal_shift( MDDecimal &d, int8_t amt )
{
  if ( d.hint <= MD_DEC_LOGn10_1 ) {
    d.hint += amt;
    while ( d.hint > MD_DEC_LOGn10_1 ) {
      d.hint--;
      d.ival *= 10;
    }
  }
  else if ( d.hint >= MD_DEC_LOGp10_1 ) {
    d.hint -= amt;
    while ( d.hint < MD_DEC_LOGp10_1 ) {
      d.hint++;
      d.ival /= 10;
    }
  }
  else if ( d.hint == MD_DEC_INTEGER ) {
    d.hint = MD_DEC_LOGn10_1 - ( amt - 1 );
  }
}

static void
decimal_mult( MDDecimal &d, uint64_t amt )
{
  d.ival *= amt;
}

int
MDStamp::parse( const char *fptr,  size_t flen,  bool is_gm_time ) noexcept
{
  static const uint64_t MS     = 1000,
                        US     = 1000 * 1000,
                        NS     = 1000 * 1000 * 1000,
                        MINUTE = 60,
                        HOUR   = 60  * MINUTE,
                        DAY    = 24  * HOUR,
                        WEEK   = 7   * DAY,
                        MONTH  = 30  * WEEK,
                        YEAR   = 365 * DAY;
  MDDate    dt;
  MDDecimal d;
  size_t    i;
  uint64_t  res = 1;
  while ( flen > 0 && fptr[ flen - 1 ] <= ' ' )
    flen -= 1;
  if ( ::memchr( fptr, ':', flen ) != NULL ) {
    MDTime tm;
    const char *p = (const char *) ::memchr( fptr, ' ', flen );
    bool have_date;

    tm.zero();
    if ( p != NULL ) {
      const char *x;
      for ( x = p; x < &fptr[ flen ]; x++ ) {
        if ( *x != ' ' )
          break;
      }
      if ( x < &fptr[ flen ] && dt.parse( x, &fptr[ flen ] - x ) == 0 )
        have_date = true;
    }
    if ( tm.parse( fptr, ( p != NULL ? p - fptr : flen ) ) == 0 ) {
      this->stamp = tm.to_utc( have_date ? &dt : NULL, is_gm_time );
      switch ( this->resolution = tm.resolution ) {
        case MD_RES_MILLISECS:
          this->stamp = this->stamp * 1000 + (uint64_t) tm.fraction;
          break;
        case MD_RES_MICROSECS:
          this->stamp = this->stamp * 1000000 + (uint64_t) tm.fraction;
          break;
        case MD_RES_NANOSECS:
          this->stamp = this->stamp * 1000000000 + (uint64_t) tm.fraction;
          break;
        default:
          this->resolution = MD_RES_SECONDS;
          break;
      }
      return 0;
    }
  }
  i = flen;
  while ( i > 0 &&
          ( ( fptr[ i - 1 ] >= 'A' && fptr[ i - 1 ] <= 'Z' ) ||
            ( fptr[ i - 1 ] >= 'a' && fptr[ i - 1 ] <= 'z' ) ) )
    i -= 1;
  if ( i != flen ) {
    size_t       len = flen - i,
                 off = 0;
    const char * units = &fptr[ i ];

    if ( len > 2 && ( fptr[ flen - 1 ] == 's' || fptr[ flen - 1 ] == 'S' ) ) {
      off = 1;
      len -= 1;
    }
    if      ( ( len == 2 && ::strncasecmp( units, "ms", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "msec", 4 ) == 0 ) ||
              ( len == 8 && ::strncasecmp( units, "millisec", 8 ) == 0 ) ||
              ( len == 11&& ::strncasecmp( units, "millisecond", 11 ) == 0 ) ) {
      res = MS;
      off += len;
    }
    else if ( ( len == 2 && ::strncasecmp( units, "ns", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "nsec", 4 ) == 0 ) ||
              ( len == 7 && ::strncasecmp( units, "nanosec", 7 ) == 0 ) ||
              ( len == 10 && ::strncasecmp( units, "nanosecond", 10 ) == 0 ) ) {
      res = NS;
      off += len;
    }
    else if ( ( len == 2 && ::strncasecmp( units, "us", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "usec", 4 ) == 0 ) ||
              ( len == 8 && ::strncasecmp( units, "microsec", 8 ) == 0 ) ||
              ( len == 11&& ::strncasecmp( units, "microsecond", 11 ) == 0 ) ) {
      res = US;
      off += len;
    }
    else if ( ( len == 2 && ::strncasecmp( units, "wk", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "week", 4 ) == 0 ) ) {
      res = WEEK;
      off += len;
    }
    else if ( ( len == 1 && ::strncasecmp( units, "d", 1 ) == 0 ) ||
              ( len == 3 && ::strncasecmp( units, "day", 3 ) == 0 ) ) {
      res = DAY;
      off += len;
    }
    else if ( ( len == 1 && ::strncasecmp( units, "h", 1 ) == 0 ) ||
              ( len == 2 && ::strncasecmp( units, "hr", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "hour", 4 ) == 0 ) ) {
      res = HOUR;
      off += len;
    }
    else if ( ( len == 1 && ::strncasecmp( units, "m", 1 ) == 0 ) ||
              ( len == 3 && ::strncasecmp( units, "min", 3 ) == 0 ) ||
              ( len == 6 && ::strncasecmp( units, "minute", 6 ) == 0 ) ) {
      res = MINUTE;
      off += len;
    }
    else if ( ( len == 3 && ::strncasecmp( units, "mon", 3 ) == 0 ) ||
              ( len == 5 && ::strncasecmp( units, "month", 5 ) == 0 ) ) {
      res = MONTH;
      off += len;
    }
    else if ( ( len == 2 && ::strncasecmp( units, "yr", 2 ) == 0 ) ||
              ( len == 4 && ::strncasecmp( units, "year", 4 ) == 0 ) ) {
      res = YEAR;
      off += len;
    }
    else if ( ( len == 1 && ::strncasecmp( units, "s", 1 ) == 0 ) ||
              ( len == 3 && ::strncasecmp( units, "sec", 3 ) == 0 ) ||
              ( len == 6 && ::strncasecmp( units, "second", 6 ) == 0 ) ) {
      res = 1;
      off += len;
    }
    flen -= off;
  }
  if ( d.parse( fptr, flen ) == 0 ) {
    MDReference mref;
    mref.zero();
    mref.fptr     = (uint8_t *) (void *) &d;
    mref.ftype    = MD_DECIMAL;
    mref.fsize    = sizeof( d );
    mref.fendian  = md_endian;

    if ( res != 1 ) {
      if ( res == MS )
        decimal_shift( d, 3 );
      else if ( res == US )
        decimal_shift( d, 6 );
      else if ( res == NS )
        decimal_shift( d, 9 );
      else if ( res == WEEK )
        decimal_mult( d, WEEK );
      else if ( res == DAY )
        decimal_mult( d, DAY );
      else if ( res == HOUR )
        decimal_mult( d, HOUR );
      else if ( res == MINUTE )
        decimal_mult( d, MINUTE );
      else if ( res == MONTH )
        decimal_mult( d, MONTH );
      else if ( res == YEAR )
        decimal_mult( d, YEAR );
    }
    return this->get_stamp( mref );
  }
  if ( dt.parse( fptr, flen ) == 0 ) {
    this->stamp = dt.to_utc( is_gm_time );
    this->resolution = MD_RES_SECONDS;
    return 0;
  }
  return Err::BAD_STAMP;
}

uint64_t
MDStamp::seconds( void ) const noexcept
{
  uint8_t  r = this->resolution;
  uint64_t s = this->stamp;
  if ( r > MD_RES_SECONDS ) {
    if ( r <= MD_RES_NANOSECS ) {
      while ( r > MD_RES_SECONDS ) {
        s /= 1000;
        r--;
      }
    }
    else if ( r == MD_RES_MINUTES )
      s *= 60;
    else if ( r == MD_RES_NULL )
      s = 0;
  }
  return s;
}

uint64_t
MDStamp::millis( void ) const noexcept
{
  uint8_t  r = this->resolution;
  uint64_t s = this->stamp;
  if ( r >= MD_RES_MILLISECS ) {
    if ( r <= MD_RES_NANOSECS ) {
      while ( r > MD_RES_MILLISECS ) {
        s /= 1000;
        r--;
      }
    }
    else if ( r == MD_RES_MINUTES )
      s *= 60;
    else if ( r == MD_RES_NULL )
      s = 0;
  }
  else {
    s *= 1000;
  }
  return s;
}

uint64_t
MDStamp::micros( void ) const noexcept
{
  uint8_t  r = this->resolution;
  uint64_t s = this->stamp;
  if ( r >= MD_RES_MICROSECS ) {
    if ( r <= MD_RES_NANOSECS ) {
      if ( r > MD_RES_MICROSECS )
        s /= 1000;
    }
    else if ( r == MD_RES_MINUTES )
      s *= 60 * 1000 * 1000;
    else if ( r == MD_RES_NULL )
      s = 0;
  }
  else {
    while ( r < MD_RES_MICROSECS ) {
      s *= 1000;
      r++;
    }
  }
  return s;
}

uint64_t
MDStamp::nanos( void ) const noexcept
{
  uint8_t  r = this->resolution;
  uint64_t s = this->stamp;
  if ( r >= MD_RES_NANOSECS ) {
    if ( r == MD_RES_MINUTES )
      s *= (uint64_t) 60 * 1000 * 1000 * 1000;
    else if ( r == MD_RES_NULL )
      s = 0;
  }
  else {
    while ( r < MD_RES_NANOSECS ) {
      s *= 1000;
      r++;
    }
  }
  return s;
}

int
MDDecimal::get_real( double &res ) const noexcept
{
  int64_t val  = this->ival;
  int     hint = this->hint;

  if ( hint >= -10 && hint <= 10 ) {
    switch ( hint ) {
      case MD_DEC_NULL:     res = 0.0;                  return 0;
      case MD_DEC_INF:      res = INFINITY;             return 0;
      case MD_DEC_NINF:     res = -INFINITY;            return 0;
      case MD_DEC_NAN:      res = NAN;                  return 0;
      case MD_DEC_NNAN:     res = -NAN;                 return 0;
      case MD_DEC_INTEGER:  res = (double) val;         return 0;
      case MD_DEC_FRAC_2:   res = (double) val / 2.0;   return 0;
      case MD_DEC_FRAC_4:   res = (double) val / 4.0;   return 0;
      case MD_DEC_FRAC_8:   res = (double) val / 8.0;   return 0;
      case MD_DEC_FRAC_16:  res = (double) val / 16.0;  return 0;
      case MD_DEC_FRAC_32:  res = (double) val / 32.0;  return 0;
      case MD_DEC_FRAC_64:  res = (double) val / 64.0;  return 0;
      case MD_DEC_FRAC_128: res = (double) val / 128.0; return 0;
      case MD_DEC_FRAC_256: res = (double) val / 256.0; return 0;
      case MD_DEC_FRAC_512: res = (double) val / 512.0; return 0;
      default: break;
    }
  }
  else if ( hint >= MD_DEC_LOGp10_1 ) {
    if ( hint <= MD_DEC_LOGp10_9 ) {
      res = (double) val * md_dec_powers_f[ hint - 10 ];
      return 0;
    }
    for ( double f = (double) val; ; hint-- ) {
      if ( hint == MD_DEC_LOGp10_9 ) {
        res = f * md_dec_powers_f[ MD_DEC_LOGp10_9 - 10 ];
        return 0;
      }
      f *= 10.0;
    }
  }
  else if ( hint <= MD_DEC_LOGn10_1 ) {
    if ( hint >= MD_DEC_LOGn10_9 ) {
      res = (double) val / md_dec_powers_f[ -(hint + 10) ];
      return 0;
    }
    for ( double f = (double) val; ; hint++ ) {
      if ( hint == MD_DEC_LOGn10_9 ) {
        res = f / md_dec_powers_f[ -(MD_DEC_LOGn10_9 + 10) ];
        return 0;
      }
      f /= 10.0;
    }
  }
  res = 0;
  return Err::BAD_DECIMAL;
}

int
MDDecimal::get_integer( int64_t &res ) const noexcept
{
  int64_t val  = this->ival;
  int     hint = this->hint;

  if ( hint >= -10 && hint <= 10 ) {
    switch ( hint ) {
      case MD_DEC_NULL:     res = 0;           return 0;
      case MD_DEC_INF:      res = 0;           return 0;
      case MD_DEC_NINF:     res = 0;           return 0;
      case MD_DEC_NAN:      res = 0;           return 0;
      case MD_DEC_NNAN:     res = 0;           return 0;
      case MD_DEC_INTEGER:  res = val;         return 0;
      case MD_DEC_FRAC_2:   res = val / 2.0;   return 0;
      case MD_DEC_FRAC_4:   res = val / 4.0;   return 0;
      case MD_DEC_FRAC_8:   res = val / 8.0;   return 0;
      case MD_DEC_FRAC_16:  res = val / 16.0;  return 0;
      case MD_DEC_FRAC_32:  res = val / 32.0;  return 0;
      case MD_DEC_FRAC_64:  res = val / 64.0;  return 0;
      case MD_DEC_FRAC_128: res = val / 128.0; return 0;
      case MD_DEC_FRAC_256: res = val / 256.0; return 0;
      case MD_DEC_FRAC_512: res = val / 512.0; return 0;
      default: break;
    }
  }
  else if ( hint >= MD_DEC_LOGp10_1 ) {
    for ( ; ; hint-- ) {
      if ( hint <= MD_DEC_LOGp10_9 ) {
        res = val * md_dec_powers_i[ hint - 10 ];
        return 0;
      }
      val *= 10;
    }
  }
  else if ( hint <= MD_DEC_LOGn10_1 ) {
    for ( ; ; hint++ ) {
      if ( hint >= MD_DEC_LOGn10_9 ) {
        res = val / md_dec_powers_f[ -(hint + 10) ];
        return 0;
      }
      val /= 10;
    }
  }
  res = 0;
  return Err::BAD_DECIMAL;
}

int
MDDecimal::degrade( void ) noexcept
{
  switch ( this->hint ) {
    case MD_DEC_NNAN:
    case MD_DEC_NAN:
    case MD_DEC_NINF:
    case MD_DEC_INF:
    case MD_DEC_NULL:
      return Err::BAD_DECIMAL;

    case MD_DEC_INTEGER:
      this->ival /= 10;
      this->hint = MD_DEC_LOGp10_1;
      return 0;

    default:
      if ( this->hint >= MD_DEC_FRAC_2 && this->hint <= MD_DEC_FRAC_512 ) {
        this->hint -= 1;
        this->ival /= 2;
        return 0;
      }
      if ( this->hint >= MD_DEC_LOGp10_1 || this->hint <= MD_DEC_LOGn10_1 ) {
        this->ival /= 10;
        this->hint++;
        if ( this->hint == -10 )
          this->hint = MD_DEC_INTEGER;
        return 0;
      }
      return Err::BAD_DECIMAL;
  }
}
#if 0
void
MDDecimal::degrade( int8_t new_hint ) noexcept
{
  if ( this->hint <= MD_DEC_LOGn10_1 ) {
    if ( new_hint == MD_DEC_INTEGER || new_hint <= MD_DEC_LOGn10_1 ) {
      while ( new_hint > this->hint && this->hint <= MD_DEC_LOGn10_1 ) {
        this->hint++;
        this->ival /= 10;
      }
      this->hint = new_hint;
    }
  }
}
#endif
void
MDDecimal::set_real( double fval ) noexcept
{
  this->ival = 0;
  if ( isnan( fval ) ) {
    if ( fval < 0.0 )
      this->hint = MD_DEC_NNAN;
    else
      this->hint = MD_DEC_NAN;
    return;
  }
  if ( isinf( fval ) ) {
    if ( fval < 0.0 )
      this->hint = MD_DEC_NINF;
    else
      this->hint = MD_DEC_INF;
    return;
  }
  bool is_neg = false;
  if ( fval < 0.0 ) {
    is_neg = true;
    fval = -fval;
  }
  static const double max_integral =
    (double) ( ( 0x7fffffffULL << 32 ) | 0xffffffffULL );
  uint32_t max_places = 0;
  while ( fval > max_integral ) {
    fval /= 10.0;
    max_places++;
  }
  /* guess the precision by using 14 places around the decimal */
  double       integral,
               decimal,
               tmp;
  const double fraction = modf( fval, &integral );
  uint32_t     places = 14;
  uint64_t     integral_ival = (uint64_t) integral;
  /* degrade places by the number of digits to the left */
  for ( uint64_t ival_places = integral_ival;
        ival_places >= 100 && places > 1; ival_places /= 10 ) {
    places--;
  }
  /* if no decimal places */
  if ( places == 0 ) {
    this->ival = (int64_t) integral_ival;
    this->hint = MD_DEC_INTEGER;
  }
  else {
    /* determine the number of digits to the right */
    uint32_t n = sizeof( md_dec_powers_f ) /
                 sizeof( md_dec_powers_f[ 0 ] ) - 1;
    double p10;
    if ( places <= n )
      p10 = md_dec_powers_f[ places ];
    else {
      p10 = md_dec_powers_f[ n ];
      while ( n < places ) {
        p10 *= 10.0;
        n++;
      }
    }
    /* multiply fraction + 1 * places wanted (.25 + 1.0) * 1000.0 = 1250 */
    tmp = modf( ( fraction + 1.0 ) * p10, &decimal );
    /* round up, if fraction of decimal places >= 0.5 */
    if ( tmp >= 0.5 ) {
      decimal += 1.0;
      if ( decimal >= p10 * 2.0 )
        integral_ival++;
    }
    else if ( decimal >= p10 * 2.0 ) {
      decimal--;
    }
    /* degrade places with trailing zeros */
    uint64_t decimal_ival = (uint64_t) decimal;
    while ( decimal_ival > 1 && decimal_ival % 10 == 0 ) {
      decimal_ival /= 10;
      places--;
    }
    /* set the integer and power (hint) */
    this->ival = (int64_t) integral_ival;
    if ( places == 0 || decimal_ival == 1 || decimal_ival == 2 ) {
      if ( max_places > 0 )
        this->hint = MD_DEC_LOGp10_1 + ( max_places - 1 );
      else
        this->hint = MD_DEC_INTEGER;
    }
    else {
      uint32_t n = sizeof( md_dec_powers_i ) /
                   sizeof( md_dec_powers_i[ 0 ] ) - 1;
      uint64_t i10;
      if ( places <= n )
        i10 = md_dec_powers_i[ places ];
      else {
        i10 = md_dec_powers_i[ n ];
        while ( n < places ) {
          i10 *= 10;
          n++;
        }
      }
      this->hint = -10 - places;
      this->ival *= i10;
      this->ival += decimal_ival % i10;
    }
  }
  if ( is_neg )
    this->ival = -this->ival;
}

size_t
MDDecimal::get_string( char *str,  size_t len,
                       bool expand_fractions ) const noexcept
{
  int64_t  val  = this->ival;
  int      hint = this->hint;
  size_t   i = 0,
           j, k, d;
  uint32_t div,
           frac;

  if ( hint >= -10 && hint <= 10 ) {
    const char *s;
    switch ( hint ) {
      case MD_DEC_NULL:
        s = "";
      copy_s:;
        for ( ; ; i++ ) {
          if ( i == len )
            goto out_of_space;
          if ( (str[ i ] = *s++) == '\0' )
            break;
        }
        goto ok_dec;

      case MD_DEC_INF:  s = "Inf";  goto copy_s;
      case MD_DEC_NINF: s = "-Inf";  goto copy_s;
      case MD_DEC_NAN:  s = "NaN";  goto copy_s;
      case MD_DEC_NNAN: s = "-NaN"; goto copy_s;

      case MD_DEC_INTEGER:
        i = int_digs( val );
        if ( i > len )
          goto out_of_space;
        int_str( val, str, i );
        goto ok_dec;

      case MD_DEC_FRAC_2:
      case MD_DEC_FRAC_4:
      case MD_DEC_FRAC_8:
      case MD_DEC_FRAC_16:
      case MD_DEC_FRAC_32:
      case MD_DEC_FRAC_64:
      case MD_DEC_FRAC_128:
      case MD_DEC_FRAC_256:
      case MD_DEC_FRAC_512:
        if ( val < 0 ) {
          if ( i == len )
           goto out_of_space;
          str[ i++ ] = '-';
          val = -val;
        }
        div  = 2 << ( hint - MD_DEC_FRAC_2 );
        frac = (uint32_t) ( (uint64_t) val % div );
        val /= div;
        d    = int_digs( val );
        if ( expand_fractions ) {
          static const uint32_t one_fraction[] = {
            5,           /* MD_DEC_FRAC_2 */
            25,          /* MD_DEC_FRAC_4 */
            125,         /* MD_DEC_FRAC_8 */
             625,        /* MD_DEC_FRAC_16 */
             3125,       /* MD_DEC_FRAC_32 */
             15625,      /* MD_DEC_FRAC_64 */
              78125,     /* MD_DEC_FRAC_128 */
              390625,    /* MD_DEC_FRAC_256 */
              1953125    /* MD_DEC_FRAC_512 */
          //998046875
          };
          frac *= one_fraction[ hint - MD_DEC_FRAC_2 ];

          j = hint - MD_DEC_FRAC_2;
          if ( i + d + j + 2 > len )
            goto out_of_space;
          int_str( val, &str[ i ], d );
          i += d;
          str[ i++ ] = '.';
          div = (uint32_t) md_dec_powers_i[ j ];
          do {
            if ( i == len )
              goto out_of_space;
            str[ i++ ] = ( ( frac / div ) % 10 ) + '0';
            div /= 10;
          } while ( div != 0 );
          goto ok_dec;
        }
        j = uint_digs( frac );
        k = uint_digs( div );

        if ( i + d + j + k + 2 > len )
          goto out_of_space;
        int_str( val, &str[ i ], d );
        i += d;
        str[ i++ ] = ' ';
        uint_str( frac, &str[ i ], j );
        i += j;
        str[ i++ ] = '/';
        uint_str( div, &str[ i ], k );
        i += k;
        goto ok_dec;

      default:
        goto bad_dec;
    }
  }
  if ( hint >= MD_DEC_LOGp10_1 ) {
    i = int_digs( val );
    if ( i + ( hint - MD_DEC_LOGp10_1 ) + 1 + 3 > len )
      goto out_of_space;
    int_str( val, str, i );
    for ( ; hint >= MD_DEC_LOGp10_1; hint-- )
      str[ i++ ] = '0';
    str[ i++ ] = '.';
    str[ i++ ] = '0';
    goto ok_dec;
  }
  if ( hint <= MD_DEC_LOGn10_1 ) {
    size_t n, m;
    i = int_digs( val );
    if ( i > len )
      goto out_of_space;
    int_str( val, str, i );
    n = i;
    i = 0;
    if ( val < 0 ) {
      n -= 1;
      i  = 1;
    }
    m = -(hint + 10);
    if ( m > n ) {
      m -= n;
      if ( i + m + n > len )
        goto out_of_space;
      ::memmove( &str[ i + m ], &str[ i ], n );
      ::memset( &str[ i ], '0', m );
      n += m;
      m = -(hint + 10);
    }
    /* 105221.60000000004
     *        |-- m ----|
     * |--- n ----------| */
    if ( n == m ) {
      if ( i + 2 + n > len )
        goto out_of_space;
      ::memmove( &str[ i + 2 ], &str[ i ], n );
      n += 2;
      str[ i ] = '0';
      str[ i + 1 ] = '.';
      i += n;
      goto ok_dec;
    }
    else {
      n -= m;
      if ( i + n + m + 1 > len )
        goto out_of_space;
      ::memmove( &str[ i + n + 1 ], &str[ i + n ], m );
      str[ i + n ] = '.';
      i += n + m + 1;
    ok_dec:;
      if ( i < len )
        str[ i ] = '\0';
      return i;
    }
  }
  /* other hits are invalid */
bad_dec:;
out_of_space:;
  if ( len > 0 )
    str[ 0 ] = '\0';
  return 0;
}

size_t
MDStamp::get_string( char *str,  size_t len ) const noexcept
{
  MDDecimal d;

  d.ival = (int64_t) this->stamp;
  d.hint = MD_DEC_INTEGER;
  switch ( this->resolution ) {
    case MD_RES_MILLISECS: d.hint = MD_DEC_LOGn10_3; break;
    case MD_RES_MICROSECS: d.hint = MD_DEC_LOGn10_6; break;
    case MD_RES_NANOSECS:  d.hint = MD_DEC_LOGn10_9; break;
    case MD_RES_MINUTES:   d.ival *= 60; break;
    case MD_RES_NULL:      d.hint = MD_DEC_NULL; break;
  }
  return d.get_string( str, len, false );
}

extern "C" {
#if defined( _MSC_VER ) || defined( __MINGW32__ )
void md_localtime( time_t t, struct tm *tmbuf ) {
  ::localtime_s( tmbuf, &t );
}
void md_gmtime( time_t t, struct tm *tmbuf ) {
  ::gmtime_s( tmbuf, &t );
}
time_t md_mktime( struct tm *tmbuf ) {
  return ::mktime( tmbuf );
}
time_t md_timegm( struct tm *tmbuf ) {
  return ::_mkgmtime( tmbuf );
}
#else
void md_localtime( time_t t, struct tm *tmbuf ) {
  ::localtime_r( &t, tmbuf );
}
void md_gmtime( time_t t, struct tm *tmbuf ) {
  ::gmtime_r( &t, tmbuf );
}
time_t md_mktime( struct tm *tmbuf ) {
  return ::mktime( tmbuf );
}
time_t md_timegm( struct tm *tmbuf ) {
  return ::timegm( tmbuf );
}
#endif
}

int
MDTime::parse( const char *fptr,  const size_t fsize ) noexcept
{
  uint64_t ms = 0, val = 0;
  uint32_t digits = 0, colon = 0, dot = 0;
  bool has_digit = false;
  this->zero();
  for ( size_t i = 0; i < fsize && fptr[ i ] != 0; i++ ) {
    if ( fptr[ i ] >= '0' && fptr[ i ] <= '9' ) {
      val = ( val * 10 ) + ( fptr[ i ] - '0' );
      digits++;
      has_digit = true;
    }
    else if ( fptr[ i ] == ':' ) {
      if ( val > 60 )
        goto bad_time;
      switch ( colon++ ) {
        case 0: this->hour   = (uint8_t) val; break; /* [05]:01:58.384 */
        case 1: this->minute = (uint8_t) val; break; /* 05:[01]:58.384 */
        case 2: this->sec    = (uint8_t) val; break; /* 05:01:[58]:384 */
        default: break;
      }
      val = 0;
      digits = 0;
    }
    else if ( fptr[ i ] == '.' ) { /* 05:01:[58].384 */
      if ( dot == 0 && colon == 2 ) /* 01:02:03.456 */
        this->sec = (uint8_t) val;
      else if ( dot == 0 && colon == 1 ) /* 01:02.456 */
        this->minute = (uint8_t) val;
      else if ( colon == 0 ) /* 1300000000.000 timestamp */
        ms = val;
      dot++;
      val = 0;
      digits = 0;
    }
  }
  if ( dot == 1 || colon == 3 ) {
    while ( digits % 3 != 0 ) {
      val *= 10;
      digits++;
    }
    if ( digits <= 9 ) {
      this->fraction = (uint32_t) val;
      this->resolution = digits / 3;
    }
    if ( ms != 0 ) {
      val = ms;
      goto get_mstime;
    }
  }
  else if ( colon == 2 ) {
    this->sec = (uint8_t) val;
    /* resolution already set */
  }
  else if ( colon == 1 ) {
    this->minute = (uint8_t) val;
    this->resolution = MD_RES_MINUTES;
  }
  /* a single number */
  else {
  get_mstime:;
    while ( val > (uint64_t) 1300000000U * (uint64_t) 1000 )
      val /= 1000;
    if ( val > 1300000000U && val <= 0xffffffffU ) {
      time_t t = (time_t) val;
      struct tm tmbuf;
      md_localtime( t, &tmbuf );

      this->hour   = tmbuf.tm_hour;
      this->minute = tmbuf.tm_min;
      this->sec    = tmbuf.tm_sec;
    }
  }
  if ( this->hour <= 24 && this->minute <= 60 && this->sec <= 60 ) {
    if ( ! has_digit )
      this->resolution |= MD_RES_NULL;
    return 0;
  }
bad_time:;
  return Err::BAD_TIME;
}

static inline bool test_date_sep( char b,  char x,  char y ) {
  return b == x || b == y;
}

static inline bool day_is_one( uint32_t m,  uint32_t y,  uint32_t &d ) {
  d = ( m != 0 && y != 0 ) ? 1 : 0;
  return true;
}

static bool get_current_year( uint32_t m,  uint32_t d,  uint32_t &y ) {
  static time_t when;
  static uint32_t yr = 2015;
  static uint32_t mon, day;
  if ( m == 0 && d == 0 ) {
    y = 0;
    return true;
  }
  time_t now = time( 0 );
  if ( now < when || now - when > 60000 ) {
    struct tm tmbuf;
    md_localtime( now, &tmbuf );

    yr   = tmbuf.tm_year + 1900;
    mon  = tmbuf.tm_mon + 1;
    day  = tmbuf.tm_mday;
    when = now;
  }
  y = yr;
  if ( m == 12 && d == 31 ) {
    if ( mon == 1 && day == 1 )
      y--;
  }
  return true;
}

static inline bool parse2digits( const char *p,  uint32_t &d ) {
  /* allow spaces for null */
  if ( p[ 0 ] == ' ' && p[ 1 ] == ' ' )
    return true;
  /* 2 nums or 1 num and a space */
  for ( uint32_t i = 0; i < 2; i++ ) {
    if ( p[ i ] >= '0' && p[ i ] <= '9' )
      d = ( d * 10 ) + ( p[ i ] - '0' );
    else if ( p[ i ] != ' ' )
      return false;
  }
  return true;
}

static bool parse_month( const char *p,  uint32_t n,  uint32_t &m ) {
  m = 0;
  if ( n == 2 ) {
    if ( parse2digits( p, m ) ) {
      if ( m == 0 || ( m >= 1 && m <= 12 ) )
        return true;
    }
  }
  else if ( n == 3 ) {
    static const char x[] = "JFMAMJJASOND";
    static const char y[] = "AEAPAUUUECOE";
    static const char z[] = "NBRRYNLGPTVC";

    for ( uint32_t i = 0; i < 12; i++ ) {
      if ( ( x[ i ] == p[ 0 ] || x[ i ] + ' ' == p[ 0 ] ) && /* upper, lower */
           ( y[ i ] == p[ 1 ] || y[ i ] + ' ' == p[ 1 ] ) &&
           ( z[ i ] == p[ 2 ] || z[ i ] + ' ' == p[ 2 ] ) ) {
        m = i + 1;
        return true;
      }
    }
    /* null */
    if ( p[ 0 ] == ' ' && p[ 1 ] == ' ' && p[ 2 ] == ' ' )
      return true;
  }
  return false;
}

static bool parse_day( const char *p,  uint32_t n,  uint32_t &d ) {
  d = 0;
  if ( n == 2 ) {
    if ( parse2digits( p, d ) ) {
      /* null or valid */
      if ( d == 0 || ( d >= 1 && d <= 31 ) )
        return true;
    }
  }
  else if ( n == 1 ) {
    if ( p[ 0 ] >= '0' && p[ 0 ] <= '9' ) {
      d = ( p[ 0 ] - '0' );
      return true;
    }
  }
  return false;
}

static bool parse_year( const char *p,  uint32_t n,  uint32_t &y ) {
  y = 0;
  if ( n == 2 || n == 4 ) {
    uint32_t i;
    for ( i = 0; i < n; i++ )
      if ( p[ i ] != ' ' )
        break;
    if ( i == n ) /* all spaces, null */
      return true;
    for ( i = 0; i < n; i++ ) {
      if ( p[ i ] >= '0' && p[ i ] <= '9' )
        y = ( y * 10 ) + ( p[ i ] - '0' );
      else
        return false;
    }
    if ( n == 2 ) {
      if ( y < 70 )
        y += 2000;
      else
        y += 1900;
      return true;
    }
    if ( y == 0 )
      return true;
    if ( y >= 1000 && y <= 9999 )
      return true;
  }
  return false;
}

static bool parse_excel_date( const char *p,  uint32_t n,  uint32_t &m,
                              uint32_t &d,  uint32_t &y ) {
  time_t t = 0;
  m = d = y = 0;

  for ( uint32_t i = 0; i < n; i++ ) {
    if ( p[ i ] >= '0' && p[ i ] <= '9' )
      t = ( t * 10 ) + ( p[ i ] - '0' );
    else
      return false;
  }
  if ( t < 25569 )
    return false;
  t = ( t - 25569 ) * 86400;

  struct tm tmbuf;
  md_localtime( t, &tmbuf );
  y = tmbuf.tm_year + 1900;
  m = tmbuf.tm_mon + 1;
  d = tmbuf.tm_mday;
  return true;
}

int
MDDate::parse( const char *fptr,  const size_t fsize ) noexcept
{
  uint32_t m, d, yr;
  uint32_t sz, sz2;
  bool success;

  this->zero();
  /* scan for nul char then eat white space at end */
  for ( sz = 0; sz < fsize && fptr[ sz ] != 0; sz++ )
    ;
  for ( sz2 = sz; sz2 > 0 && fptr[ sz2 - 1 ] <= ' '; )
    sz2--;
  if ( sz2 != sz ) {
    if ( sz2 > 0 && fptr[ sz2 - 1 ] != '/' && fptr[ sz2 - 1 ] != '-' )
      sz = sz2;
  }
  success = false;

try_again:;
  m = d = yr = 0;
  switch ( sz ) {
    case 5: /* Feb14, 12345 (excel date) */
      /* Feb14 */
      success = ( parse_month( fptr, 3, m ) &&
                  parse_year( &fptr[ 3 ], 2, yr ) &&
                  day_is_one( m, yr, d ) );
      /* 12345 */
      if ( ! success )
        success = parse_excel_date( fptr, 5, m, d, yr );
      break;
    case 6: /* May 15 */
      success = ( fptr[ 3 ] == ' ' &&
                  parse_month( fptr, 3, m ) &&
                  parse_year( &fptr[ 4 ], 2, yr ) );
      if ( success )
        d = 1;
      break;
    case 7: /* 02APR13 */
      /* 02APR13 */
      success = ( parse_day( fptr, 2, d ) &&
                  parse_month( &fptr[ 2 ], 3, m ) &&
                  parse_year( &fptr[ 5 ], 2, yr ) );
      break;
    case 8: /* 01/01/95, JAN 2000, 20190601, 3 Jun 21, Jun 3 21 */
      /* 01/01/95 */
      if ( fptr[ 2 ] == '/' && fptr[ 5 ] == '/' )
        success = ( parse_month( fptr, 2, m ) &&
                    parse_day( &fptr[ 3 ], 2, d ) &&
                    parse_year( &fptr[ 6 ], 2, yr ) );
      /* JAN 2000 */
      if ( ! success && fptr[ 3 ] == ' ' )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_year( &fptr[ 4 ], 4, yr ) &&
                    day_is_one( m, yr, d ) );
      /* 3 Jun 21 */
      if ( ! success && test_date_sep( fptr[ 1 ], '-', ' ' ) &&
                        test_date_sep( fptr[ 5 ], '-', ' ' ) )
        success = ( parse_day( fptr, 1, d ) &&
                    parse_month( &fptr[ 2 ], 3, m ) &&
                    parse_year( &fptr[ 6 ], 2, yr ) );
      /* Jun 3 21 */
      if ( ! success && test_date_sep( fptr[ 3 ], '-', ' ' ) &&
                        test_date_sep( fptr[ 5 ], '-', ' ' ) )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_day( &fptr[ 4 ], 1, d ) &&
                    parse_year( &fptr[ 6 ], 2, yr ) );
      /* 20190601 */
      if ( ! success )
        success = ( parse_year( fptr, 4, yr ) &&
                    parse_month( &fptr[ 4 ], 2, m ) &&
                    parse_day( &fptr[ 6 ], 2, d ) );
      break;
    case 9: /* 13-Jul-12, 13 Jul 12, 18Apr2014, Jul 13 12 */
      /* 13-Jul-12, 13 Jul 12 */
      if ( test_date_sep( fptr[ 2 ], '-', ' ' ) &&
           test_date_sep( fptr[ 6 ], '-', ' ' ) )
        success = ( parse_day( fptr, 2, d ) &&
                    parse_month( &fptr[ 3 ], 3, m ) &&
                    parse_year( &fptr[ 7 ], 2, yr ) );
      /* 18Apr2014 */
      if ( ! success )
        success = ( parse_day( fptr, 2, d ) &&
                    parse_month( &fptr[ 2 ], 3, m ) &&
                    parse_year( &fptr[ 5 ], 4, yr ) );
      /* Jul 13 12 */
      if ( ! success &&
           test_date_sep( fptr[ 3 ], '-', ' ' ) &&
           test_date_sep( fptr[ 6 ], '-', ' ' ) )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_day( &fptr[ 4 ], 2, d ) &&
                    parse_year( &fptr[ 7 ], 2, yr ) );
      break;
    case 10: /* 04/23/2014, 2014/04/08, 2014-09-04, 21 08 2014, Mon Jun 08 */
      /* 04/23/2014, 04-23-2014 */
      if ( test_date_sep( fptr[ 2 ], '/', '-' ) &&
           test_date_sep( fptr[ 5 ], '/', '-' ) )
        success = ( parse_month( fptr, 2, m ) &&
                    parse_day( &fptr[ 3 ], 2, d ) &&
                    parse_year( &fptr[ 6 ], 4, yr ) );
      /* 2014-09-04, 2014/09/04 */
      if ( ! success && test_date_sep( fptr[ 4 ], '-', '/' ) &&
                        test_date_sep( fptr[ 7 ], '-', '/' ) )
        success = ( parse_year( fptr, 4, yr ) &&
                    parse_month( &fptr[ 5 ], 2, m ) &&
                    parse_day( &fptr[ 8 ], 2, d ) );
      /* 21 08 2014 */
      if ( ! success && fptr[ 2 ] == ' ' &&
                        fptr[ 5 ] == ' ' )
        success = ( parse_day( fptr, 2, d ) &&
                    parse_month( &fptr[ 3 ], 2, m ) &&
                    parse_year( &fptr[ 6 ], 4, yr ) );
      /* Mon Jun 08 */
      if ( ! success && fptr[ 3 ] == ' ' &&
                        fptr[ 7 ] == ' ' )
        success = ( parse_month( &fptr[ 4 ], 3, m ) &&
                    parse_day( &fptr[ 8 ], 2, d ) &&
                    get_current_year( m, d, yr ) );
      /* 2013 Jul 2 */
      if ( ! success && test_date_sep( fptr[ 4 ], ' ', '-' ) &&
                        test_date_sep( fptr[ 8 ], ' ', '-' ) )
        success = ( parse_year( fptr, 4, yr ) &&
                    parse_month( &fptr[ 5 ], 3, m ) &&
                    parse_day( &fptr[ 9 ], 1, d ) );
      break;
    case 11: /* 08 JUL 2013, 31-Aug-2014, JUL 08 2013, Aug-31-2013,
                06/01 05:36 */
      /* 08 JUL 2013, 31-Aug-2014 */
      if ( test_date_sep( fptr[ 2 ], ' ', '-' ) &&
           test_date_sep( fptr[ 6 ], ' ', '-' ) )
        success = ( parse_day( fptr, 2, d ) &&
                    parse_month( &fptr[ 3 ], 3, m ) &&
                    parse_year( &fptr[ 7 ], 4, yr ) );
      /* JUL 08 2013, Aug-31-2013 */
      if ( ! success && test_date_sep( fptr[ 3 ], ' ', '-' ) &&
                        test_date_sep( fptr[ 6 ], ' ', '-' ) )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_day( &fptr[ 4 ], 2, d ) &&
                    parse_year( &fptr[ 7 ], 4, yr ) );
      /* 06/01 05:36 */
      if ( ! success && test_date_sep( fptr[ 2 ], '/', '-' ) &&
                        fptr[ 5 ] == ' ' && fptr[ 8 ] == ':' )
        success = ( parse_month( fptr, 2, m ) &&
                    parse_day( &fptr[ 3 ], 2, d ) &&
                    get_current_year( m, d, yr ) );
      /* Jul 1, 2013 */
      if ( ! success && fptr[ 3 ] == ' ' &&
                        fptr[ 5 ] == ',' && fptr[ 6 ] == ' ' )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_day( &fptr[ 4 ], 1, d ) &&
                    parse_year( &fptr[ 7 ], 4, yr ) );
      /* 2013 Jul 28 */
      if ( ! success && test_date_sep( fptr[ 4 ], ' ', '-' ) &&
                        test_date_sep( fptr[ 8 ], ' ', '-' ) )
        success = ( parse_year( fptr, 4, yr ) &&
                    parse_month( &fptr[ 5 ], 3, m ) &&
                    parse_day( &fptr[ 9 ], 2, d ) );
      break;
    case 12: /* Jul 11, 2013 */
      if ( fptr[ 3 ] == ' ' && fptr[ 6 ] == ',' && fptr[ 7 ] == ' ' )
        success = ( parse_month( fptr, 3, m ) &&
                    parse_day( &fptr[ 4 ], 2, d ) &&
                    parse_year( &fptr[ 8 ], 4, yr ) );
      break;
    case 0: /* null */
      return 0;
  }
  if ( success ) {
    if ( d >= 1 && d <= 31 &&
         m >= 1 && m <= 12 &&
         yr >= 1000 && yr <= 9999 ) {
      this->day  = d;
      this->mon  = m;
      this->year = yr;
      return 0;
    }
    if ( d == 0 && m == 0 && ( yr == 0 || yr == 2000 ) )
      return 0;
    /* invalid range */
  }
  if ( ! success ) {
    /* eat white space */
    if ( fptr[ sz - 1 ] == ' ' ) {
      sz--;
      goto try_again;
    }
    if ( fptr[ 0 ] == ' ' ) {
      fptr++;
      sz--;
      goto try_again;
    }
  }
  return Err::BAD_DATE;
}

static const uint8_t _NO_SEP_ = 1;
static const char  * _EMPTY_  = NULL;

static size_t
cpy3( char *ptr,  size_t len,  uint32_t t1, const char *mon1,
      uint8_t sep,  uint32_t t2,  const char *mon2 = NULL,
      uint8_t sep2 = 0, uint32_t t3 = 0,  const char *mon3 = NULL )
{
  char * sav = NULL, tmp[ 16 ];
  size_t j = 0;

  if ( len < 16 ) {
    sav = ptr;
    ptr = tmp;
  }
  if ( mon1 != NULL ) {
    do {
      ptr[ j++ ] = (uint8_t) *mon1++;
    } while ( *mon1 != '\0' );
  }
  else {
    if ( t1 >= 1000 ) { /* if year */
      ptr[ j++ ] = ( ( t1 / 1000 ) % 10 ) + '0';
      ptr[ j++ ] = ( ( t1 / 100 ) % 10 ) + '0';
    }
    ptr[ j++ ] = ( ( t1 / 10 ) % 10 ) + '0';
    ptr[ j++ ] =   ( t1 % 10 )        + '0';
  }
  if ( sep > _NO_SEP_ )
    ptr[ j++ ] = sep;
  if ( mon2 != NULL ) {
    do {
      ptr[ j++ ] = (uint8_t) *mon2++;
    } while ( *mon2 != '\0' );
  }
  else {
    if ( t2 >= 1000 ) { /* if year */
      ptr[ j++ ] = ( ( t2 / 1000 ) % 10 ) + '0';
      ptr[ j++ ] = ( ( t2 / 100 ) % 10 ) + '0';
    }
    ptr[ j++ ] = ( ( t2 / 10 ) % 10 ) + '0';
    ptr[ j++ ] =   ( t2 % 10 )        + '0';
  }
  if ( sep2 != 0 ) {
    if ( sep2 > _NO_SEP_ )
      ptr[ j++ ] = sep2;
    if ( mon3 != NULL ) {
      do {
        ptr[ j++ ] = (uint8_t) *mon3++;
      } while ( *mon3 != '\0' );
    }
    else {
      if ( t3 >= 1000 ) { /* if year */
        ptr[ j++ ] = ( ( t3 / 1000 ) % 10 ) + '0';
        ptr[ j++ ] = ( ( t3 / 100 ) % 10 ) + '0';
      }
      ptr[ j++ ] = ( ( t3 / 10 ) % 10 ) + '0';
      ptr[ j++ ] =   ( t3 % 10 )        + '0';
    }
  }
  if ( sav != NULL ) {
    if ( j > len - 1 )
      j = len - 1;
    ::memcpy( sav, tmp, j );
    sav[ j ] = '\0';
  }
  else {
    ptr[ j ] = '\0';
  }
  return j;
}

size_t
MDTime::get_string( char *str,  size_t len ) const noexcept
{
  uint32_t places, digit;
  size_t   n = 0;

  if ( len <= 1 ) {
    if ( len == 1 )
      str[ 0 ] = 0;
    return 0;
  }
  if ( this->is_null() ) {
    const char * p;
    if ( this->res() == MD_RES_MINUTES )
      p = "  :  ";
    else
      p = "  :  :  ";
    for ( ; *p != '\0'; p++ ) {
      if ( n < len - 1 )
        str[ n++ ] = *p;
    }
    str[ n ] = '\0';
    return n;
  }
  if ( this->res() == MD_RES_MINUTES )
    return cpy3( str, len, this->hour, _EMPTY_, ':', this->minute );
  n = cpy3( str, len, this->hour, _EMPTY_, ':', this->minute, _EMPTY_, ':',
            this->sec );
  switch ( this->res() ) {
    case MD_RES_MILLISECS: places = 1000; break;
    case MD_RES_MICROSECS: places = 1000 * 1000; break;
    case MD_RES_NANOSECS:  places = 1000 * 1000 * 1000; break;
    default:               places = 0; break;
  }
  if ( places != 0 ) {
    if ( n < len - 1 ) {
      str[ n++ ] = '.';
      while ( n < len - 1 ) {
        digit = this->fraction % places;
        places /= 10;
        digit /= places;
        str[ n++ ] = '0' + (char) digit;
        if ( places == 1 )
          break;
      }
      str[ n ] = '\0';
    }
  }
  return n;
}


size_t
MDDate::get_string( char *str,  size_t len,  MDDateFormat fmt ) const noexcept
{
  static const char *MONTH_STR[] = {
   0, "JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC" };

  uint32_t t1 = 0, t2 = 0, t3 = 0;
  const char *mon1 = _EMPTY_, *mon2 = _EMPTY_, *mon3 = _EMPTY_;
  uint8_t sep1, sep2;

  if ( len <= 1 ) {
    if ( len == 1 )
      str[ 0 ] = 0;
    return 0;
  }
  if ( ( fmt & MD_DATE_FMT_dmy ) == MD_DATE_FMT_dmy ) {
    t1 = this->day; t2 = this->mon; t3 = this->year;
    if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
      mon2 = MONTH_STR[ this->mon ];
    if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
      t3 %= 100;
  }
  else if ( ( fmt & MD_DATE_FMT_mdy ) == MD_DATE_FMT_mdy ) {
    t1 = this->mon; t2 = this->day; t3 = this->year;
    if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
      mon1 = MONTH_STR[ this->mon ];
    if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
      t3 %= 100;
  }
  else if ( ( fmt & MD_DATE_FMT_ymd ) == MD_DATE_FMT_ymd ) {
    t1 = this->year; t2 = this->mon; t3 = this->day;
    if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
      mon2 = MONTH_STR[ this->mon ];
    if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
      t1 %= 100;
  }
  else {
    if ( ( fmt & MD_DATE_FMT_dd1 ) != 0 ) t1 = this->day;
    else if ( ( fmt & MD_DATE_FMT_dd2 ) != 0 ) t2 = this->day;
    else if ( ( fmt & MD_DATE_FMT_dd3 ) != 0 ) t3 = this->day;
    if ( ( fmt & MD_DATE_FMT_mm1 ) != 0 ) {
      t1 = this->mon;
      if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
        mon1 = MONTH_STR[ this->mon ];
    }
    else if ( ( fmt & MD_DATE_FMT_mm2 ) != 0 ) {
      t2 = this->mon;
      if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
        mon2 = MONTH_STR[ this->mon ];
    }
    else if ( ( fmt & MD_DATE_FMT_mm3 ) != 0 ) {
      t3 = this->mon;
      if ( ( fmt & MD_DATE_FMT_MMM ) == MD_DATE_FMT_MMM )
        mon3 = MONTH_STR[ this->mon ];
    }
    if ( ( fmt & MD_DATE_FMT_yy1 ) != 0 ) {
      t1 = this->year;
      if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
        t1 %= 100;
    }
    else if ( ( fmt & MD_DATE_FMT_yy2 ) != 0 ) {
      t2 = this->year;
      if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
        t2 %= 100;
    }
    else if ( ( fmt & MD_DATE_FMT_yy3 ) != 0 ) {
      t3 = this->year;
      if ( ( fmt & MD_DATE_FMT_yyyy ) == 0 )
        t3 %= 100;
    }
  }
  if ( ( fmt & MD_DATE_FMT_SPACE ) == MD_DATE_FMT_SPACE )
    sep1 = ' ';
  else if ( ( fmt & MD_DATE_FMT_SLASH ) == MD_DATE_FMT_SLASH )
    sep1 = '/';
  else if ( ( fmt & MD_DATE_FMT_DASH ) == MD_DATE_FMT_DASH )
    sep1 = '-';
  else
    sep1 = _NO_SEP_;
  if ( t3 != 0 )
    sep2 = sep1;
  else
    sep2 = 0;
  if ( ( t1 | t2 ) == 0 ) {
    for ( size_t i = 0; ; i++ ) {
      if ( i == len - 1 || i == 11 ) {
        str[ i ] = '\0';
        return i;
      }
      str[ i ] = ' ';
    }
  }
  return cpy3( str, len, t1, mon1, sep1, t2, mon2, sep2, t3, mon3 );
}

uint64_t
MDDate::to_utc( bool is_gm_time ) noexcept
{
  struct tm tmbuf;
  time_t t;

  ::memset( &tmbuf, 0, sizeof( tmbuf ) );
  if ( this->year == 0 || this->mon == 0 || this->day == 0 ) {
    t = ::time( NULL );
    if ( is_gm_time )
      md_gmtime( t, &tmbuf );
    else
      md_localtime( t, &tmbuf );
    tmbuf.tm_sec = 0;
    tmbuf.tm_min = 0;
    tmbuf.tm_hour = 0;
    tmbuf.tm_wday = 0;
    tmbuf.tm_yday = 0;
  }
  if ( this->year != 0 )
    tmbuf.tm_year  = this->year - 1900;
  if ( this->mon != 0 )
    tmbuf.tm_mon   = this->mon - 1; /* range 1 -> 12 */
  if ( this->day != 0 )
    tmbuf.tm_mday  = this->day;
  tmbuf.tm_isdst = -1;

  if ( is_gm_time )
    t = md_timegm( &tmbuf );
  else
    t = md_mktime( &tmbuf );
  if ( t == -1 )
    return 0;
  return (uint64_t) t;
}

uint64_t
MDTime::to_utc( MDDate *dt,  bool is_gm_time ) noexcept
{
  struct tm tmbuf;
  time_t t;
  bool   next_day = false;

  ::memset( &tmbuf, 0, sizeof( tmbuf ) );

  if ( dt == NULL || dt->year == 0 || dt->mon == 0 || dt->day == 0 ) {
    t = ::time( NULL );
    if ( is_gm_time )
      md_gmtime( t, &tmbuf );
    else
      md_localtime( t, &tmbuf );

    if ( dt == NULL && this->hour < tmbuf.tm_hour )
      next_day = true;
  }
  else {
    tmbuf.tm_isdst = -1;
  }
  if ( dt != NULL && dt->year != 0 )
    tmbuf.tm_year  = dt->year - 1900;
  if ( dt != NULL && dt->mon != 0 )
    tmbuf.tm_mon   = dt->mon - 1; /* range 1 -> 12 */
  if ( dt != NULL && dt->day != 0 )
    tmbuf.tm_mday  = dt->day;

  tmbuf.tm_hour = this->hour;
  tmbuf.tm_min  = this->minute;
  tmbuf.tm_sec  = this->sec;
  if ( is_gm_time )
    t = md_timegm( &tmbuf );
  else
    t = md_mktime( &tmbuf );
  if ( t == -1 )
    return 0;
  if ( next_day )
    return t + ( 24 * 60 * 60 );
  return (uint64_t) t;
}

extern "C" {
// MDName functions
void md_name_zero(MDName_t *name) { ((rai::md::MDName *)name)->zero(); }
bool md_name_equals_name(const MDName_t *name, const MDName_t *nm) { return ((rai::md::MDName *)name)->equals(*(rai::md::MDName *)nm); }
bool md_name_equals(const MDName_t *name, const char *fname, size_t len2) { return ((rai::md::MDName *)name)->equals(fname, len2); }
// MDDecimal functions
void md_decimal_set(MDDecimal_t *dec, int64_t i, int8_t h) { ((rai::md::MDDecimal *)dec)->set(i, h); }
int md_decimal_parse_len(MDDecimal_t *dec, const char *s, const size_t fsize) { return ((rai::md::MDDecimal *)dec)->parse(s, fsize); }
int md_decimal_parse(MDDecimal_t *dec, const char *s) { return ((rai::md::MDDecimal *)dec)->parse(s); }
void md_decimal_zero(MDDecimal_t *dec) { ((rai::md::MDDecimal *)dec)->zero(); }
int md_decimal_get_real(MDDecimal_t *dec, double *val) { return ((rai::md::MDDecimal *)dec)->get_real(*val); }
int md_decimal_get_integer(MDDecimal_t *dec, int64_t *val) { return ((rai::md::MDDecimal *)dec)->get_integer(*val); }
int md_decimal_degrade(MDDecimal_t *dec) { return ((rai::md::MDDecimal *)dec)->degrade(); }
void md_decimal_set_real(MDDecimal_t *dec, double fval) { ((rai::md::MDDecimal *)dec)->set_real(fval); }
int md_decimal_get_decimal(MDDecimal_t *dec, MDReference_t *mref) { return ((rai::md::MDDecimal *)dec)->get_decimal(*(rai::md::MDReference *)mref); }
size_t md_decimal_get_string(MDDecimal_t *dec, char *str, size_t len, bool expand_fractions) { return ((rai::md::MDDecimal *)dec)->get_string(str, len, expand_fractions); }
// MDTime functions
void md_time_set(MDTime_t *time, uint8_t h, uint8_t m, uint8_t s, uint32_t f, uint8_t r) { ((rai::md::MDTime *)time)->set(h, m, s, f, r); }
int md_time_parse(MDTime_t *time, const char *fptr, const size_t fsize) { return ((rai::md::MDTime *)time)->parse(fptr, fsize); }
void md_time_zero(MDTime_t *time) { ((rai::md::MDTime *)time)->zero(); }
size_t md_time_get_string(MDTime_t *time, char *str, size_t len) { return ((rai::md::MDTime *)time)->get_string(str, len); }
bool md_time_is_null(MDTime_t *time) { return ((rai::md::MDTime *)time)->is_null(); }
uint8_t md_time_res(MDTime_t *time) { return ((rai::md::MDTime *)time)->res(); }
const char *md_time_res_string(MDTime_t *time) { return ((rai::md::MDTime *)time)->res_string(); }
int md_time_get_time(MDTime_t *time, MDReference_t *mref) { return ((rai::md::MDTime *)time)->get_time(*(rai::md::MDReference *)mref); }
uint64_t md_time_to_utc(MDTime_t *time, MDDate_t *dt, bool is_gm_time) { return ((rai::md::MDTime *)time)->to_utc((rai::md::MDDate *)dt, is_gm_time); }
// MDDate functions
void md_date_set(MDDate_t *date, uint16_t y, uint8_t m, uint8_t d) { ((rai::md::MDDate *)date)->set(y, m, d); }
void md_date_zero(MDDate_t *date) { ((rai::md::MDDate *)date)->zero(); }
size_t md_date_get_string(MDDate_t *date, char *str, size_t len, MDDateFormat fmt) { return ((rai::md::MDDate *)date)->get_string(str, len, fmt); }
bool md_date_is_null(MDDate_t *date) { return ((rai::md::MDDate *)date)->is_null(); }
/*int md_date_parse_format(const char *s, MDDateFormat *fmt) { return rai::md::MDDate::parse_format(s, *fmt); }*/
int md_date_parse(MDDate_t *date, const char *fptr, const size_t flen) { return ((rai::md::MDDate *)date)->parse(fptr, flen); }
int md_date_get_date(MDDate_t *date, MDReference_t *mref) { return ((rai::md::MDDate *)date)->get_date(*(rai::md::MDReference *)mref); }
uint64_t md_date_to_utc(MDDate_t *date, bool is_gm_time) { return ((rai::md::MDDate *)date)->to_utc(is_gm_time); }
// MDReference functions
void md_reference_zero(MDReference_t *ref) { ((rai::md::MDReference *)ref)->zero(); }
void md_reference_set(MDReference_t *ref, void *fp, size_t sz, MDType ft, MDEndian end) { ((rai::md::MDReference *)ref)->set(fp, sz, ft, end); }
void md_reference_set_string(MDReference_t *ref, const char *p, size_t len) { ((rai::md::MDReference *)ref)->set_string(p, len); }
void md_reference_set_string_z(MDReference_t *ref, const char *p) { ((rai::md::MDReference *)ref)->set_string(p); }
bool md_reference_equals(MDReference_t *ref, MDReference_t *mref) { return ((rai::md::MDReference *)ref)->equals(*(rai::md::MDReference *)mref); }

void md_output_init( MDOutput_t **mout ) { *mout = (MDOutput_t *) new ( ::malloc( sizeof( MDOutput ) ) ) MDOutput(); } 
void md_output_release( MDOutput_t *mout ) { delete reinterpret_cast<MDOutput *>( mout ); }
int md_output_open( MDOutput_t *mout,  const char *fn,  const char *mode ) { return reinterpret_cast<MDOutput *>(mout)->open( fn, mode ); }
int md_output_close( MDOutput_t *mout ) { return reinterpret_cast<MDOutput *>(mout)->close(); }
int md_output_flush( MDOutput_t *mout ) { return reinterpret_cast<MDOutput *>(mout)->flush(); }
size_t md_output_write( MDOutput_t *mout,  const void *buf,  size_t buflen ) { return reinterpret_cast<MDOutput *>(mout)->write( buf, buflen ); }
int md_output_puts( MDOutput_t *mout,  const char *s ) { return reinterpret_cast<MDOutput *>(mout)->puts( s ); }
int md_output_printf( MDOutput_t *mout,  const char *fmt, ... )
{
  MDOutput *p = reinterpret_cast<MDOutput *>(mout);
  FILE * fp = ( p->filep == NULL ? stdout : (FILE *) p->filep );
  va_list ap;
  int n;
  va_start( ap, fmt );
  n = vfprintf( fp, fmt, ap );
  va_end( ap );
  return n;
}
int md_output_printe( MDOutput_t *mout,  const char *fmt, ... )
{
  MDOutput *p = reinterpret_cast<MDOutput *>(mout);
  FILE * fp = ( p->filep == NULL ? stderr : (FILE *) p->filep );
  va_list ap;
  int n;
  va_start( ap, fmt );
  n = vfprintf( fp, fmt, ap );
  va_end( ap );
  if ( p->filep == NULL )
    fflush( stderr );
  return n;
}
int md_output_print_hex( MDOutput_t *mout,  const void *buf,  size_t buflen ) { return reinterpret_cast<MDOutput *>(mout)->print_hex( buf, buflen ); }
int md_output_print_msg_hex( MDOutput_t *mout,  struct MDMsg_s *msg ) { return reinterpret_cast<MDOutput *>(mout)->print_hex( (MDMsg *) msg ); }

MDFieldIter_t *md_field_iter_copy( MDFieldIter_t *iter ) { return (MDFieldIter_t *) static_cast<MDFieldIter *>(iter)->copy(); }
int md_field_iter_get_name( MDFieldIter_t *iter,  MDName_t *name ) { return static_cast<MDFieldIter *>(iter)->get_name( *(MDName *) name ); }
int md_field_iter_copy_name( MDFieldIter_t *iter,  char *name,  size_t *name_len,  MDFid *fid ) { return static_cast <MDFieldIter *>(iter)->copy_name( name, *name_len, *fid ); }
int md_field_iter_get_reference( MDFieldIter_t *iter,  MDReference_t *mref ) { return static_cast<MDFieldIter *>(iter)->get_reference( *(MDReference *) mref ); }
int md_field_iter_get_hint_reference( MDFieldIter_t *iter,  MDReference_t *mref ) { return static_cast<MDFieldIter *>(iter)->get_hint_reference( *(MDReference *) mref ); }
int md_field_iter_get_enum( MDFieldIter_t *iter,  MDReference_t *mref, MDEnum_t *enu ) { return static_cast<MDFieldIter *>(iter)->get_enum( *(MDReference *) mref, *(MDEnum *) enu ); }
int md_field_iter_find( MDFieldIter_t *iter,  const char *name, size_t name_len, MDReference_t *mref ) { return static_cast<MDFieldIter *>(iter)->find( name, name_len, *(MDReference *) mref ); }
int md_field_iter_first( MDFieldIter_t *iter ) { return static_cast<MDFieldIter *>(iter)->first(); }
int md_field_iter_next( MDFieldIter_t *iter ) { return static_cast<MDFieldIter *>(iter)->next(); }
int md_field_iter_update( MDFieldIter_t *iter,  MDReference_t *mref ) { return static_cast<MDFieldIter *>(iter)->update( *(MDReference *) mref ); }
size_t md_field_iter_fname_string( MDFieldIter_t *iter, char *fname_buf,  size_t *fname_len ) { return static_cast<MDFieldIter *>(iter)->fname_string( fname_buf, *fname_len ); }
int md_field_iter_print( MDFieldIter_t *iter, MDOutput_t *out ) { return static_cast<MDFieldIter *>(iter)->print( (MDOutput *) out ); }
int md_field_iter_print_fmt( MDFieldIter_t *iter, MDOutput_t *out, int indent_newline, const char *fname_fmt, const char *type_fmt ) { return static_cast<MDFieldIter *>(iter)->print( (MDOutput *) out, indent_newline, fname_fmt, type_fmt ); }
#if 0
#endif
}
