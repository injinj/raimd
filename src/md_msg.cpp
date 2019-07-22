#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

namespace rai {
namespace md {

static const uint32_t MAX_MSG_CLASS   = 64,
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

void
MDMatchGroup::add_match( MDMatch &ma ) /* add match to matchgroup */
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

static bool
cmp_wd( MDMatch &ma,  void *y )
{
  switch ( ma.len ) {
    case 0: return true;
    case 1: return ma.buf[ 0 ] == ((uint8_t *) y)[ 0 ];
    case 2: return icmp<uint16_t>( ma.buf, y );
    case 4: return icmp<uint32_t>( (void *) ma.buf, y );
    case 8: return icmp<uint64_t>( (void *) ma.buf, y );
    default: return ::memcmp( ma.buf, y, ma.len ) == 0;
  }
}

MDMsg *
MDMatchGroup::match( void *bb,  size_t off,  size_t end,
                     uint32_t h,  MDDict *d,  MDMsgMem *m ) /* try matchgroup */
{
  uint16_t i   = 1;
  void   * val = NULL;
  /* check that msg data at offset has a match */
  if ( this->haslen ) {
    val = (void *) &((uint8_t *) bb)[ off + this->off ];
    i = this->xoff[ *(uint8_t *) val ]; /* if char matches, i > 0 */
    if ( i == 0 )
      return NULL;
  }
  /* start at offset which matches and go to end */
  for ( ; i <= this->count; i++ ) {
    MDMatch & ma = *this->matches[ i - 1 ];
    if ( this->off + ma.len <= end ) {
      if ( cmp_wd( ma, val ) ) {
        MDMsg *msg = ma.unpack( bb, off, end, h, d, m );
        if ( msg != NULL )
          return msg;
      }
    }
  }
  return NULL;
}

MDMatch *
MDMatchGroup::is_msg_type( void *bb,  size_t off,  size_t end,  uint32_t h )
{
  uint16_t i   = 1;
  void   * val = NULL;
  /* check that msg data at offset has a match */
  if ( this->haslen ) {
    val = (void *) &((uint8_t *) bb)[ off + this->off ];
    i = this->xoff[ *(uint8_t *) val ]; /* if char matches, i > 0 */
    if ( i == 0 )
      return NULL;
  }
  /* start at offset which matches and go to end */
  for ( ; i <= this->count; i++ ) {
    MDMatch & ma = *this->matches[ i - 1 ];
    if ( this->off + ma.len <= end ) {
      if ( cmp_wd( ma, val ) ) {
        if ( ma.is_msg_type( bb, off, end, h ) )
          return &ma;
      }
    }
  }
  return NULL;
}

void
MDMsg::add_match( MDMatch &ma ) /* add msg matcher to a match group */
{
  MDMatchGroup * mg = NULL;
  uint32_t i;
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
  else { /* has a match length, find match group by offset */
    for ( i = 0; i < md_group_cnt; i++ ) {
      mg = md_match_group[ i ];
      if ( mg->haslen != 0 && mg->off == ma.off )
        break;
    }
  }
  /* a match group at offset already exists */
  if ( i < md_group_cnt )
    mg->add_match( ma );
  else { /* create a new match group for offset */
    p  = ::malloc( sizeof( MDMatchGroup ) );
    mg = new ( p ) MDMatchGroup();
    mg->add_match( ma );

    md_match_group[ md_group_cnt++ ] = mg;
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
               MDDict *d,  MDMsgMem *m )
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
MDMsg::is_msg_type( void *bb,  size_t off,  size_t end,  uint32_t h )
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

void
MDMsgMem::alloc_slow( size_t size,  void *ptr )
{
  void  * p,
       ** area = (void **) this->mem[ 0 ],
       ** next;
  /* use malloc() for msg mem when local stack storage is exhausted */
  if ( size > MEM_CNT - 1 )
    p = ::malloc( sizeof( void * ) * ( size + 1 ) );
  else
    p = ::malloc( sizeof( void * ) * MEM_CNT );
  next = (void **) p;
  next[ 0 ] = (void *) area;
  this->mem[ 0 ] = (void *) next;
  area = next;
  this->mem_off = 1 + size;
  *(void **) ptr = &area[ 1 ];
}

void
MDMsgMem::extend( size_t old_size,  size_t new_size,  void *ptr )
{
  void  ** area = (void **) this->mem[ 0 ],
         * tmp;
  uint32_t new_sz = this->mem_size( new_size ),
           old_sz = this->mem_size( old_size );
  /* a realloc() the last data alloced, if possible */
  if ( this->mem_off > old_sz &&
       this->mem_off + new_sz <= MEM_CNT ) {
    if ( (void *) &area[ this->mem_off - old_sz ] == *(void **) ptr ) {
      this->mem_off += ( new_sz - old_sz );
      return;
    }
  }
  this->alloc( new_size, &tmp );
  ::memcpy( tmp, *(void **) ptr, old_size );
  *(void **) ptr = tmp;
}

void
MDMsgMem::reuse( void )
{
  /* release malloc()ed mem */
  while ( this->mem[ 0 ] != (void *) this->mem ) {
    void * next = this->mem[ 0 ];
    this->mem[ 0 ] = ((void **) next)[ 0 ];
    ::free( next );
  }
  /* reset offset where alloc starts */
  this->mem_off = 1;
}

void
MDMsg::print( MDOutput *out,  int indent_newline,  const char *fname_fmt,
              const char *type_fmt )
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
Err::err( int status )
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
  /* 39 */ { NO_PARTIAL,            "No partial", mod },
  /* 40 */ { FILE_NOT_FOUND,        "File not found", mod },
  /* 41 */ { DICT_PARSE_ERROR,      "Dictionary parse error", mod },
  /* 42 */ { ALLOC_FAIL,            "Allocation failed", mod },
  /* 43 */ { BAD_SUBJECT,           "Bad subject", mod },
  /* 44 */ { 44,                    "Unk", mod }
  };
  static const uint32_t num = sizeof( err ) / sizeof( err[ 0 ] ) - 1;

  return &err[ (uint32_t) status < num ? (uint32_t) status : num ];
}

