#ifndef __rai_raimd__tib_sass_msg_h__
#define __rai_raimd__tib_sass_msg_h__

#include <raimd/md_msg.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t TIB_SASS_TYPE_ID      = 0x179ca0f5,
                      TIB_SASS_FORM_TYPE_ID = 0xa08b0040;
#ifndef __cplusplus
#define TIB_SASS_TYPE_ID      0x179ca0f5U
#define TIB_SASS_FORM_TYPE_ID 0xa08b0040U
#endif
MDMsg_t *tib_sass_msg_unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                              MDDict_t *d,  MDMsgMem_t *m );
MDMsgWriter_t *tib_sass_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,
                                           void *buf_ptr, size_t buf_sz );
MDMsgWriter_t *tib_sass_msg_writer_create_with_form( MDMsgMem_t *mem,  MDFormClass_t *form,
                                           void *buf_ptr, size_t buf_sz );
#ifdef __cplusplus
}
namespace rai {
namespace md {

enum MDSassDictFlags {
  MD_PRIMITIVE = 1,
  MD_FIXED     = 2
};

struct TibSassMsg : public MDMsg {
  /* used by UnPack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibSassMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, end, d, m ) {}

  virtual const char *get_proto_string( void ) noexcept;
  virtual uint32_t get_type_id( void ) noexcept;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept;

  /* may return tibmsg, sass qform or rv */
  static bool is_tibsassmsg( void *bb,  size_t off,  size_t end,
                             uint32_t h ) noexcept;
  static TibSassMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                             MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

static inline size_t tib_sass_pack_size( size_t fsize ) {
  return ( 1U + ( ( fsize + 1U ) >> 1 ) ) << 1; /* fid + fsize  2b aligned */
}
static inline size_t tib_sass_partial_pack_size( size_t fsize ) {
  return tib_sass_pack_size( fsize + 4 ); /* fid + off + len, 2b aligned */
}
static inline size_t tib_sass_variable_pack_size( size_t asize, size_t fsize ) {
  if ( fsize <= 0xffffU )
    return tib_sass_pack_size( asize + 2 ); /* fid + len, 2b aligned */
  return tib_sass_pack_size( asize + 4 ); /* fid + len, 2b aligned */
}

struct TibSassFieldIter : public MDFieldIter {
  const char * fname;
  uint32_t     fsize;
  MDFid        fid;
  MDType       ftype;
  uint8_t      flags,
               fnamelen;
  MDDecimal    dec;
  MDTime       time;
  MDDate       date;

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  TibSassFieldIter( MDMsg &m ) : MDFieldIter( m ), fname( 0 ), fsize( 0 ),
    fid( 0 ), ftype( MD_NODATA ), flags( MD_FIXED ), fnamelen( 0 ) {}

  void dup_sass( TibSassFieldIter &i ) {
    i.fname = this->fname;
    i.fsize = this->fsize;
    i.fid = this->fid;
    i.ftype = this->ftype;
    i.flags = this->flags,
    i.fnamelen = this->fnamelen;
    i.dec = this->dec;
    i.time = this->time;
    i.date = this->date;
    this->MDFieldIter::dup_iter( i );
  }
  virtual MDFieldIter *copy( void ) noexcept final;
  virtual int get_name( MDName &name ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_hint_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack( void ) noexcept;

  size_t pack_size( void ) const {
    return tib_sass_pack_size( this->fsize );
  }
  static size_t partial_pack_size( uint16_t len ) {
    return tib_sass_partial_pack_size( len );
  }
  static size_t variable_pack_size_2( size_t asize ) {
    return tib_sass_pack_size( asize + 2 ); /* fid + len, 2b aligned */
  }
  static size_t variable_pack_size_4( size_t asize ) {
    return tib_sass_pack_size( asize + 4 ); /* fid + len, 2b aligned */
  }
};

enum {
  TSS_HINT_NONE            = 0,   /* no hint */
  TSS_HINT_DENOM_2         = 1,   /* 1/2 */
  TSS_HINT_DENOM_4         = 2,
  TSS_HINT_DENOM_8         = 3,
  TSS_HINT_DENOM_16        = 4,
  TSS_HINT_DENOM_32        = 5,
  TSS_HINT_DENOM_64        = 6,
  TSS_HINT_DENOM_128       = 7,
  TSS_HINT_DENOM_256       = 8,   /* 1/256 */
  TSS_HINT_PRECISION_1     = 17,  /* 10^-1 */
  TSS_HINT_PRECISION_2     = 18,
  TSS_HINT_PRECISION_3     = 19,
  TSS_HINT_PRECISION_4     = 20,
  TSS_HINT_PRECISION_5     = 21,
  TSS_HINT_PRECISION_6     = 22,
  TSS_HINT_PRECISION_7     = 23,
  TSS_HINT_PRECISION_8     = 24,
  TSS_HINT_PRECISION_9     = 25,   /* 10^-9 */
  TSS_HINT_BLANK_VALUE     = 127,  /* no value */
  TSS_HINT_DATE_TYPE       = 256,  /* SASS TSS_STIME */
  TSS_HINT_TIME_TYPE       = 257,  /* SASS TSS_SDATE */
  TSS_HINT_MF_DATE_TYPE    = 258,  /* marketfeed date */
  TSS_HINT_MF_TIME_TYPE    = 259,  /* marketfeed time */
  TSS_HINT_MF_TIME_SECONDS = 260,  /* marketfeed time_seconds */
  TSS_HINT_MF_ENUM         = 261   /* marketfeed enum */
};

struct MDFormClass;
struct MDFormEntry;
struct MDLookup;

struct TibSassMsgWriter : public MDMsgWriterBase {
  MDDict      * dict;
  MDFormClass * form;
  int           unk_fid;
  bool          use_form;

  void * operator new( size_t, void *ptr ) { return ptr; }
  TibSassMsgWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len ) noexcept;
  TibSassMsgWriter( MDMsgMem &m,  MDFormClass &f,  void *bb,
                    size_t len ) noexcept;
  TibSassMsgWriter & error( int status ) {
    if ( this->err == 0 )
      this->err = status;
    return *this;
  }
  TibSassMsgWriter & unknown_fid( void ) {
    this->unk_fid++;
    return *this;
  }
  void reset( void ) {
    this->off      = 0;
    this->err      = 0;
    this->unk_fid  = 0;
    this->use_form = false;
  }
  TibSassMsgWriter & append_form_record( void ) noexcept;

  bool lookup( MDLookup &by,  const MDFormEntry *&entry ) noexcept;
  bool get( MDLookup &by,  const MDFormEntry *&entry ) noexcept;
  TibSassMsgWriter & append_ref( MDFid fid,  MDReference &mref ) noexcept;
/*  TibSassMsgWriter & append_ref( const char *fname,  size_t fname_len,
                                 MDReference &mref,  MDReference & ) {
    return this->append_ref( fname, fname_len, mref );
  }*/
  TibSassMsgWriter & append_ref( const char *fname,  size_t fname_len,
                                 MDReference &mref ) noexcept;
  TibSassMsgWriter & append_ref( MDFid fid,  MDType ftype,  uint32_t fsize,
                                 uint8_t flags,  MDReference &mref,
                                 const MDFormEntry *entry ) noexcept;

  bool has_space( size_t len ) {
    bool b = ( this->off + 8 + len <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool resize( size_t len ) noexcept;
  size_t update_hdr( void ) {
    this->buf[ 0 ] = 0x11;
    this->buf[ 1 ] = 0x11;
    this->buf[ 2 ] = 0x11;
    this->buf[ 3 ] = 0x12;
    this->buf[ 4 ] = ( this->off >> 24 ) & 0xffU;
    this->buf[ 5 ] = ( this->off >> 16 ) & 0xffU;
    this->buf[ 6 ] = ( this->off >> 8 ) & 0xffU;
    this->buf[ 7 ] = this->off & 0xffU;
    return this->off + 8;
  }
  template< class T >
  TibSassMsgWriter & append_type( MDFid fid,  T val,  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
  }
  template< class T >
  TibSassMsgWriter & append_type( const char *fname,  size_t fname_len,  T val,
                                  MDType t ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) &val;
    mref.fsize   = sizeof( val );
    mref.ftype   = t;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  template< class T >
  TibSassMsgWriter & append_int( MDFid fid,  T ival ) {
    return this->append_type( fid, ival, MD_INT );
  }
  template< class T >
  TibSassMsgWriter & append_uint( MDFid fid,  T uval ) {
    return this->append_type( fid, uval, MD_UINT );
  }
  template< class T >
  TibSassMsgWriter & append_real( MDFid fid,  T rval ) {
    return this->append_type( fid, rval, MD_REAL );
  }

  template< class T >
  TibSassMsgWriter & append_int( const char *fname,  size_t fname_len,
                                 T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  TibSassMsgWriter & append_uint( const char *fname,  size_t fname_len,
                                  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  TibSassMsgWriter & append_real( const char *fname,  size_t fname_len,
                                  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }

  TibSassMsgWriter & append_string( MDFid fid,  const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fid, mref );
  }

  TibSassMsgWriter & append_string( const char *fname,  size_t fname_len,
                                    const char *str,  size_t len ) {
    MDReference mref;
    mref.fptr    = (uint8_t *) (void *) str;
    mref.fsize   = len;
    mref.ftype   = MD_STRING;
    mref.fendian = md_endian;
    return this->append_ref( fname, fname_len, mref );
  }

  TibSassMsgWriter & append_decimal( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDDecimal &dec, const MDFormEntry *entry ) noexcept;
  TibSassMsgWriter & append_time( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDTime &time,  const MDFormEntry *entry ) noexcept;
  TibSassMsgWriter & append_date( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDDate &date,  const MDFormEntry *entry ) noexcept;
  TibSassMsgWriter & append_enum( MDFid fid,  MDType ftype,  uint32_t fsize,
                           MDEnum &enu,  const MDFormEntry *entry ) noexcept;

  TibSassMsgWriter & append_decimal( MDFid fid,  MDDecimal &dec ) noexcept;
  TibSassMsgWriter & append_time( MDFid,  MDTime &time ) noexcept;
  TibSassMsgWriter & append_date( MDFid,  MDDate &date ) noexcept;

  TibSassMsgWriter & append_decimal( const char *fname,  size_t fname_len,
                                  MDDecimal &dec ) noexcept;
  TibSassMsgWriter & append_time( const char *fname,  size_t fname_len,
                                  MDTime &time ) noexcept;
  TibSassMsgWriter & append_date( const char *fname,  size_t fname_len,
                                  MDDate &date ) noexcept;
  TibSassMsgWriter & append_enum( const char *fname,  size_t fname_len,
                                  MDEnum &enu ) noexcept;
  TibSassMsgWriter & append_iter( MDFieldIter *iter ) noexcept;
  int convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept;
};

}
} // namespace rai

#endif
#endif
