#ifndef __rai_raimd__md_int_h__
#define __rai_raimd__md_int_h__

#include <math.h>

namespace rai {
namespace md {

template<class Int>
static inline Int to_ival( const void *val ) {
  Int i;
  ::memcpy( &i, val, sizeof( i ) );
  return i;
}

template<MDEndian end>
static inline uint16_t get_u16( const void *val ) {
  const uint8_t *u = (const uint8_t *) val;
  return end == MD_LITTLE ?
    ( ( (uint16_t) u[ 1 ] << 8 ) | ( (uint16_t) u[ 0 ] ) ) :
    ( ( (uint16_t) u[ 0 ] << 8 ) | ( (uint16_t) u[ 1 ] ) );
}

template<MDEndian end>
static inline int16_t get_i16( const void *val ) {
  return (int16_t) get_u16<end>( val );
}

template<MDEndian end>
static inline uint32_t get_u32( const void *val ) {
  const uint8_t *u = (const uint8_t *) val;
  return end == MD_LITTLE ?
    ( ( (uint32_t) u[ 3 ] << 24 ) | ( (uint32_t) u[ 2 ] << 16 ) |
      ( (uint32_t) u[ 1 ] << 8 )  |   (uint32_t) u[ 0 ] ) :
    ( ( (uint32_t) u[ 0 ] << 24 ) | ( (uint32_t) u[ 1 ] << 16 ) |
      ( (uint32_t) u[ 2 ] << 8 )  |   (uint32_t) u[ 3 ] );
}

template<MDEndian end>
static inline int32_t get_i32( const void *val ) {
  return (int32_t) get_u32<end>( val );
}

template<MDEndian end>
static inline float get_f32( const void *val ) {
  union { uint32_t u; float f; } x;
  x.u = get_u32<end>( val );
  return x.f;
}

template<MDEndian end>
static inline uint64_t get_u64( const void *val ) {
  const uint8_t *u = (const uint8_t *) val;
  return end == MD_LITTLE ?
    ( ( (uint64_t) u[ 7 ] << 56 ) | ( (uint64_t) u[ 6 ] << 48 ) |
      ( (uint64_t) u[ 5 ] << 40 ) | ( (uint64_t) u[ 4 ] << 32 ) |
      ( (uint64_t) u[ 3 ] << 24 ) | ( (uint64_t) u[ 2 ] << 16 ) |
      ( (uint64_t) u[ 1 ] << 8 )  |   (uint64_t) u[ 0 ] ) :
    ( ( (uint64_t) u[ 0 ] << 56 ) | ( (uint64_t) u[ 1 ] << 48 ) |
      ( (uint64_t) u[ 2 ] << 40 ) | ( (uint64_t) u[ 3 ] << 32 ) |
      ( (uint64_t) u[ 4 ] << 24 ) | ( (uint64_t) u[ 5 ] << 16 ) |
      ( (uint64_t) u[ 6 ] << 8 )  |   (uint64_t) u[ 7 ] );
}

template<MDEndian end>
static inline int64_t get_i64( const void *val ) {
  return (int64_t) get_u64<end>( val );
}

template<MDEndian end>
static inline double get_f64( const void *val ) {
  union { uint64_t u; double f; } x;
  x.u = get_u64<end>( val );
  return x.f;
}

template<class T>
static inline T get_uint( const void *val, MDEndian end ) {
  if ( sizeof( T ) == sizeof( uint16_t ) )
    return end == MD_LITTLE ? get_u16<MD_LITTLE>( val ): get_u16<MD_BIG>( val );
  if ( sizeof( T ) == sizeof( uint32_t ) )
    return end == MD_LITTLE ? get_u32<MD_LITTLE>( val ): get_u32<MD_BIG>( val );
  if ( sizeof( T ) == sizeof( uint64_t ) )
    return end == MD_LITTLE ? get_u64<MD_LITTLE>( val ): get_u64<MD_BIG>( val );
  return ((const uint8_t *) val)[ 0 ];
}

template<class T>
static inline T get_uint( const MDReference &mref ) {
  if ( mref.fsize == sizeof( uint16_t ) )
    return get_uint<uint16_t>( mref.fptr, mref.fendian );
  if ( mref.fsize == sizeof( uint32_t ) )
    return get_uint<uint32_t>( mref.fptr, mref.fendian );
  if ( mref.fsize == sizeof( uint64_t ) )
    return get_uint<uint64_t>( mref.fptr, mref.fendian );
  return get_uint<uint8_t>( mref.fptr, mref.fendian );
}

template<class T>
static inline T get_int( const void *val, MDEndian end ) {
  if ( sizeof( T ) == sizeof( int16_t ) )
    return end == MD_LITTLE ? get_i16<MD_LITTLE>( val ): get_i16<MD_BIG>( val );
  if ( sizeof( T ) == sizeof( int32_t ) )
    return end == MD_LITTLE ? get_i32<MD_LITTLE>( val ): get_i32<MD_BIG>( val );
  if ( sizeof( T ) == sizeof( int64_t ) )
    return end == MD_LITTLE ? get_i64<MD_LITTLE>( val ): get_i64<MD_BIG>( val );
  return ((const int8_t *) val)[ 0 ];
}

template<class T>
static inline T get_int( const MDReference &mref ) {
  if ( mref.fsize == sizeof( int16_t ) )
    return get_int<int16_t>( mref.fptr, mref.fendian );
  if ( mref.fsize == sizeof( int32_t ) )
    return get_int<int32_t>( mref.fptr, mref.fendian );
  if ( mref.fsize == sizeof( int64_t ) )
    return get_int<int64_t>( mref.fptr, mref.fendian );
  return get_int<int8_t>( mref.fptr, mref.fendian );
}

template<class T>
static inline T get_float( const void *val, MDEndian end ) {
  if ( sizeof( T ) == sizeof( float ) )
    return end == MD_LITTLE ? get_f32<MD_LITTLE>( val ): get_f32<MD_BIG>( val );
  if ( sizeof( T ) == sizeof( double ) )
    return end == MD_LITTLE ? get_f64<MD_LITTLE>( val ): get_f64<MD_BIG>( val );
  return 0;
}

template<class T>
static inline T get_float( const MDReference &mref ) {
  if ( mref.fsize == sizeof( float ) )
    return get_float<float>( mref.fptr, mref.fendian );
  if ( mref.fsize == sizeof( double ) )
    return get_float<double>( mref.fptr, mref.fendian );
  return 0;
}

static inline uint16_t parse_u16( const char *val,  char **end ) {
  return ::strtoul( val, end, 0 );
}
static inline int16_t parse_i16( const char *val,  char **end ) {
  return ::strtol( val, end, 0 );
}
static inline uint32_t parse_u32( const char *val,  char **end ) {
  return ::strtoul( val, end, 0 );
}
static inline int32_t parse_i32( const char *val,  char **end ) {
  return ::strtol( val, end, 0 );
}
static inline float parse_f32( const char *val,  char **end ) {
  return ::strtof( val, end );
}
static inline uint64_t parse_u64( const char *val,  char **end ) {
  return ::strtoull( val, end, 0 );
}
static inline int64_t parse_i64( const char *val,  char **end ) {
  return ::strtoll( val, end, 0 );
}
static inline double parse_f64( const char *val,  char **end ) {
  return ::strtod( val, end );
}

template<class T>
static inline T parse_int( const char *val,  char **end ) {
  if ( sizeof( T ) == sizeof( int16_t ) )
    return parse_i16( val, end );
  if ( sizeof( T ) == sizeof( int32_t ) )
    return parse_i32( val, end );
  if ( sizeof( T ) == sizeof( int64_t ) )
    return parse_i64( val, end );
  return 0;
}

template<class T>
static inline T parse_uint( const char *val,  char **end ) {
  if ( sizeof( T ) == sizeof( uint16_t ) )
    return parse_u16( val, end );
  if ( sizeof( T ) == sizeof( uint32_t ) )
    return parse_u32( val, end );
  if ( sizeof( T ) == sizeof( uint64_t ) )
    return parse_u64( val, end );
  return 0;
}

template<class T>
static inline T parse_float( const char *val,  char **end ) {
  if ( sizeof( T ) == sizeof( float ) )
    return parse_f32( val, end );
  if ( sizeof( T ) == sizeof( double ) )
    return parse_f64( val, end );
  return 0;
}

template<class T>
static inline int cvt_number( const MDReference &mref, T &val ) {
  switch ( mref.ftype ) {
    case MD_BOOLEAN:
    case MD_ENUM:
    case MD_UINT:   val = get_uint<uint64_t>( mref ); return 0;
    case MD_INT:    val = get_int<int64_t>( mref ); return 0;
    case MD_REAL:   val = get_float<double>( mref ); return 0;
    case MD_STRING: val = parse_u64( (char *) mref.fptr, NULL ); return 0;
    case MD_DECIMAL: {
      MDDecimal dec;
      double f;
      dec.get_decimal( mref );
      if ( dec.hint == MD_DEC_INTEGER )
        val = dec.ival;
      else {
        dec.get_real( f );
        val = f;
      }
      return 0;
    }
    default: break;
  }
  return Err::BAD_CVT_NUMBER;
}

static inline bool is_mask( uint32_t m ) { return ( m & ( m + 1 ) ) == 0; }

/* integer to string routines */
static inline uint64_t nega( int64_t v ) {
  if ( (uint64_t) v == ( (uint64_t) 1 << 63 ) )
    return ( (uint64_t) 1 << 63 );
  return (uint64_t) -v;
}

static inline size_t uint_digs( uint64_t v ) {
  for ( size_t n = 1; ; n += 4 ) {
    if ( v < 10 )    return n;
    if ( v < 100 )   return n + 1;
    if ( v < 1000 )  return n + 2;
    if ( v < 10000 ) return n + 3;
    v /= 10000;
  }
}

static inline size_t int_digs( int64_t v ) {
  if ( v < 0 ) return 1 + uint_digs( nega( v ) );
  return uint_digs( v );
}
/* does not null terminate (most strings have lengths, not nulls) */
static inline size_t uint_str( uint64_t v,  char *buf,  size_t len ) {
  for ( size_t pos = len; v >= 10; ) {
    const uint64_t q = v / 10, r = v % 10;
    buf[ --pos ] = '0' + r;
    v = q;
  }
  buf[ 0 ] = '0' + v;
  return len;
}

static inline size_t uint_str( uint64_t v,  char *buf ) {
  return uint_str( v, buf, uint_digs( v ) );
}

static inline size_t int_str( int64_t v,  char *buf,  size_t len ) {
  if ( v < 0 ) {
    buf[ 0 ] = '-';
    return 1 + uint_str( nega( v ), &buf[ 1 ], len - 1 );
  }
  return uint_str( v, buf, len );
}

static inline size_t int_str( int64_t v,  char *buf ) {
  return int_str( v, buf, int_digs( v ) );
}

static inline size_t float_str( double f,  char *buf ) {
  size_t places = 14,
         off    = 0;
  if ( isnan( f ) ) {
    if ( f < 0 )
      buf[ off++ ] = '-';
    ::strcpy( &buf[ off ], "NaN" );
    return off + 3;
  }
  if ( isinf( f ) ) {
    if ( f < 0 )
      buf[ off++ ] = '-';
    ::strcpy( &buf[ off ], "Inf" );
    return off + 3;
  }

  if ( f < 0.0 ) {
    buf[ off++ ] = '-';
    f = -f;
  }
  /* find the integer and the fraction + 1.0 */
  double       integral,
               tmp,
               p10,
               decimal;
  const double fraction      = modf( (double) f, &integral );
  uint64_t     integral_ival = (uint64_t) integral,
               decimal_ival;

  static const double powtab[] = {
    1.0, 10.0, 100.0, 1000.0, 10000.0,
    100000.0, 1000000.0, 10000000.0,
    100000000.0, 1000000000.0, 10000000000.0,
    100000000000.0, 1000000000000.0, 10000000000000.0,
    100000000000000.0
    };
  static const size_t ptsz = sizeof( powtab ) / sizeof( powtab[ 0 ] );
  if ( places < ptsz )
    p10 = powtab[ places ];
  else
    p10 = pow( (double) 10.0, (double) places );

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

  off = uint_str( integral_ival, buf );
  /* convert the decimal to 1ddddd, the 1 is replaced with a '.' below */
  decimal_ival = (uint64_t) decimal;

  while ( decimal_ival >= 10000 && decimal_ival % 10000 == 0 )
    decimal_ival /= 10000;
  while ( decimal_ival > 1 && decimal_ival % 10 == 0 )
    decimal_ival /= 10;
  /* at least one zero */
  if ( decimal_ival <= 2 ) {
    buf[ off++ ] = '.';
    buf[ off++ ] = '0';
  }
  else {
    size_t pt = off;
    off += int_str( decimal_ival, &buf[ off ] );
    buf[ pt ] = '.';
  }
  return off;
}

static inline char hexchar( uint8_t n ) {
  return (char) ( n <= 9 ? ( n + '0' ) : ( n - 10 + 'a' ) );
}
static inline char hexchar2( uint8_t n ) { /* upper case */
  return (char) ( n <= 9 ? ( n + '0' ) : ( n - 10 + 'A' ) );
}
static inline void hexstr32( uint32_t n,  char *s ) {
  int j = 0;
  for ( int i = 32; i > 0; ) {
    i -= 4;
    s[ j++ ] = hexchar2( ( n >> i ) & 0xf );
  }
}
static inline void hexstr64( uint64_t n,  char *s ) {
  int j = 0;
  for ( int i = 64; i > 0; ) {
    i -= 4;
    s[ j++ ] = hexchar2( ( n >> i ) & 0xf );
  }
}

static inline int to_string( const MDReference &mref, char *sbuf,
                             size_t &slen ) {
  switch ( mref.ftype ) {
    case MD_UINT:
      slen = uint_str( get_uint<uint64_t>( mref ), sbuf ); return 0;
    case MD_INT:
      slen = int_str( get_int<int64_t>( mref ), sbuf ); return 0;
    case MD_REAL:
      slen = float_str( get_float<double>( mref ), sbuf ); return 0;
    case MD_DECIMAL: {
      MDDecimal dec;
      dec.get_decimal( mref );
      slen = dec.get_string( sbuf, slen );
      return 0;
    }
    case MD_DATE: {
      MDDate date;
      date.get_date( mref );
      slen = date.get_string( sbuf, slen );
      return 0;
    }
    case MD_TIME: {
      MDTime time;
      time.get_time( mref );
      slen = time.get_string( sbuf, slen );
      return 0;
    }
    case MD_BOOLEAN: {
      if ( get_uint<uint8_t>( mref ) ) {
        slen = 4;
        ::strcpy( sbuf, "true" );
      }
      else {
        slen = 5;
        ::strcpy( sbuf, "false" );
      }
      return 0;
    }
    default:
      return Err::BAD_CVT_STRING;
  }
}

}
}

#endif
