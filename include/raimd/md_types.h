#ifndef __rai_raimd__md_types_h__
#define __rai_raimd__md_types_h__
/* int64_t */
#include <stdint.h>
/* memcpy() */
#include <string.h>
/* malloc(), free() */
#include <stdlib.h>
/* __BYTE_ORDER */
#include <endian.h>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MD_LITTLE = 0, /* x86, little first */
  MD_BIG    = 1  /* sparc, big first */
} MDEndian;

/* what is endian of the cpu */
static const MDEndian md_endian =
#if __BYTE_ORDER == __LITTLE_ENDIAN
  MD_LITTLE;
#else
  MD_BIG;
#endif

typedef enum { /* field types */
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
  MD_STREAM      = 24,
  MD_XML         = 25
#define MD_TYPE_COUNT 26
} MDType;

typedef int32_t MDFid; /* fid enumerates fields */

typedef struct MDName_s {
  const char * fname;    /* name of a field, can be NULL */
  size_t       fnamelen; /* length of fname: 0 is null, 1 is empty string */
  MDFid        fid;      /* fid of the field, if 0 than not defined */
} MDName_t;

void md_name_zero( MDName_t *name );
bool md_name_equals_name( const MDName_t *name, const MDName_t *nm );
bool md_name_equals( const MDName_t *name, const char *fname, size_t len2 );

typedef struct MDOutput_s MDOutput_t;
void md_output_init( MDOutput_t **mout );
void md_output_release( MDOutput_t *mout );
int md_output_open( MDOutput_t *mout,  const char *fn,  const char *mode );
int md_output_close( MDOutput_t *mout );
int md_output_flush( MDOutput_t *mout );
size_t md_output_write( MDOutput_t *mout,  const void *buf,  size_t buflen );
int md_output_puts( MDOutput_t *mout,  const char *s );
int md_output_printf( MDOutput_t *mout,  const char *fmt, ... ) __attribute__((format(printf,2,3)));;
int md_output_printe( MDOutput_t *mout,  const char *fmt, ... ) __attribute__((format(printf,2,3)));;
int md_output_print_hex( MDOutput_t *mout,  const void *buf,  size_t buflen );
struct MDMsg_s;
int md_output_print_msg_hex( MDOutput_t *mout,  struct MDMsg_s *msg );

/* reference to a field data element or array data elem */
typedef struct MDReference_s {
  uint8_t * fptr;     /* field data memory */
  size_t    fsize;    /* size of field */
  MDType    ftype;    /* type, MD_STRING, MD_INT, etc */
  MDEndian  fendian;  /* if machine type, the endian of the data */
  MDType    fentrytp; /* if array, the element type */
  uint32_t  fentrysz; /* the size of each element, fsize is the entire array */
} MDReference_t;

// MDReference functions
void md_reference_zero( MDReference_t *ref );
void md_reference_set( MDReference_t *ref, void *fp, size_t sz, MDType ft, MDEndian end );
void md_reference_set_string( MDReference_t *ref, const char *p, size_t len );
void md_reference_set_string_z( MDReference_t *ref, const char *p );
bool md_reference_equals( MDReference_t *ref, MDReference_t *mref );

/* for msg_type = { MDLIT( "MSG_TYPE" ), 4005 }; */
#define MDLIT( STR ) STR, sizeof( STR )

typedef enum {
  MD_RES_SECONDS   = 0, /* resolution of MDTime, MDStamp */
  MD_RES_MILLISECS = 1,
  MD_RES_MICROSECS = 2,
  MD_RES_NANOSECS  = 3,
  MD_RES_MINUTES   = 4,
  MD_RES_NULL      = 8
} MDTimeResolution;

typedef enum { /* decimal hint for exponent or fraction */
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
} MDDecimalHint;

/* move decimal right: N * index[ MD_DEC_LOGp10_2 - 10 ] = N00.0
                left:  N / index[ -(MD_DEC_LOGn10_2 + 10) ] = 0.0N */
#define MD_DECIMAL_POWERS \
 { 1, 10, 100, 1000, 10000, 100000, 1000 * 1000, 1000 * 10000, \
   1000 * 100000, 1000 * 1000 * 1000 /* up to 9 to keep in 32 bits */}

/* convert to real: N / index[ hint ] = N / ( 1 << (hint-1) ) */
#define MD_DECIMAL_FRACTIONS \
 { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 }

typedef struct MDDecimal_s { /* base 10 decimal */
  int64_t ival;  /* 64 bits of significand */
  int8_t  hint;  /* the exponent or fraction */
} MDDecimal_t;

// MDDecimal functions
void md_decimal_set(MDDecimal_t *dec, int64_t i, int8_t h);
int md_decimal_parse_len(MDDecimal_t *dec, const char *s, const size_t fsize);
int md_decimal_parse(MDDecimal_t *dec, const char *s);
void md_decimal_zero(MDDecimal_t *dec);
int md_decimal_get_real(MDDecimal_t *dec, double *val);
int md_decimal_get_integer(MDDecimal_t *dec, int64_t *val);
int md_decimal_degrade(MDDecimal_t *dec);
void md_decimal_set_real(MDDecimal_t *dec, double fval);
int md_decimal_get_decimal(MDDecimal_t *dec, MDReference_t *mref);
size_t md_decimal_get_string(MDDecimal_t *dec, char *str, size_t len, bool expand_fractions);

typedef enum { /* formats for converting MDDate to string */
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
} MDDateFormat;

typedef struct MDDate_s {
  uint16_t year; /* 4 digit year */
  uint8_t  mon,  /* month 1 -> 12 */
           day;  /* day 1 -> 31 */
} MDDate_t;

// MDDate functions
void md_date_set(MDDate_t *date, uint16_t y, uint8_t m, uint8_t d);
void md_date_zero(MDDate_t *date);
size_t md_date_get_string(MDDate_t *date, char *str, size_t len, MDDateFormat fmt);
bool md_date_is_null(MDDate_t *date);
/*int md_date_parse_format(const char *s, MDDateFormat *fmt);*/
int md_date_parse(MDDate_t *date, const char *fptr, const size_t flen);
int md_date_get_date(MDDate_t *date, MDReference_t *mref);
uint64_t md_date_to_utc(MDDate_t *date, bool is_gm_time);

typedef struct MDTime_s {
  uint8_t  hour,   /* hour: 0 -> 23 */
           minute, /* minute: 0 -> 59 */
           sec,    /* second: 0 -> 59 */
           resolution; /* 0 = sec, 1 = ms, 2 = us, 3 = ns, 4 = min */
  uint32_t fraction; /* fraction of a second depending on resolution */
} MDTime_t;

// MDTime functions
void md_time_set(MDTime_t *time, uint8_t h, uint8_t m, uint8_t s, uint32_t f, uint8_t r);
int md_time_parse(MDTime_t *time, const char *fptr, const size_t fsize);
void md_time_zero(MDTime_t *time);
size_t md_time_get_string(MDTime_t *time, char *str, size_t len);
bool md_time_is_null(MDTime_t *time);
uint8_t md_time_res(MDTime_t *time);
const char *md_time_res_string(MDTime_t *time);
int md_time_get_time(MDTime_t *time, MDReference_t *mref);
uint64_t md_time_to_utc(MDTime_t *time, MDDate_t *dt, bool is_gm_time);

typedef struct MDStamp_s {
  uint64_t stamp;      /* UTC stamp */
  uint8_t  resolution; /* resolution of stamp (nsecs, usecs, msecs, secs) */
} MDStamp_t;

typedef struct MDEnum_s {
  uint16_t     val;      /* the enum value 0 -> N */
  const char * disp;     /* string associated with val */
  size_t       disp_len; /* length of disp[] */
} MDEnum_t;

#ifdef __cplusplus
}

namespace rai {
namespace md {

struct MDMsg;
struct MDReference;
struct MDDate;

enum MDOutputHint {
  MD_OUTPUT_OPAQUE_TO_HEX = 1,/* instead of excaping opaque, print hex */
  MD_OUTPUT_OPAQUE_TO_B64 = 2
};
struct MDOutput {
  void * filep;
  int output_hints;
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  MDOutput( int hints = 0 ) : filep( 0 ), output_hints( hints ) {}
  virtual ~MDOutput() { if ( this->filep != NULL ) this->close(); }
  virtual size_t write( const void *buf,  size_t buflen ) noexcept;
  virtual int puts( const char *s ) noexcept; /* funcs send output to stdout */
  virtual int printf( const char *fmt, ... ) noexcept
    __attribute__((format(printf,2,3)));
  virtual int printe( const char *fmt, ... ) noexcept
    __attribute__((format(printf,2,3)));
  int print_hex( const void *buf,  size_t buflen ) noexcept;
  int print_hex( const MDMsg *m ) noexcept; /* print hex buf */
  int indent( int i ) { /* message data indented */
    if ( i > 0 )
      return this->printf( "%*s", i, "" );
    return 0;
  }
  virtual int open( const char *path,  const char *mode ) noexcept;
  virtual int close( void ) noexcept;
  virtual int flush( void ) noexcept;
};

/* is the type stored in endian format */
static inline bool is_integer( MDType t ) {
  if ( t >= MD_BOOLEAN && t <= MD_UINT )
    return true;
  if ( t == MD_IPDATA || t == MD_ENUM || t == MD_STAMP )
    return true;
  return false;
}
static inline bool is_endian_type( MDType t ) {
  if ( t >= MD_BOOLEAN && t <= MD_REAL )
    return true;
  if ( t == MD_ENUM || t == MD_STAMP )
    return true;
  return false; /* not including complex types like decimal */
}

struct MDName : public MDName_s {
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
    return this->equals( nm.fname, nm.fnamelen );
  }
  bool equals( const char *fname,  size_t len2 ) const {
    size_t len  = this->fnamelen;
    if ( len > 0 && this->fname[ len - 1 ] == '\0' )
      len--;
    if ( len2 > 0 && fname[ len2 - 1 ] == '\0' )
      len2--;
    return len == len2 && ::memcmp( this->fname, fname, len ) == 0;
  }
};
#if 0
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
#endif
struct MDDecimal : public MDDecimal_s { /* base 10 decimal */
  MDDecimal() {}
  MDDecimal( int64_t i,  int8_t h ) { this->set( i, h ); }
  MDDecimal( double fval ) { this->set_real( fval ); }
  MDDecimal( const MDDecimal &d ) { this->set( d.ival, d.hint ); }
  MDDecimal &operator=( const MDDecimal &d ) {
    this->ival = d.ival; this->hint = d.hint;
    return *this;
  }
  void set( int64_t i,  int8_t h ) { this->ival = i; this->hint = h; }
  int parse( const char *s ) { return this->parse( s, ::strlen( s ) ); }
  int parse( const char *s,  const size_t fsize ) noexcept;
  void zero( void ) { ival = 0; hint = 0; }
  int get_real( double &val ) const noexcept;
  int get_integer( int64_t &val ) const noexcept;
  /*void degrade( int8_t new_hint ) noexcept;*/
  int degrade( void ) noexcept;
  void set_real( double fval ) noexcept;
  int get_decimal( const MDReference &mref ) noexcept;
  size_t get_string( char *str,  size_t len,  bool expand_fractions = true
                  /* if expand is true, display .5 else 1/2 */ ) const noexcept;
};

struct MDTime : public MDTime_s {
  MDTime() {}
  MDTime( uint8_t h,  uint8_t m,  uint8_t s,  uint32_t f,  uint8_t r ) {
    this->set( h, m, s, f, r );
  }
  MDTime( const MDTime &t ) { *this = t; }
  MDTime &operator=( const MDTime &t ) {
    this->set( t.hour, t.minute, t.sec, t.fraction, t.resolution );
    return *this;
  }
  void set( uint8_t h,  uint8_t m,  uint8_t s,  uint32_t f,  uint8_t r ) {
    this->hour = h; this->minute = m; this->sec = s; this->resolution = r;
    this->fraction = f;
  }
  int parse( const char *fptr, const size_t fsize ) noexcept;
  void zero( void ) { this->set( 0, 0, 0, 0, 0 ); }
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
#if 0
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
#endif
struct MDDate : public MDDate_s {
  MDDate() {}
  MDDate( uint16_t y,  uint8_t m,  uint8_t d ) { this->set( y, m, d ); }
  MDDate( const MDDate &d ) { *this = d; }
  MDDate &operator=( const MDDate &d ) {
    this->set( d.year, d.mon, d.day );
    return *this;
  }
  void set( uint16_t y,  uint8_t m,  uint8_t d ) {
    this->year = y; this->mon = m; this->day = d;
  }
  void zero( void ) { this->set( 0, 0, 0 ); }
  size_t get_string( char *str,  size_t len,
                     MDDateFormat fmt = MD_DATE_FMT_default ) const noexcept;
  bool is_null( void ) const { return this->year == 0 && this->mon == 0 &&
                                      this->day == 0; }
  /*static int parse_format( const char *s,  MDDateFormat &fmt ) noexcept;*/
  int parse( const char *fptr,  const size_t flen ) noexcept;
  int get_date( const MDReference &mref ) noexcept;
  uint64_t to_utc( bool is_gm_time = false ) noexcept;
};

struct MDStamp : public MDStamp_s {
  MDStamp() {}
  MDStamp( uint64_t s,  uint8_t r ) { this->set( s, r ); }
  MDStamp( const MDStamp &s ) { *this = s; }
  MDStamp &operator=( const MDStamp &s ) {
    this->set( s.stamp, s.resolution );
    return *this;
  }
  void set( uint64_t s,  uint8_t r ) { this->stamp = s; this->resolution = r; }
  void zero( void ) { this->set( 0, 0 ); }
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

struct MDEnum : public MDEnum_s {
  MDEnum() {}
  MDEnum( uint16_t v,  const char *d,  size_t l ) { this->set( v, d, l ); }
  void set( uint16_t v,  const char *d,  size_t l ) {
    this->val = v; this->disp = d; this->disp_len = l;
  }
  void zero( void ) { this->set( 0, 0, 0 ); }
};

/* reference to a field data element or array data elem */
struct MDReference : public MDReference_s {
  MDReference() {}
  MDReference( void *fp,  size_t sz,  MDType ft, MDEndian end = md_endian ) {
    this->set( fp, sz, ft, end );
  }
  MDReference( void *fp,  size_t sz,  MDType ft, MDType fetp,  uint32_t fesz,
               MDEndian end = md_endian ) {
    this->fptr     = (uint8_t *) fp;
    this->fsize    = sz;
    this->ftype    = ft;
    this->fendian  = end;
    this->fentrytp = fetp;
    this->fentrysz = fesz;
  }
  void zero() {
    this->set();
  }
  void set( void *fp = NULL,  size_t sz = 0,  MDType ft = MD_NODATA,
            MDEndian end = md_endian ) {
    this->fptr     = (uint8_t *) fp;
    this->fsize    = sz;
    this->ftype    = ft;
    this->fendian  = end;
    this->fentrytp = MD_NODATA;
    this->fentrysz = 0;
  }
  template<class T>
  void set_uint( T &uval ) {
    this->set( &uval, sizeof( T ), MD_UINT );
  }
  template<class T>
  void set_int( T &ival ) {
    this->set( &ival, sizeof( T ), MD_INT );
  }
  void set_string( const char *p,  size_t len ) {
    this->set( (void *) p, len, MD_STRING );
  }
  void set_string( const char *p ) { 
    this->set_string( p, ::strlen( p ) );
  }
  bool equals( const MDReference &mref ) const {
    return this->ftype == mref.ftype &&
           this->fsize == mref.fsize &&
           this->fendian == mref.fendian &&
           this->fentrytp == mref.fentrytp &&
           this->fentrysz == mref.fentrysz &&
           ( this->fsize == 0 ||
             ::memcmp( this->fptr, mref.fptr, this->fsize ) == 0 );
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
const char *md_sass_msg_type_str( uint16_t msg_type,  char *buf ) noexcept;
const char *md_sass_rec_status_str( uint16_t msg_type,  char *buf ) noexcept;

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
    NO_FORM               = 39, /* No enum */
    NO_PARTIAL            = 40, /* No partial */
    FILE_NOT_FOUND        = 41, /* File not found */
    DICT_PARSE_ERROR      = 42, /* Dictionary parse error */
    ALLOC_FAIL            = 43, /* Allocation failed */
    BAD_SUBJECT           = 44, /* Bad subject */
    BAD_FORMAT            = 45  /* Bad msg structure */
  };
  MDError err( int status ) noexcept;
}

}
}

#endif

#include <raimd/md_int.h>

#endif
