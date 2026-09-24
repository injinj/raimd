#ifndef __rai_raimd__mf_msg_h__
#define __rai_raimd__mf_msg_h__

#include <raimd/md_msg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t MARKETFEED_TYPE_ID = 0x8ab3f4ae;
#ifndef __cplusplus
#define MARKETFEED_TYPE_ID 0x8ab3f4aeU
#endif

MDMsg_t * mf_msg_unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                         MDDict_t *d,  MDMsgMem_t *m );
bool md_msg_mf_get_flist( MDMsg_t *m,  uint16_t *flist );
bool md_msg_mf_get_rtl( MDMsg_t *m,  uint32_t *rtl );
MDMsgWriter_t * mf_msg_writer_create( MDMsgMem_t *mem,  MDDict_t *d,
                                      void *buf_ptr,  size_t buf_sz );

#ifdef __cplusplus
}
namespace rai {
namespace md {

enum MktfdFunc { /* func kinds */
  Cmd_2            = 2,
  BcastMsg_4       = 4,
  Drop_308         = 308,
  Close_312        = 312,
  Upd_316          = 316,
  Correct_317      = 317,
  Verify_318       = 318,
  Rec_340          = 340,
  Snap_342         = 342,
  AggregateUpd_350 = 350,
  Status_407       = 407
};

uint16_t mf_func_to_sass_msg_type( uint16_t func ) noexcept;

struct MktfdMsg : public MDMsg {
  size_t          data_start,
                  data_end;
  const uint8_t * tagp,    /* XX tag used by client (not used anymore) */
                * ricp,    /* the ric code (INTC.O) */
                * textp;   /* inline status string */
  size_t          taglen,  /* len of tagp */
                  riclen,  /* len of ricp */
                  textlen; /* len of textp */
  int32_t         rstatus, /* status code after RS:1e in hdr (verify) */
                  flist,   /* field list code after US:1f in hdr (initial) */
                  rtl,     /* used to be seqno */
                  status;  /* a status code */
  uint16_t        func;    /* the function: 308/316/318/340/... */

  int parse_header( void ) noexcept;

  /* used by unpack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  MktfdMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, end, d, m ), data_start( 0 ), data_end( 0 ),
      tagp( 0 ), ricp( 0 ), textp( 0 ), taglen( 0 ), riclen( 0 ), textlen( 0 ),
      rstatus( 0 ), flist( 0 ), rtl( 0 ), status( 0 ), func( 0 ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;

  /* may return tibmsg, sass qform or rv */
  static bool is_marketfeed( void *bb,  size_t off,  size_t end,
                             uint32_t h ) noexcept;
  static MktfdMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                           MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
  virtual int create_writer( MDMsgWriterBase *&wr,  MDMsgMem &mem,  MDDict *d,
                             void *bb,  size_t len ) noexcept final;
};

struct MDLookup;
/* Marketfeed record encoder, the MF counterpart of RwfMsgWriter +
 * RwfFieldListWriter (and of TibSassMsgWriter for the SASS form of the
 * same data).  A record is:
 *
 *   <FS>func<US>tag<GS>ric[<RS>rstatus][<US>flist]<US>rtl[<GS>status[<RS>text]]
 *   { <RS>fid<US>value }* <FS>
 *
 * The header pieces are written in that order by start() + add_*(); the
 * first append_*() closes the header.  Field values are the ASCII forms
 * the ADS produces for SSL sinks (from captures against ADS 3.x): REAL as
 * signed decimal or fraction ("+64 4/8", "-0.20", "+13126500"), UINT and
 * ENUM as plain digits, TIME "HH:MM" or "HH:MM:SS" by the dictionary MF
 * length, DATE "DD MON YYYY", strings space padded to the MF length.
 * Types and lengths come from the app_a / RDM dictionary when present. */
struct MktfdMsgWriter : public MDMsgWriterBase {
  MDDict * dict;      /* dictionary with MF types and lengths (app_a) */
  size_t   hdr_off;   /* offset where the field data starts */
  uint16_t func;      /* 340, 316, 318, ... */
  bool     in_hdr,    /* header still open, add_*() allowed */
           closed;    /* trailing <FS> written */

  void * operator new( size_t, void *ptr ) { return ptr; }
  MktfdMsgWriter( MDMsgMem &m,  MDDict *d,  void *bb,  size_t len ) noexcept;

  MktfdMsgWriter & error( int status ) {
    if ( this->err == 0 )
      this->err = status;
    return *this;
  }
  void reset( void ) {
    this->off = 0; this->err = 0; this->hdr_off = 0; this->func = 0;
    this->in_hdr = false; this->closed = false;
  }
  bool has_space( size_t len ) {
    bool b = ( this->off + len + 1 <= this->buflen );
    if ( ! b ) b = this->resize( len );
    return b;
  }
  bool resize( size_t len ) noexcept;

  /* header: <FS>func<US>tag<GS>ric */
  MktfdMsgWriter & start( uint16_t func,  const char *ric,  size_t riclen,
                          const char *tag = "XX",  size_t taglen = 2 ) noexcept;
  MktfdMsgWriter & add_rstatus( int32_t rstatus ) noexcept; /* <RS>n  340/342/318 */
  MktfdMsgWriter & add_flist( uint16_t flist ) noexcept;    /* <US>n  340/342/318 */
  MktfdMsgWriter & add_rtl( uint32_t rtl ) noexcept;        /* <US>n */
  MktfdMsgWriter & add_rtl( void ) noexcept;                /* <US>   (empty, updates) */
  MktfdMsgWriter & add_status( int32_t status,  const char *text = NULL,
                               size_t textlen = 0 ) noexcept; /* <GS>n[<RS>text] */
  void end_hdr( void ) { this->in_hdr = false; this->hdr_off = this->off; }

  /* fields: <RS>fid<US>value */
  MktfdMsgWriter & append_raw( MDFid fid,  const char *val,
                               size_t vallen ) noexcept;
  MktfdMsgWriter & append_ref( MDFid fid,  MDReference &mref ) noexcept;
  MktfdMsgWriter & append_ref( const char *fname,  size_t fname_len,
                               MDReference &mref ) noexcept;
  MktfdMsgWriter & append_ref( const char *fname,  MDReference &mref ) {
    return this->append_ref( fname, ::strlen( fname ), mref );
  }
  MktfdMsgWriter & append_ref( MDLookup &by,  MDReference &mref ) noexcept;
  MktfdMsgWriter & append_string( MDFid fid,  const char *s,  size_t slen ) {
    MDReference mref( (void *) s, slen, MD_STRING, md_endian );
    return this->append_ref( fid, mref );
  }
  MktfdMsgWriter & append_string( const char *fname,  size_t fname_len,
                                  const char *s,  size_t slen ) {
    MDReference mref( (void *) s, slen, MD_STRING, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  MktfdMsgWriter & append_decimal( MDFid fid,  MDDecimal &dec ) {
    MDReference mref( (void *) &dec, sizeof( dec ), MD_DECIMAL, md_endian );
    return this->append_ref( fid, mref );
  }
  MktfdMsgWriter & append_time( MDFid fid,  MDTime &time ) {
    MDReference mref( (void *) &time, sizeof( time ), MD_TIME, md_endian );
    return this->append_ref( fid, mref );
  }
  MktfdMsgWriter & append_date( MDFid fid,  MDDate &date ) {
    MDReference mref( (void *) &date, sizeof( date ), MD_DATE, md_endian );
    return this->append_ref( fid, mref );
  }
  template< class T >
  MktfdMsgWriter & append_type( MDFid fid,  T val,  MDType t ) {
    MDReference mref( (void *) &val, sizeof( val ), t, md_endian );
    return this->append_ref( fid, mref );
  }
  template< class T >
  MktfdMsgWriter & append_type( const char *fname,  size_t fname_len,  T val,
                                MDType t ) {
    MDReference mref( (void *) &val, sizeof( val ), t, md_endian );
    return this->append_ref( fname, fname_len, mref );
  }
  template< class T >
  MktfdMsgWriter & append_int( MDFid fid,  T ival ) {
    return this->append_type( fid, ival, MD_INT );
  }
  template< class T >
  MktfdMsgWriter & append_uint( MDFid fid,  T uval ) {
    return this->append_type( fid, uval, MD_UINT );
  }
  template< class T >
  MktfdMsgWriter & append_enum( MDFid fid,  T eval ) {
    return this->append_type( fid, eval, MD_ENUM );
  }
  template< class T >
  MktfdMsgWriter & append_real( MDFid fid,  T rval ) {
    return this->append_type( fid, rval, MD_REAL );
  }
  template< class T >
  MktfdMsgWriter & append_int( const char *fname,  size_t fname_len,  T ival ) {
    return this->append_type( fname, fname_len, ival, MD_INT );
  }
  template< class T >
  MktfdMsgWriter & append_uint( const char *fname,  size_t fname_len,  T uval ) {
    return this->append_type( fname, fname_len, uval, MD_UINT );
  }
  template< class T >
  MktfdMsgWriter & append_real( const char *fname,  size_t fname_len,  T rval ) {
    return this->append_type( fname, fname_len, rval, MD_REAL );
  }
  /* copy the fields of another message (rwf, sass, ...) */
  virtual int append_iter( MDFieldIter *iter ) noexcept override final;
  /* all fields of msg; skip_hdr drops the SASS MSG_TYPE/REC_TYPE/SEQ_NO/
   * REC_STATUS/SYMBOL fields (the caller writes the MF header instead) */
  virtual int convert_msg( MDMsg &msg,  bool skip_hdr ) noexcept override final;
  /* MSG_TYPE/REC_TYPE/SEQ_NO/REC_STATUS as fields, the SASS convention */
  virtual int append_sass_hdr( MDFormClass *form, uint16_t msg_type,
                               uint16_t rec_type, uint16_t seqno,
                               uint16_t status, const char *subj,
                               size_t sublen ) noexcept override final;
  /* write the trailing <FS>, return the record length */
  virtual size_t update_hdr( void ) noexcept override final;
};

struct MktfdFieldIter : public MDFieldIter {
  MDType       ftype;    /* field type from dictionary */
  uint32_t     fsize;    /* length of string, size of int */
  const char * fname;    /* name associated with fid */
  uint8_t      fnamelen; /* len of fname */
  MDValue      val;      /* union of temp values */
  MDFid        fid;      /* field fid, used for dictionary lookup */
  MDDecimal    dec;      /* if price */
  MDTime       time;     /* if time field */
  MDDate       date;     /* if data field */
  size_t       data_off; /* position after <US> where data starts */
  uint16_t     position, /* partial offset / escape position */
               repeat;   /* repeat code / escape code */
  uint8_t      rep_buf[ 128 ]; /* for repeat escape codes */

  /* used by GetFieldIterator() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  MktfdFieldIter( MDMsg &m ) : MDFieldIter( m ), ftype( MD_NODATA ), fsize( 0 ),
      fname( 0 ), fnamelen( 0 ), fid( 0 ), data_off( 0 ), position( 0 ),
      repeat( 0 ) {}

  void lookup_fid( void ) noexcept;
  virtual int get_name( MDName &name ) noexcept final;
  virtual int set_name( const char *fname,  size_t fnamelen,
                        MDName &name ) noexcept final;
  virtual int get_enum( MDReference &mref,  MDEnum &enu ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_hint_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int find_next( const char *name, size_t name_len,
                         MDReference &mref ) noexcept final;
  virtual int find( const MDName &n,  MDReference &mref ) noexcept final;
  virtual int find_next( const MDName &n,  MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack( void ) noexcept;
};

}
} // namespace rai

#endif
#endif
