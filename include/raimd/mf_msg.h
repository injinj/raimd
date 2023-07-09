#ifndef __rai_raimd__mf_msg_h__
#define __rai_raimd__mf_msg_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

static const uint32_t MARKETFEED_TYPE_ID = 0x8ab3f4ae;

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
  virtual int get_enum( MDReference &mref,  MDEnum &enu ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_hint_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  int unpack( void ) noexcept;
};

}
} // namespace rai

#endif
