#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

extern "C" {
#define md_stringify(S) md_str(S)
#define md_str(S) #S

const char *
md_get_version( void )
{
  return md_stringify( MD_VER );
}
}

namespace rai {
namespace md {

static const uint32_t MAX_MSG_CLASS   = 256,
                      MATCH_HASH_SIZE = 256;

static uint32_t md_group_cnt,    /* size of md_match_group */
                md_add_cnt;      /* size of md_match_arr */

static MDMatchGroup * md_match_group[ MAX_MSG_CLASS ];
static MDMatch      * md_match_arr[ MAX_MSG_CLASS ];

static struct {
  uint32_t hint, idx;
} md_match_hash[ MATCH_HASH_SIZE ];

static uint8_t md_match_ftype[ MATCH_HASH_SIZE ];

}
}

MDMatch *
MDMsg::first_match( uint32_t &i ) noexcept
{
  i = 0;
  if ( i == md_add_cnt )
    return NULL;
  return md_match_arr[ i ];
}

MDMatch *
MDMsg::next_match( uint32_t &i ) noexcept
{
  if ( ++i >= md_add_cnt )
    return NULL;
  return md_match_arr[ i ];
}

void
MDMatchGroup::add_match( MDMatch &ma ) noexcept /* add match to matchgroup */
{
  if ( this->count == 0 ) {
    this->off    = ma.off;
    this->haslen = ma.len;
  }
  void *m = ::realloc( this->matches, 
                       sizeof( this->matches[ 0 ] ) * ( this->count + 1 ) );
  this->matches = (MDMatch **) m;
  this->matches[ this->count++ ] = &ma;
  if ( this->xoff[ ma.buf[ 0 ] ] == 0 ) {
    uint8_t x = (uint8_t) ( this->count < 0xff ? this->count : 0xff );
    this->xoff[ ma.buf[ 0 ] ] = x;
  }
}

template <class Type> inline bool
icmp( void *p,  void *q )
{
  Type x, y;
  ::memcpy( &x, p, sizeof( Type ) );
  ::memcpy( &y, q, sizeof( Type ) );
  return ( x == y );
}

static inline bool
cmp_wd( MDMatch &ma,  void *y )
{
  switch ( ma.len ) {
    case 0: return true;
    case 1: return ma.buf[ 0 ] == ((uint8_t *) y)[ 0 ];
    case 2: return icmp<uint16_t>( ma.buf, y );
    case 4: return icmp<uint32_t>( (void *) ma.buf, y );
    case 8: return icmp<uint64_t>( (void *) ma.buf, y );
    case 0xff: return false;
    default: return ::memcmp( ma.buf, y, ma.len ) == 0;
  }
}

MDMsg *
MDMatchGroup::match2( void *bb,  size_t off,  size_t end,  uint32_t h,
                      MDDict *d,  MDMsgMem &m,  uint16_t i ) noexcept
{
  /* start at offset which matches and go to end */
  for ( ; i <= this->count; i++ ) {
    MDMatch & ma = *this->matches[ i - 1 ];
    if ( this->off + ma.len <= end ) {
      if ( cmp_wd( ma, &((uint8_t *) bb)[ off + this->off ] ) ) {
        MDMsg *msg = ma.unpack( bb, off, end, h, d, m );
        if ( msg != NULL )
          return msg;
      }
    }
  }
  return NULL;
}

MDMatch *
MDMatchGroup::is_msg_type2( void *bb,  size_t off,  size_t end,
                            uint32_t h,  uint16_t i ) noexcept
{
  /* start at offset which matches and go to end */
  for ( ; i <= this->count; i++ ) {
    MDMatch & ma = *this->matches[ i - 1 ];
    if ( this->off + ma.len <= end ) {
      if ( cmp_wd( ma, &((uint8_t *) bb)[ off + this->off ] ) ) {
        if ( ma.is_msg_type( bb, off, end, h ) )
          return &ma;
      }
    }
  }
  return NULL;
}

void
MDMsg::add_match( MDMatch &ma ) noexcept /* add msg matcher to a match group */
{
  MDMatchGroup * mg = NULL;
  uint32_t i = 0;
  void   * p;

  if ( md_add_cnt == MAX_MSG_CLASS )
    return;
  for ( i = 0; i < md_add_cnt; i++ )
    if ( md_match_arr[ i ] == &ma )
      return;
  md_match_arr[ md_add_cnt++ ] = &ma;

  if ( ma.ftype > 0 )
    md_match_ftype[ ma.ftype ] = md_add_cnt;
  if ( ma.len == 0 ) {  /* if no length, nothing to match, always try it */
    for ( i = 0; i < md_group_cnt; i++ ) {
      mg = md_match_group[ i ];
      if ( mg->haslen == 0 )
        break;
    }
  }
  else if ( ma.len != 0xff ) { /* has a match length, find match group by off */
    for ( i = 0; i < md_group_cnt; i++ ) {
      mg = md_match_group[ i ];
      if ( mg->haslen != 0 && mg->off == ma.off )
        break;
    }
  }
  if ( ma.len != 0xff ) {
    /* a match group at offset already exists */
    if ( i < md_group_cnt )
      mg->add_match( ma );
    else { /* create a new match group for offset */
      p  = ::malloc( sizeof( MDMatchGroup ) );
      mg = new ( p ) MDMatchGroup();
      mg->add_match( ma );

      md_match_group[ md_group_cnt++ ] = mg;
    }
  }
  /* if msg class has a external type hash */
  if ( ma.hint_size > 0 ) {
    for ( i = 0; i < ma.hint_size; i++ ) {
      uint32_t j = ma.hint[ i ] % MATCH_HASH_SIZE;
      while ( md_match_hash[ j ].hint != 0 )
        j = ( j + 1 ) % MATCH_HASH_SIZE;
      md_match_hash[ j ].hint = ma.hint[ i ];
      md_match_hash[ j ].idx  = md_add_cnt - 1; /* index of md_match_arr[] */
    }
  }
}

MDMsg *
MDMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
               MDDict *d,  MDMsgMem &m ) noexcept
{
  MDMatchGroup * mg;
  MDMsg        * msg;
  uint32_t       i, j;
  /* if nothing init, use auto unpack */
  if ( md_add_cnt == 0 )
    md_init_auto_unpack();
  /* find decoder by md_match_hash[ h ] */
  if ( h != 0 ) {
    if ( h < MATCH_HASH_SIZE ) {
      if ( (i = md_match_ftype[ h ]) != 0 ) {
        MDMatch &ma = *md_match_arr[ i - 1 ];
        if ( (msg = ma.unpack( bb, off, end, h, d, m )) != NULL )
          return msg;
      }
    }
    j = h % MATCH_HASH_SIZE;
    for (;;) {
      if ( md_match_hash[ j ].hint == h ) {
        i = md_match_hash[ j ].idx;
        MDMatch &ma = *md_match_arr[ i ];
        if ( (msg = ma.unpack( bb, off, end, h, d, m )) != NULL )
          return msg;
      }
      else if ( md_match_hash[ j ].hint == 0 )
        break;
      j = ( j + 1 ) % MATCH_HASH_SIZE;
    }
  }
  /* find decoder by match group offset -- char at bb offset selects decoder */
  for ( i = 0; i < md_group_cnt; i++ ) {
    mg = md_match_group[ i ];
    if ( (msg = mg->match( bb, off, end, h, d, m )) != NULL )
      return msg;
  }
  return NULL;
}

uint32_t
MDMsg::is_msg_type( void *bb,  size_t off,  size_t end,  uint32_t h ) noexcept
{
  MDMatchGroup * mg;
  MDMatch      * ma;
  uint32_t       i, j;
  /* if nothing init, use auto unpack */
  if ( md_add_cnt == 0 )
    md_init_auto_unpack();
  /* find decoder by md_match_hash[ h ] */
  if ( h != 0 ) {
    if ( h < MATCH_HASH_SIZE ) {
      if ( (i = md_match_ftype[ h ]) != 0 ) {
        ma = md_match_arr[ i - 1 ];
        if ( ma->is_msg_type( bb, off, end, h ) )
          goto found_msg_type;
      }
    }
    j = h % MATCH_HASH_SIZE;
    for (;;) {
      if ( md_match_hash[ j ].hint == h ) {
        i = md_match_hash[ j ].idx;
        ma = md_match_arr[ i ];
        if ( ma->is_msg_type( bb, off, end, h ) )
          goto found_msg_type;
      }
      else if ( md_match_hash[ j ].hint == 0 )
        break;
      j = ( j + 1 ) % MATCH_HASH_SIZE;
    }
  }
  /* find decoder by match group offset -- char at bb offset selects decoder */
  for ( i = 0; i < md_group_cnt; i++ ) {
    mg = md_match_group[ i ];
    if ( (ma = mg->is_msg_type( bb, off, end, h )) != NULL )
      goto found_msg_type;
  }
  return 0;

found_msg_type:;
  if ( ma->hint[ 0 ] != 0 )
    return ma->hint[ 0 ];
  return ma->ftype;
}

void *
MDMsgMem::alloc_slow( size_t size ) noexcept
{
  MemBlock * p;
  size_t     alloc_size = sizeof( MemBlock ) + sizeof( void * ),
             edge;
  edge = MEM_CNT;
  if ( size + 1 > MEM_CNT ) {
    edge = size;
    alloc_size += ( size + 1 - MEM_CNT ) * sizeof( void * );
  }
  /* use malloc() for msg mem when local stack storage is exhausted */
  p = (MemBlock *) ::malloc( alloc_size );
  p->next = this->blk_ptr;
  p->size = edge;
  this->blk_ptr = p;
#if __GNUC__ >= 10
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
  p->mem[ edge ] = p; /* mark trailing edge to error on array overflow */
#if __GNUC__ >= 10
#pragma GCC diagnostic pop
#endif
  this->mem_off = (uint32_t) size;
  return p->mem;
}

void
MDMsgMem::extend( size_t old_size,  size_t new_size,  void *ptr ) noexcept
{
  size_t new_sz = this->align_size( new_size ),
         old_sz = this->align_size( old_size );
  /* a realloc() the last data alloced, if possible */
  if ( (size_t) this->mem_off >= old_sz &&
   (void *) &this->blk_ptr->mem[ this->mem_off - old_sz ] == *(void **) ptr ) {
    size_t new_off = (size_t) this->mem_off - old_sz + new_sz;
    if ( new_off <= MEM_CNT ) {
      this->mem_off = new_off;
      return;
    }
  }
  void * tmp;
  this->alloc( new_size, &tmp );
  ::memcpy( tmp, *(void **) ptr, old_size );
  *(void **) ptr = tmp;
}

void
MDMsgMem::release( void ) noexcept
{
  /* release malloc()ed mem */
  while ( this->blk_ptr != &this->blk ) {
    MemBlock * next = this->blk_ptr->next;
    if ( (void *) this->blk_ptr != this->blk_ptr->mem[ this->blk_ptr->size ] )
      this->error();
    else
      ::free( this->blk_ptr );
    this->blk_ptr = next;
  }
  this->mem_off = 0;
}

#include <stdio.h>
void
MDMsgMem::error( void ) noexcept
{
  fprintf( stderr, "**** MDMsgMem error blk_ptr %p self %p ****\n",
           this->blk_ptr, this->blk_ptr->mem[ this->blk_ptr->size ] );
}

void
MDMsg::print( MDOutput *out,  int indent_newline,  const char *fname_fmt,
              const char *type_fmt ) noexcept
{
  MDMsgMemSwap swap( this->mem ); /* temporary mem pushed on the stack */
  MDFieldIter *f;
  MDReference mref;
  /* if msg mem is a list of fields */
  if ( this->get_field_iter( f ) == 0 ) {
    if ( f->first() == 0 ) {
      do {
        f->print( out, indent_newline, fname_fmt, type_fmt );
      } while ( f->next() == 0 );
    }
  }
  /* otherwise msg is a single data element or array */
  else if ( this->get_reference( mref ) == 0 ) {
    char * buf;
    size_t len;
    if ( this->get_quoted_string( mref, buf, len ) == 0 ) {
      out->puts( buf );
      out->puts( "\n" );
    }
  }
}

MDError
Err::err( int status ) noexcept
{
  static const char       mod[] = "MD";
  static const MDErrorRec err[] = {
  /*  0 */ { 0,                     "OK", mod },
  /*  1 */ { BAD_MAGIC_NUMBER,      "Unable to find message class for data", mod },
  /*  2 */ { BAD_BOUNDS,            "Message boundary overflows length", mod },
  /*  3 */ { BAD_FIELD_SIZE,        "Unable to determine field size", mod },
  /*  4 */ { BAD_FIELD_TYPE,        "Unable to determine field type", mod },
  /*  5 */ { BAD_FIELD_BOUNDS,      "Field boundary overflows message boundary", mod },
  /*  6 */ { BAD_CVT_BOOL,          "Unable to convert field to boolean", mod },
  /*  7 */ { BAD_CVT_STRING,        "Unable to convert field to string", mod },
  /*  8 */ { BAD_CVT_NUMBER,        "Unable to convert field to number", mod },
  /*  9 */ { NOT_FOUND,             "Not found", mod },
  /* 10 */ { NO_DICTIONARY,         "No dictionary loaded", mod },
  /* 11 */ { UNKNOWN_FID,           "Unable to find FID definition", mod },
  /* 12 */ { NULL_FID,              "FID is null value", mod },
  /* 13 */ { BAD_HEADER,            "Bad header", mod },
  /* 14 */ { BAD_FIELD,             "Bad field", mod },
  /* 15 */ { BAD_DECIMAL,           "Bad decimal hint", mod },
  /* 16 */ { BAD_NAME,              "Name length overflow", mod },
  /* 17 */ { ARRAY_BOUNDS,          "Array bounds index overflow", mod },
  /* 18 */ { BAD_STAMP,             "Bad stamp", mod },
  /* 19 */ { BAD_DATE,              "Bad date", mod },
  /* 20 */ { BAD_TIME,              "Bad time", mod },
  /* 21 */ { BAD_SUB_MSG,           "Bad submsg", mod },
  /* 22 */ { INVALID_MSG,           "Unable to use message", mod },
  /* 23 */ { NO_MSG_IMPL,           "No message implementation", mod },
  /* 24 */ { EXPECTING_TRUE,        "Expected 'true' token", mod },
  /* 25 */ { EXPECTING_FALSE,       "Expected 'false' token", mod },
  /* 26 */ { EXPECTING_AR_VALUE,    "Expecting array value", mod },
  /* 27 */ { UNEXPECTED_ARRAY_END,  "Unterminated array, missing ']'", mod },
  /* 28 */ { EXPECTING_COLON,       "Expecting ':' in object", mod },
  /* 29 */ { EXPECTING_OBJ_VALUE,   "Expecting value in object", mod },
  /* 30 */ { UNEXPECTED_OBJECT_END, "Unterminated object, missing '}'", mod },
  /* 31 */ { UNEXPECTED_STRING_END, "Unterminated string, missing '\"'", mod },
  /* 32 */ { INVALID_CHAR_HEX,      "Can't string hex escape sequence", mod },
  /* 33 */ { EXPECTING_NULL,        "Expecting 'null' token", mod },
  /* 34 */ { NO_SPACE,              "Size check failed, no space", mod },
  /* 35 */ { BAD_CAST,              "Can't cast to number", mod },
  /* 36 */ { TOO_BIG,               "Json too large for parser", mod },
  /* 37 */ { INVALID_TOKEN,         "Invalid token", mod },
  /* 38 */ { NO_ENUM,               "No enum", mod },
  /* 39 */ { NO_FORM,               "No form", mod },
  /* 40 */ { NO_PARTIAL,            "No partial", mod },
  /* 41 */ { FILE_NOT_FOUND,        "File not found", mod },
  /* 42 */ { DICT_PARSE_ERROR,      "Dictionary parse error", mod },
  /* 43 */ { ALLOC_FAIL,            "Allocation failed", mod },
  /* 44 */ { BAD_SUBJECT,           "Bad subject", mod },
  /* 45 */ { BAD_FORMAT,            "Bad msg structure", mod },
  /* 46 */ { 46,                    "Unk", mod }
  };
  static const uint32_t num = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ (uint32_t) status < num ? (uint32_t) status : num ];
}

