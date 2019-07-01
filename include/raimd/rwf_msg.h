#ifndef __rai_raimd__rwf_msg_h__
#define __rai_raimd__rwf_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

enum RwfFieldListFlags {
  HAS_FIELD_LIST_INFO = 1,
  HAS_SET_DATA        = 2,
  HAS_SET_ID          = 4,
  HAS_STANDARD_DATA   = 8
};

struct RwfFieldListHdr {
  uint8_t  flags, /* flags above */
           dict_id;
  int16_t  flist;
  uint16_t field_cnt,
           data_type; /* 132 = DT_FIELD_LIST */
  size_t   data_start;
};

static const uint32_t RWF_TYPE_ID = 0x25cdabca;

struct RwfMsg : public MDMsg {
  RwfFieldListHdr hdr;

  static int parse_header( const uint8_t *buf,  size_t buflen,
                           RwfFieldListHdr &hdr );
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RwfMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) final;
  virtual uint32_t get_type_id( void ) final;
  virtual int get_field_iter( MDFieldIter *&iter ) final;

  /* may return tibmsg, sass qform or rv */
  static bool is_rwf( void *bb,  size_t off,  size_t end,  uint32_t h );
  static RwfMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                         MDDict *d,  MDMsgMem *m );
  static void init_auto_unpack( void );
};

struct RwfFieldIter : public MDFieldIter {
  MDType       ftype;     /* field type from dictionary */
  uint32_t     fsize;     /* length of string, size of int */
  const char * fname;     /* name associated with fid */
  uint8_t      fnamelen;  /* len of fname */
  MDValue      val;       /* union of temp values */
  MDFid        fid;       /* field fid, used for dictionary lookup */
  MDDecimal    dec;       /* if price */
  MDTime       time;      /* if time field */
  MDDate       date;      /* if data field */
  size_t       data_off,  /* position after header where data starts */
               field_idx; /* current field index */
  uint16_t     position;  /* partial offset / escape position */

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  RwfFieldIter( MDMsg &m ) : MDFieldIter( m ), ftype( MD_NODATA ), fsize( 0 ),
      fname( 0 ), fnamelen( 0 ), fid( 0 ), data_off( 0 ), field_idx( 0 ),
      position( 0 ) {}

  void lookup_fid( void );
  virtual int get_name( MDName &name ) final;
  virtual int get_enum( MDReference &mref,  MDEnum &enu ) final;
  virtual int get_reference( MDReference &mref ) final;
  virtual int get_hint_reference( MDReference &mref ) final;
  virtual int find( const char *name, size_t name_len, MDReference &mref )final;
  virtual int first( void ) final;
  virtual int next( void ) final;
  int unpack( void );
};

/* 0     1     2     3     4     5     6      7      <- offset
 * [         magic        ][  0     0     0     132 ]
 * [flag][ 3  ][dict][ flist    ][ nflds     ]
 */
struct RwfMsgWriter {
  MDDict  * dict;
  uint8_t * buf;
  size_t    off,
            buflen;
  uint16_t  nflds,
            flist;

  RwfMsgWriter( MDDict *d,  void *bb,  size_t len );

  int append_ref( MDFid fid,  MDReference &mref );
  int append_ref( const char *fname,  size_t fname_len,  MDReference &mref );
  int append_ref( MDFid fid, MDType ftype, uint32_t fsize, MDReference &mref );

  bool has_space( size_t len ) const {
    return this->off + len <= this->buflen;
  }

  size_t update_hdr( void ) {
    this->buf[ 0 ]  = 0x25;
    this->buf[ 1 ]  = 0xcd;
    this->buf[ 2 ]  = 0xab;
    this->buf[ 3 ]  = 0xca;
    this->buf[ 4 ]  = 0;
    this->buf[ 5 ]  = 0;
    this->buf[ 6 ]  = 0;
    this->buf[ 7 ]  = 132;
    this->buf[ 8 ]  = HAS_FIELD_LIST_INFO | HAS_STANDARD_DATA;
    this->buf[ 9 ]  = 3;
    this->buf[ 10 ] = 1;
    this->buf[ 11 ] = ( this->flist >> 8 ) & 0xffU;
    this->buf[ 12 ] = this->flist & 0xffU;
    this->buf[ 13 ] = ( this->nflds >> 8 ) & 0xffU;
    this->buf[ 14 ] = this->nflds & 0xffU;
    return this->off;
  }

  template< class T >
  int append_type( MDFid fid,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
  }

  template< class T >
  int append_type( const char *fname,  size_t fname_len,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  int append_ival( MDFid fid,  const void *ival,  size_t ilen,  MDType t );

  int append_ival( const char *fname,  size_t fname_len,  const void *ival,
                   size_t ilen,  MDType t );
  int pack_uval( MDFid fid,  const uint8_t *ival,  size_t ilen );

  int pack_ival( MDFid fid,  const uint8_t *ival,  size_t ilen );

  int pack_partial( MDFid fid, const uint8_t *fptr,  size_t fsize,
                    size_t foffset );
  template< class T >
  int append_int( MDFid fid,  T ival ) {
    return this->append_ival( fid, &ival, sizeof( ival ), MD_INT );
  }
  template< class T >
  int append_uint( MDFid fid,  T uval ) {
    return this->append_ival( fid, &uval, sizeof( uval ), MD_UINT );
  }
  template< class T >
  int append_real( MDFid fid,  T rval ) {
    return this->append_type( fid, rval, MD_REAL );
  }

  template< class T >
  int append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_ival( fname, fname_len, &ival, sizeof( ival ), MD_INT );
  }
  template< class T >
  int append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_ival( fname, fname_len, &uval, sizeof( uval ), MD_UINT);
  }
  template< class T >
  int append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }

  int append_string( MDFid fid,  const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
  }

  int append_string( const char *fname,  size_t fname_len,
                     const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  int append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                      MDDecimal &dec );
  int append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                   MDTime &time );
  int append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                   MDDate &date );

  int append_decimal( MDFid fid,  MDDecimal &dec );
  int append_time( MDFid,  MDTime &time );
  int append_date( MDFid,  MDDate &date );

  int append_decimal( const char *fname,  size_t fname_len,  MDDecimal &dec );
  int append_time( const char *fname,  size_t fname_len,  MDTime &time );
  int append_date( const char *fname,  size_t fname_len,  MDDate &date );
};

}
} // namespace rai

#endif
