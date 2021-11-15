#ifndef __rai_raimd__md_types_h__
#define __rai_raimd__md_types_h__
/* int64_t */
#include <stdint.h>
/* memcpy() */
#include <string.h>
/* malloc(), free() */
#include <stdlib.h>

namespace rai {
namespace md {

typedef int32_t MDFid; /* fid enumerates fields */

/* endian of the current arch, needs ifdef for big endian */
static const bool is_little_endian = true;

struct MDMsg;
struct MDReference;
struct MDDate;

enum MDOutputHint {
  MD_OUTPUT_OPAQUE_TO_HEX = 1,/* instead of excaping opaque, print hex */
  MD_OUTPUT_OPAQUE_TO_B64 = 2
};
struct MDOutput {
  int output_hints;
  MDOutput( int hints = 0 ) : output_hints( hints ) {}
  virtual int puts( const char *s ) noexcept; /* funcs send output to stdout */
  virtual int printf( const char *fmt, ... ) noexcept;
  int print_hex( const void *buf,  size_t buflen ) noexcept;
  int print_hex( const MDMsg *m ) noexcept; /* print hex buf */
  int indent( int i ) { /* message data indented */
    if ( i > 0 )
      return this->printf( "%*s", i, "" );
    return 0;
  }
};

enum MDType { /* field types */
  MD_NODATA      =  0, /* undefined field type */
  MD_MESSAGE     =  1,
  MD_STRING      =  2,
  MD_OPAQUE      =  3,
  MD_BOOLEAN     =  4,
  MD_INT         =  5,
  MD_UINT        =  6,
  MD_REAL        =  7,
  MD_ARRAY       =  8,
  MD_PARTIAL     =  9,
  MD_IPDATA      = 10, /* ^^ above enums are the same as tibmsg ( 0 -> 10 ) */
  MD_SUBJECT     = 11,
  MD_ENUM        = 12,
  MD_TIME        = 13,
  MD_DATE        = 14,
  MD_DATETIME    = 15,
  MD_STAMP       = 16,
  MD_DECIMAL     = 17,
  MD_LIST        = 18,
  MD_HASH        = 19,
  MD_SET         = 20,
  MD_ZSET        = 21,
  MD_GEO         = 22,
  MD_HYPERLOGLOG = 23,
  MD_STREAM      = 24
#define MD_TYPE_COUNT 25
};

enum MDEndian {
  MD_LITTLE = 0, /* x86, little first */
  MD_BIG    = 1  /* sparc, big first */
};

enum MDFlags {
  MD_PRIMITIVE = 1,
  MD_FIXED     = 2
};

/* what is endian of the cpu */
static const MDEndian md_endian = ( is_little_endian ? MD_LITTLE : MD_BIG );

/* is the type stored in endian format */
static inline bool is_integer( MDType t ) {
  if ( t >= MD_BOOLEAN && t <= MD_UINT )
    return true;
  if ( t == MD_IPDATA || t == MD_ENUM || t == MD_STAMP )
    return true;
  return false;
}

struct MDName {
  const char * fname;    /* name of a field, can be NULL */
  size_t       fnamelen; /* length of fname: 0 is null, 1 is empty string */
  MDFid        fid;      /* fid of the field, if 0 than not defined */

  MDName() {}
  MDName( const char *f,  size_t l = 0,  MDFid i = 0 ) {
    this->fname = f;
    if ( (this->fnamelen = l) == 0 )
      this->fnamelen = ::strlen( f ) + 1;
    this->fid = i;
  }
  void zero( void ) {
    this->fname    = NULL;
    this->fnamelen = 0;
    this->fid      = 0;
  }
  bool equals( const MDName &nm ) const {
    return nm.fnamelen == this->fnamelen &&
           ::memcmp( nm.fname, this->fname, this->fnamelen ) == 0;
  }
};
/* for msg_type = { MDLIT( "MSG_TYPE" ), 4005 }; */
#define MDLIT( STR ) STR, sizeof( STR )

enum MDTimeResolution {
  MD_RES_SECONDS   = 0, /* resolution of MDTime, MDStamp */
  MD_RES_MILLISECS = 1,
  MD_RES_MICROSECS = 2,
  MD_RES_NANOSECS  = 3,
  MD_RES_MINUTES   = 4,
  MD_RES_NULL      = 8
};

enum MDDecimalHint { /* decimal hint for exponent or fraction */
  MD_DEC_NNAN     = -4, /* -NaN */
  MD_DEC_NAN      = -3, /* NaN */
  MD_DEC_NINF     = -2, /* -Inf */
  MD_DEC_INF      = -1, /* Inf */
  MD_DEC_NULL     = 0,  /* empty */
  MD_DEC_INTEGER  = 1,  /* X^0 */
  MD_DEC_FRAC_2   = 2,  /* X/2 */
  MD_DEC_FRAC_4   = 3,  /* X/4 */
  MD_DEC_FRAC_8   = 4,  /* X/8 */
  MD_DEC_FRAC_16  = 5,  /* X/16 */
  MD_DEC_FRAC_32  = 6,  /* X/32 */
  MD_DEC_FRAC_64  = 7,  /* X/64 */
  MD_DEC_FRAC_128 = 8,  /* X/128 */
  MD_DEC_FRAC_256 = 9,  /* X/256 */
  MD_DEC_FRAC_512 = 10, /* X/512 */

  /* +10^N */
  MD_DEC_LOGp10_1 = 11, MD_DEC_LOGp10_2 = 12, MD_DEC_LOGp10_3 = 13,
  MD_DEC_LOGp10_4 = 14, MD_DEC_LOGp10_5 = 15, MD_DEC_LOGp10_6 = 16,
  MD_DEC_LOGp10_7 = 17, MD_DEC_LOGp10_8 = 18, MD_DEC_LOGp10_9 = 19,
  MD_DEC_LOGp10_10 = 20, /* these are legal: 21, 22, ... */

  /* -10^N */
  MD_DEC_LOGn10_1 = -11, MD_DEC_LOGn10_2 = -12, MD_DEC_LOGn10_3 = -13,
  MD_DEC_LOGn10_4 = -14, MD_DEC_LOGn10_5 = -15, MD_DEC_LOGn10_6 = -16,
  MD_DEC_LOGn10_7 = -17, MD_DEC_LOGn10_8 = -18, MD_DEC_LOGn10_9 = -19,
  MD_DEC_LOGn10_10 = -20 /* these are legal: -21, -22, ... */
};

/* move decimal right: N * index[ MD_DEC_LOGp10_2 - 10 ] = N00.0
                left:  N / index[ -(MD_DEC_LOGn10_2 + 10) ] = 0.0N */
#define MD_DECIMAL_POWERS \
 { 1, 10, 100, 1000, 10000, 100000, 1000 * 1000, 1000 * 10000, \
   1000 * 100000, 1000 * 1000 * 1000 /* up to 9 to keep in 32 bits */}

/* convert to real: N / index[ hint ] = N / ( 1 << (hint-1) ) */
#define MD_DECIMAL_FRACTIONS \
 { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 }

struct MDDecimal { /* base 10 decimal */
  int64_t ival;  /* 64 bits of significand */
  int8_t  hint;  /* the exponent or fraction */
  MDDecimal() {}
  MDDecimal( int64_t i,  int8_t h ) : ival( i ), hint( h ) {}
  MDDecimal( double fval ) { this->set_real( fval ); }
  MDDecimal( double fval,  int h ) noexcept;
  int parse( const char *s ) { return this->parse( s, ::strlen( s ) ); }
  int parse( const char *s,  const size_t fsize ) noexcept;
  void zero( void ) { ival = 0; hint = 0; }
  int get_real( double &val ) const noexcept;
  void degrade( int8_t new_hint ) noexcept;
  void set_real( double fval ) noexcept;
  int get_decimal( const MDReference &mref ) noexcept;
  size_t get_string( char *str,  size_t len,  bool expand_fractions = true
                  /* if expand is true, display .5 else 1/2 */ ) const noexcept;
};

struct MDTime {
  uint8_t  hour, /* hour: 0 -> 23 */
           min,  /* minute: 0 -> 59 */
           sec,  /* second: 0 -> 59 */
           resolution; /* 0 = sec, 1 = ms, 2 = us, 3 = ns, 4 = min */
  uint32_t fraction; /* fraction of a second depending on resolution */
  MDTime() {}
  MDTime( uint8_t h,  uint8_t m,  uint8_t s,  uint32_t f,  uint8_t r )
    : hour( h ), min( m ), sec( s ), resolution( r ), fraction( f ) {}
  int parse( const char *fptr, const size_t fsize ) noexcept;
  void zero( void ) { hour = 0; min = 0; sec = 0; resolution = 0; fraction = 0;}
  size_t get_string( char *str,  size_t len ) const noexcept;
  bool is_null( void ) const { return ( this->resolution & MD_RES_NULL ) != 0; }
  uint8_t res( void ) const { return this->resolution & ~MD_RES_NULL; }
  const char *res_string( void ) {
    static const char *res_str[] = { "s", "ms", "us", "ns", "m", 0, 0, 0, 0 };
    return res_str[ this->res() & 7 ];
  }
  int get_time( const MDReference &mref ) noexcept;
  uint64_t to_utc( MDDate *dt = NULL, bool is_gm_time = false ) noexcept;
};

enum MDDateFormat { /* formats for converting MDDate to string */
  MD_DATE_FMT_dd1  =     0x1,
  MD_DATE_FMT_dd2  =     0x2,
  MD_DATE_FMT_dd3  =     0x4,

  MD_DATE_FMT_mm1  =     0x8,
  MD_DATE_FMT_mm2  =    0x10,
  MD_DATE_FMT_mm3  =    0x20,

  MD_DATE_FMT_yy1  =    0x40,
  MD_DATE_FMT_yy2  =    0x80,
  MD_DATE_FMT_yy3  =   0x100,

  MD_DATE_FMT_SPACE =  0x200,
  MD_DATE_FMT_DASH  =  0x400,
  MD_DATE_FMT_SLASH =  0x800,
  MD_DATE_FMT_MMM   = 0x1000,
  MD_DATE_FMT_yyyy  = 0x2000,

  MD_DATE_FMT_dmy   = MD_DATE_FMT_dd1 | MD_DATE_FMT_mm2 | MD_DATE_FMT_yy3,
  MD_DATE_FMT_mdy   = MD_DATE_FMT_mm1 | MD_DATE_FMT_dd2 | MD_DATE_FMT_yy3,
  MD_DATE_FMT_ymd   = MD_DATE_FMT_yy1 | MD_DATE_FMT_mm2 | MD_DATE_FMT_dd3,

  MD_DATE_FMT_default      /* dd MMM yyyy */
   = MD_DATE_FMT_dmy | MD_DATE_FMT_SPACE | MD_DATE_FMT_MMM | MD_DATE_FMT_yyyy,
  MD_DATE_FMT_MMM_dd_yyyy  /* MMM dd yyyy */
   = MD_DATE_FMT_mdy | MD_DATE_FMT_SPACE | MD_DATE_FMT_MMM | MD_DATE_FMT_yyyy,
  MD_DATE_FMT_ddMMMyy      /* ddMMMyy */
   = MD_DATE_FMT_dmy | MD_DATE_FMT_MMM,
  MD_DATE_FMT_dd_MMM_yy    /* dd MMM yy */
   = MD_DATE_FMT_dmy | MD_DATE_FMT_MMM | MD_DATE_FMT_SPACE,
  MD_DATE_FMT_ddMMMyyyy    /* ddMMMyyyy */
   = MD_DATE_FMT_dmy | MD_DATE_FMT_MMM | MD_DATE_FMT_yyyy,
  MD_DATE_FMT_dd_mm_yyyy   /* dd mm yyyy */
   = MD_DATE_FMT_dmy | MD_DATE_FMT_yyyy | MD_DATE_FMT_SPACE,
  MD_DATE_FMT_mmSddSyy     /* mm/dd/yy */
   = MD_DATE_FMT_mdy | MD_DATE_FMT_SLASH,
  MD_DATE_FMT_mmSddSyyyy   /* mm/dd/yyyy */
   = MD_DATE_FMT_mdy | MD_DATE_FMT_yyyy | MD_DATE_FMT_SLASH,
  MD_DATE_FMT_yyyymmdd     /* yyyymmdd */
   = MD_DATE_FMT_ymd | MD_DATE_FMT_yyyy,
  MD_DATE_FMT_yyyyDmmDdd   /* yyyy-mm-dd */
   = MD_DATE_FMT_ymd | MD_DATE_FMT_yyyy | MD_DATE_FMT_DASH,
  MD_DATE_FMT_yyyySmmSdd   /* yyyy/mm/dd */
   = MD_DATE_FMT_ymd | MD_DATE_FMT_yyyy | MD_DATE_FMT_SLASH
};

struct MDDate {
  uint16_t year; /* 4 digit year */
  uint8_t  mon,  /* month 1 -> 12 */
           day;  /* day 1 -> 31 */
  MDDate() {}
  MDDate( uint16_t y,  uint8_t m,  uint8_t d ) : year( y ), mon( m ), day( d ) {}
  void zero( void ) { year = 0; mon = 0; day = 0; }
  size_t get_string( char *str,  size_t len,
                     MDDateFormat fmt = MD_DATE_FMT_default ) const noexcept;
  bool is_null( void ) const { return this->year == 0 && this->mon == 0 &&
                                      this->day == 0; }
  static int parse_format( const char *s,  MDDateFormat &fmt ) noexcept;
  int parse( const char *fptr,  const size_t flen ) noexcept;
  int get_date( const MDReference &mref ) noexcept;
  uint64_t to_utc( bool is_gm_time = false ) noexcept;
};

struct MDStamp {
  uint64_t stamp;      /* UTC stamp */
  uint8_t  resolution; /* resolution of stamp (nsecs, usecs, msecs, secs) */
  MDStamp() {}
  MDStamp( uint64_t s,  uint8_t r ) : stamp( s ), resolution( r ) {}
  void zero( void ) { this->stamp = 0; this->resolution = 0; }
  size_t get_string( char *str,  size_t len ) const noexcept;
  int parse( const char *fptr,  size_t flen, bool is_gm_time = false ) noexcept;
  int get_stamp( const MDReference &mref ) noexcept;
  bool is_null( void ) const { return ( this->resolution & MD_RES_NULL ) != 0; }
  uint8_t res( void ) const { return this->resolution & ~MD_RES_NULL; }
  uint64_t seconds( void ) const noexcept;
  uint64_t millis( void ) const noexcept;
  uint64_t micros( void ) const noexcept;
  uint64_t nanos( void ) const noexcept;
};

struct MDEnum {
  uint16_t     val;      /* the enum value 0 -> N */
  const char * disp;     /* string associated with val */
  size_t       disp_len; /* length of disp[] */
  MDEnum() {}
  MDEnum( uint16_t v,  const char *d,  size_t l )
    : val( v ), disp( d ), disp_len( l ) {}
  void zero( void ) { this->val = 0; this->disp = 0; this->disp_len = 0; }
};

/* reference to a field data element or array data elem */
struct MDReference {
  uint8_t * fptr;     /* field data memory */
  size_t    fsize;    /* size of field */
  MDType    ftype;    /* type, MD_STRING, MD_INT, etc */
  MDEndian  fendian;  /* if machine type, the endian of the data */
  MDType    fentrytp; /* if array, the element type */
  uint8_t   fentrysz; /* the size of each element, fsize is the entire array */

  void zero() {
    this->set();
  }

  void set( void *fp = NULL,  size_t sz = 0,  MDType ft = MD_NODATA,
            MDEndian end = MD_LITTLE ) {
    this->fptr     = (uint8_t *) fp;
    this->fsize    = sz;
    this->ftype    = ft;
    this->fendian  = end;
    this->fentrytp = MD_NODATA;
    this->fentrysz = 0;
  }
};

union MDValue { /* used for setting fptr temporary above w/string msg formats */
  char    * str;
  double    f64;
  float     f32;
  int64_t   i64;
  uint64_t  u64;
  int32_t   i32;
  uint32_t  u32;
  int16_t   i16;
  uint16_t  u16;
  int8_t    i8;
  uint8_t   u8;
  bool      b;
};

bool string_is_true( const char *s ) noexcept; /* string to boolean */

const char *md_type_str( MDType type,  size_t size = 0 ) noexcept;

MDType md_str_type( const char *str,  size_t &size ) noexcept; /* str to type */

struct MDErrorRec {
  int          status; /* enum error below */
  const char * descr, * mod; /* strings associated with error */
};
typedef const struct MDErrorRec * MDError;

namespace Err {
  enum {
    BAD_MAGIC_NUMBER      = 1, /* Can't determine message type */
    BAD_BOUNDS            = 2, /* Message boundary overflows */
    BAD_FIELD_SIZE        = 3, /* Unable to determine field size */
    BAD_FIELD_TYPE        = 4, /* Unable to determine field type */
    BAD_FIELD_BOUNDS      = 5, /* Field boundary overflows message boundary */
    BAD_CVT_BOOL          = 6, /* Unable to convert field to boolean */
    BAD_CVT_STRING        = 7, /* Unable to convert field to string */
    BAD_CVT_NUMBER        = 8, /* Unable to convert field to number */
    NOT_FOUND             = 9, /* Not found */
    NO_DICTIONARY         = 10, /* No dictionary loaded */
    UNKNOWN_FID           = 11, /* Unable to find FID definition */
    NULL_FID              = 12, /* FID is null value */
    BAD_HEADER            = 13, /* Bad header */
    BAD_FIELD             = 14, /* Bad field */
    BAD_DECIMAL           = 15, /* Bad decimal hint */
    BAD_NAME              = 16, /* Name length overflow */
    ARRAY_BOUNDS          = 17, /* Array bounds index overflow */
    BAD_STAMP             = 18, /* Bad stamp */
    BAD_DATE              = 19, /* Bad date */
    BAD_TIME              = 20, /* Bad time */
    BAD_SUB_MSG           = 21, /* Bad submsg */
    INVALID_MSG           = 22, /* Unable to use message */
    NO_MSG_IMPL           = 23, /* No implementation for function */
    EXPECTING_TRUE        = 24, /* Expected 'true' token */
    EXPECTING_FALSE       = 25, /* Expected 'false' token */
    EXPECTING_AR_VALUE    = 26, /* Expecting array value */
    UNEXPECTED_ARRAY_END  = 27, /* Unterminated array, missing ']' */
    EXPECTING_COLON       = 28, /* Expecting ':' in object */
    EXPECTING_OBJ_VALUE   = 29, /* Expecting value in object */
    UNEXPECTED_OBJECT_END = 30, /* Unterminated object, missing '}' */
    UNEXPECTED_STRING_END = 31, /* Unterminated string, missing '\"' */
    INVALID_CHAR_HEX      = 32, /* Can't string hex escape sequence */
    EXPECTING_NULL        = 33, /* Expecting 'null' token */
    NO_SPACE              = 34, /* Size check failed, no space */
    BAD_CAST              = 35, /* Can't cast to number */
    TOO_BIG               = 36, /* Json too large for parser */
    INVALID_TOKEN         = 37, /* Invalid token */
    NO_ENUM               = 38, /* No enum */
    NO_PARTIAL            = 39, /* No partial */
    FILE_NOT_FOUND        = 40, /* File not found */
    DICT_PARSE_ERROR      = 41, /* Dictionary parse error */
    ALLOC_FAIL            = 42, /* Allocation failed */
    BAD_SUBJECT           = 43, /* Bad subject */
    BAD_FORMAT            = 44  /* Bad msg structure */
  };
  MDError err( int status ) noexcept;
}

}
}

#include <raimd/md_int.h>

#endif
