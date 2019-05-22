#ifndef __rai_raimd__md_msg_h__
#define __rai_raimd__md_msg_h__

#include <raimd/md_field_iter.h>

namespace rai {
namespace md {

struct MDMsgMem {
  static const uint32_t NO_REF_COUNT = 0x7fffffffU,
                        MEM_CNT      = 255;
  uint32_t mem_off, /* offset in mem[] where next is allocated */
           ref_cnt; /* incremented when attached to a MDMsg */
  void   * mem[ MEM_CNT ];

  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  /* ref count should be set when created dynamically,
   * this default is set to not release memory based on refs */
  MDMsgMem() : ref_cnt( NO_REF_COUNT ) {
    this->mem[ 0 ] = (void *) this->mem;
    this->mem_off  = 1;
  }
  ~MDMsgMem() {
    if ( this->mem[ 0 ] != (void *) this->mem )
      this->reuse();
  }

  static uint32_t mem_size( size_t sz ) {
    return ( sz + sizeof( void * ) - 1 ) / sizeof( void * );
  }
  void alloc( size_t size,  void *ptr ) {
    size = this->mem_size( size );
    if ( size + this->mem_off <= MEM_CNT ) {
      void ** area = (void **) this->mem[ 0 ];
      *(void **) ptr = &area[ this->mem_off ];
      this->mem_off += size;
      return;
    }
    this->alloc_slow( size, ptr );
  }
  /* when out of static space, add mem block using malloc() */
  void alloc_slow( size_t size,  void *ptr );
  /* extend last alloc for more space */
  void extend( size_t old_size,  size_t new_size,  void *ptr );
  /* alloc for strings */
  char *stralloc( size_t len,  const char *str ) {
    char * s = NULL;
    if ( str != NULL ) {
      this->alloc( len + 1, &s );
      ::memcpy( s, str, len );
      s[ len ] = '\0';
    }
    return s;
  }
  void reuse( void ); /* reuse all memory (overwritting existing) */
};

struct MDMsgMemSwap { /* push new local allocation for the current stack */
  MDMsgMem *& sav, * old;
  MDMsgMem    tmp;

  MDMsgMemSwap( MDMsgMem *&swap ) : sav( swap ), old( swap ) {
    swap = &this->tmp;
  }
  ~MDMsgMemSwap() {
    this->sav = this->old;
  }
};

struct MDDict;
struct MDMsg;

typedef bool (*md_is_msg_type_f)( void *bb,  size_t off,  size_t end,
                                  uint32_t h );
typedef MDMsg *(*md_msg_unpack_f)( void *bb,  size_t off,  size_t end,
                                   uint32_t h,  MDDict *d,  MDMsgMem *m );

struct MDMatch { /* match msg features in the header */
  uint8_t  off,       /* offset of feature */
           len,       /* length of feature match */
           hint_size, /* an external hint[] words */
           ftype;
  uint8_t  buf[ 4 ];  /* the values to match against the offset */
  uint32_t hint[ 2 ]; /* external hints */

  md_is_msg_type_f is_msg_type; /* test whether msg matches */
  md_msg_unpack_f  unpack;      /* wrap the msg in a decoder */
};

struct MDMatchGroup {
  MDMatch ** matches;     /* matches at this offset */
  uint8_t    off, haslen; /* the offset */
  uint16_t   count;       /* the count of matches[] */
  uint8_t    xoff[ 256 ]; /* the values at off which are valid */
    /* uint8_t x = xoff[ msgdata[ off ] ]
       MDMatch *match = x > 0 ? matches[ x - 1 ] : NULL;
       if ( match != NULL && match->is_msg_type( msgdata ) )
         match->unpack( msgdata ) */
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  MDMatchGroup() : matches( 0 ), off( 0 ), haslen( 0 ), count( 0 ) {
    ::memset( this->xoff, 0, sizeof( this->xoff ) );
  }
  void add_match( MDMatch &ma );
  MDMsg * match( void *bb,  size_t off,  size_t end,  uint32_t h,
                 MDDict *d,  MDMsgMem *m );
};

struct MDMsg {
  void     * msg_buf;
  size_t     msg_off,    /* offset where message starts */
             msg_end;    /* end offset of msg_buf */
  MDDict   * dict;
  MDMsgMem * mem; /* ref count increment in unpack() and get_sub_msg()
                     ref count decrement in release() */
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void * ) { /*::free( ptr );*/ } /* do nothing */

  MDMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : msg_buf( bb ), msg_off( off ), msg_end( end ), dict( d ), mem( m ) {}
  ~MDMsg() { this->release(); }

  static void add_match( MDMatch &ma );
  /* Unpack message and make available for extracting field values */
  static MDMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h = 0,
                        MDDict *d = NULL, MDMsgMem *m = NULL );
  /* dereference msg mem */
  void release( void ) {
    if ( this->mem != NULL && this->mem->ref_cnt != MDMsgMem::NO_REF_COUNT ) {
      MDMsgMem *m = this->mem;
      this->mem = NULL; /* prevent multiple derefs */
      if ( --m->ref_cnt == 0 )
        delete m;
    }
  }
  /* Print message in 'TIB' format to output stream */
  void print( MDOutput *out ) {
    this->print( out, 1, "%-18s : " /* fname fmt */,
                         "%-10s %3d : " /* type fmt */);
  }
  /* Print message with printf style fmts */
  void print( MDOutput *out,  int indent_newline,  const char *fname_fmt,
              const char *type_fmt );
  /* Print buffer hex values */
  static void print_hex( MDOutput *out,  const void *msgBuf,
                         size_t offset,  size_t length );
  /* Used by field iterators to creae a sub message */
  virtual const char *get_proto_string( void );
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg );
  virtual int get_reference( MDReference &mref );
  virtual int get_field_iter( MDFieldIter *&iter );
  virtual int get_array_ref( MDReference &mref, size_t i, MDReference &aref );

  int msg_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int hash_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int set_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int zset_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int geo_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int hll_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int get_quoted_string( MDReference &mref,  char *&buf,  size_t &len );
  int get_escaped_string( MDReference &mref,  const char *quotes,
                           char *&buf,  size_t &len );
  int get_string( MDReference &mref, char *&buf,  size_t &len );
  int time_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int array_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int list_to_string( MDReference &mref,  char *&buf,  size_t &len );
  int concat_array_to_string( char **str,  size_t *k,  size_t num_entries,
                              size_t tot_len,  char *&buf,  size_t &len );
};

}
} // namespace rai

#endif
