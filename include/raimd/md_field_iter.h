#ifndef __rai_raimd__md_field_iter_h__
#define __rai_raimd__md_field_iter_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

struct MDFieldIter { /* generic field iterator */
  MDMsg & iter_msg;
  size_t  field_start,   /* marks the area of a field, if msg is byte blob */
          field_end;

  MDFieldIter( MDMsg &m )
    : iter_msg( m ), field_start( 0 ), field_end( 0 ) {}

  /* these fuctions implemented in subclass */
  virtual int get_name( MDName &name ) noexcept;
  virtual int copy_name( char *name,  size_t &name_len,  MDFid &fid ) noexcept;
  virtual int get_reference( MDReference &mref ) noexcept;
  virtual int get_hint_reference( MDReference &mref ) noexcept;
  virtual int get_enum( MDReference &mref, MDEnum &enu ) noexcept;
  virtual int find( const char *name,  size_t name_len,
                    MDReference &mref ) noexcept;
  int find( const char *name,  MDReference &mref ) {
    return this->find( name, ::strlen( name ), mref );
  }
  virtual int first( void ) noexcept;
  virtual int next( void ) noexcept;

  /* escaped strings is used for json escaping */
  size_t fname_string( char *fname_buf,  size_t &fname_len ) noexcept;
  int print( MDOutput *out, int indent_newline, const char *fname_fmt,
             const char *type_fmt ) noexcept;
  int print( MDOutput *out ) {
    return this->print( out, 1, "%-18s : " /* fname fmt */,
                                "%-10s %3d : " /* type fmt */);
  }
};

struct MDFieldReader { /* iterator but w bool return with err field */
  MDFieldIter * iter;
  MDReference   mref;
  int           err;

  MDFieldReader( MDMsg &m ) noexcept;
  bool find( const char *fname,  size_t fnamelen ) noexcept;
  bool find( const char *fname ) {
    return this->find( fname, ::strlen( fname ) );
  }
  bool name( MDName &n ) noexcept;
  bool first( void ) noexcept;
  bool next( void ) noexcept;
  bool first( MDName &n ) {
    return this->first() && this->name( n );
  }
  bool next( MDName &n ) {
    return this->next() && this->name( n );
  }
  MDType type( void ) noexcept;
  bool get_value( void *val,  size_t len,  MDType t ) noexcept;
  template <class I>
  bool get_int( I &ival ) {
    return this->get_value( &ival, sizeof( I ), MD_INT );
  }
  template <class I>
  bool get_uint( I &ival ) {
    return this->get_value( &ival, sizeof( I ), MD_UINT );
  }
  template <class F>
  bool get_real( F &fval ) {
    return this->get_value( &fval, sizeof( F ), MD_REAL );
  }
  bool get_string( char *&sval,  size_t &slen ) noexcept;
  bool get_string( char *buf, size_t buflen, size_t &blen ) noexcept;
  bool get_time( MDTime &time ) {
    return this->get_value( &time, sizeof( MDTime ), MD_TIME );
  }
  bool get_date( MDDate &date ) {
    return this->get_value( &date, sizeof( MDDate ), MD_DATE );
  }
  bool get_decimal( MDDecimal &dec ) {
    return this->get_value( &dec, sizeof( MDDecimal ), MD_DECIMAL );
  }
  bool get_sub_msg( MDMsg *&msg ) noexcept;
};

struct MDIterMap {
  const char * fname;
  size_t       fname_len;
  void       * fptr;
  size_t       fsize, elem_fsize;
  MDType       ftype, elem_ftype;
  uint32_t   * elem_count;

  MDIterMap() : fname( 0 ), fptr( 0 ), fsize( 0 ), elem_fsize( 0 ),
    ftype( MD_NODATA ), elem_ftype( MD_NODATA ), elem_count( 0 ) {}

  void elem( const char *fn,  void *fp,  size_t sz,  MDType ft ) {
    this->ftype     = ft; 
    this->fname     = fn;
    this->fname_len = ::strlen( fn );
    this->fptr      = fp;
    this->fsize     = sz;
  }
  void string( const char *fn,  void *fp,  size_t sz ) {
    this->elem( fn, fp, sz, MD_STRING );
  }
  void opaque( const char *fn,  void *fp,  size_t sz ) {
    this->elem( fn, fp, sz, MD_OPAQUE );
  }
  void uint( const char *fn,  void *fp,  size_t sz ) {
    this->elem( fn, fp, sz, MD_UINT );
  }
  template <class I>
  void uint( const char *fn,  I &ival ) {
    this->elem( fn, &ival, sizeof( I ), MD_UINT );
  }
  void sint( const char *fn,  void *fp,  size_t sz ) {
    this->elem( fn, fp, sz, MD_INT );
  }
  template <class I>
  void sint( const char *fn,  I &ival ) {
    this->elem( fn, &ival, sizeof( I ), MD_INT );
  }
  void array( const char *fn,  void *fp,  uint32_t *elcnt,
              size_t arsz,  size_t elsz,  MDType eltp ) {
    this->elem( fn, fp, arsz, MD_ARRAY );
    this->elem_fsize = elsz;
    this->elem_ftype = eltp;
    this->elem_count = elcnt;
  }
  void uint_array( const char *fn,  void *fp,  uint32_t *elcnt,
                   size_t arsz,  size_t elsz ) {
    this->array( fn, fp, elcnt, arsz, elsz, MD_UINT );
  }
  void sint_array( const char *fn,  void *fp,  uint32_t *elcnt,
                   size_t arsz,  size_t elsz ) {
    this->array( fn, fp, elcnt, arsz, elsz, MD_INT );
  }
  void string_array( const char *fn,  void *fp,  uint32_t *elcnt,
                     size_t arsz,  size_t elsz ) {
    this->array( fn, fp, elcnt, arsz, elsz, MD_STRING );
  }
  void opaque_array( const char *fn,  void *fp,  uint32_t *elcnt,
                     size_t arsz,  size_t elsz ) {
    this->array( fn, fp, elcnt, arsz, elsz, MD_OPAQUE );
  }
  bool index_array( size_t i,  void *&ptr,  size_t &sz ) noexcept;
  bool copy_string( size_t i,  MDReference &mref ) noexcept;
  bool copy_uint( size_t i,  MDReference &mref ) noexcept;
  bool copy_sint( size_t i,  MDReference &mref ) noexcept;
  bool copy_array( MDMsg &msg,  MDReference &mref ) noexcept;

  static size_t get_map( MDMsg &msg,  MDIterMap *map,  size_t n,
                         MDFieldIter *iter = NULL ) noexcept;
};

}
} // namespace rai

#endif
