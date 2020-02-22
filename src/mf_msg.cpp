#include <stdio.h>
#include <raimd/mf_msg.h>
#include <raimd/md_dict.h>

using namespace rai;
using namespace md;

const char *
MktfdMsg::get_proto_string( void ) noexcept
{
  return "MARKETFEED";
}

uint32_t
MktfdMsg::get_type_id( void ) noexcept
{
  return MARKETFEED_TYPE_ID;
}

static MDMatch mktfd_match = {
  .off         = 0,
  .len         = 1, /* cnt of buf[] */
  .hint_size   = 1, /* cnt of hint[] */
  .ftype       = (uint8_t) MARKETFEED_TYPE_ID,
  .buf         = { 0x1c },
  .hint        = { MARKETFEED_TYPE_ID },
  .is_msg_type = MktfdMsg::is_marketfeed,
  .unpack      = (md_msg_unpack_f) MktfdMsg::unpack
};

namespace rai {
namespace md {

enum {
  CSI1 = 0x5bU, /* '[' control sequence inducer part */
  HPA  = 0x60U, /* '`' -- the horizontal position adjust char */
  REP  = 0x62U, /* 'b' -- repitition char */
  ESC  = 0x1bU, /* <ESC> <CSI1> n ` -- for partials */
  FS_  = 0x1cU, /* <FS>..msg..<FS> */
  GS_  = 0x1dU, /* <GS>ric */
  RS_  = 0x1eU, /* <RS>fid<US>val */
  US_  = 0x1fU,
  CSI2 = 0x1bU | 0x80U /* <CSI2> n ` -- second form of partials */
};

}
}

void
MktfdMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( mktfd_match );
}

bool
MktfdMsg::is_marketfeed( void *bb,  size_t off,  size_t end,
                         uint32_t ) noexcept
{
  if ( off + 5 < end ) {
    const uint8_t *buf = (uint8_t *) bb;
    /* look for <FS>num<US>*/
    if ( buf[ end - 1 ] == FS_ && buf[ off ] == FS_ ) {
      size_t i = off + 1;
      uint8_t d = buf[ i++ ] - '0';
      if ( d <= 9 ) {
        do {
          d = buf[ i++ ] - '0';
        } while ( d <= 9 );
        if ( buf[ i - 1 ] == US_ )
          return true;
      }
    }
  }
  return false;
}

static inline bool
getdigit( uint8_t c,  uint8_t &d )
{
  return (d = (c - '0')) <= 9;
}

static void
scan_short( const uint8_t *buf,  size_t &i,  uint16_t &ival )
{
  uint8_t d;
  ival = 0;
  for ( i++; getdigit( buf[ i ], d ); i++ )
    ival = ( ival * 10 ) + d;
}

static void
scan_int( const uint8_t *buf,  size_t &i,  int32_t &ival )
{
  bool neg;
  uint8_t d;
  ival = 0;
  if ( buf[ ++i ] == '-' ) {
    neg = true;
    i++;
  }
  else {
    neg = false;
  }
  for ( ; getdigit( buf[ i ], d ); i++ )
    ival = ( ival * 10 ) + d;
  if ( neg )
    ival = -ival;
}

static inline uint8_t
scan_sep( const uint8_t *buf,  size_t &i )
{
  for ( ; ; i++ ) {
    if ( buf[ i ] >= FS_ && buf[ i ] <= US_ )
      return buf[ i ];
  }
}

static inline uint8_t
scan_esc_sep( const uint8_t *buf,  size_t &i,  uint16_t &repeat )
{
  for ( ; ; i++ ) {
    if ( ( buf[ i ] & 0x7fU ) == ESC ) {
      uint8_t c = buf[ i ];
      if ( repeat == 0 ) {
        size_t off = 0;
        if ( buf[ i ] == CSI2 ) { /* 0x80 | ESC n b */
          scan_short( &buf[ i ], off, repeat );
        }
        else if ( buf[ i + 1 ] == CSI1 ) { /* ESC [ n b */
          scan_short( &buf[ i + 1 ], off, repeat );
          off++;
        }
        if ( buf[ i + off ] == REP )
          i += off + 1;
        else
          repeat = 0;
      }
      return c;
    }
    if ( buf[ i ] == RS_ || buf[ i ] == FS_ )
      return buf[ i ];
  }
}

static inline uint8_t
scan_field_sep( const uint8_t *buf,  size_t &i )
{
  for ( ; ; i++ ) {
    if ( buf[ i ] == RS_ || buf[ i ] == FS_ )
      return buf[ i ];
  }
}

enum {
  NO_FUNC = 1,
  NO_RIC  = 2
};

int
MktfdMsg::parse_header( void ) noexcept
{
  uint8_t * buf = (uint8_t *) this->msg_buf;
  size_t    i   = this->msg_off;
  uint8_t   c;
  /* <FS:1c>func<US:1f>tag<GS:1d>ric[<RS:1e>rstatus][<US:1f>flist][<US:1f>rtl]
     <RS:1e>fid<US:1f>val<RS:1e> ... <FS:1c> */
  if ( scan_sep( buf, i ) != FS_ )
    return NO_FUNC;
  scan_short( buf, i, this->func );  /* FS:1c func */
  /* find the tag */
  c = scan_sep( buf, i );
  if ( c == US_ ) {
    this->taglen = ++i;
    this->tagp   = &buf[ this->taglen ];   /* US:1f tag */
    c = scan_sep( buf, i );
    this->taglen = i - this->taglen;
  }
  /* find the ric, must have this */
  if ( c != GS_ )
    return NO_RIC;
  this->riclen = ++i;
  this->ricp   = &buf[ this->riclen ];   /* GS:1d ric */
  c = scan_sep( buf, i );
  this->riclen = i - this->riclen;

  if ( c == RS_ && ( this->func == 340 || this->func == 342 ||
                    this->func == 318 ) ) {
    /* if func == 318, this is 3 = full verify, 4 = partial verify */
    scan_int( buf, i, this->rstatus );      /* RS:1e rstatus */
    c = scan_sep( buf, i );
  }
  /* initial(340)/snap(342)/verify(318) have flist */
  if ( c == US_ && ( this->func == 340 || this->func == 342 ||
                     this->func == 318 ) ) {
    scan_int( buf, i, this->flist );        /* US:1f flist */
    c = scan_sep( buf, i );                 /* can be US:1f delta-rtl */
  }                                        /* when aggregate update (350) */
  /* aggregate_update(350) */
  if ( c == US_ && this->func == 350 ) {
    int32_t delta_rtl;
    scan_int( buf, i, delta_rtl );          /* US:1f delta-rtl */
    c = scan_sep( buf, i );
  }
  if ( c == US_ ) {
    scan_int( buf, i, this->rtl );          /* US:1f rtl */
    c = scan_sep( buf, i );
  }
  if ( c == GS_ ) {
    scan_int( buf, i, this->status );       /* GS:1d status */
    c = scan_sep( buf, i );

    if ( c == RS_ ) {
      this->textlen = ++i;
      this->textp   = &buf[ this->taglen ];   /* RS:1e text */
      c = scan_sep( buf, i );
      this->textlen = i - this->textlen;
    }
  }
  if ( c != FS_ && c != RS_ ) { /* ignore the rest of header */
    for (;;) {
      c = scan_sep( buf, i );
      if ( c == FS_ || c == RS_ )
        break;
      i++;
    }
  }
  this->data_start = i;
  this->data_end   = this->msg_end;
  if ( this->data_start < this->data_end )
    this->data_end--; /* stop before FS */
  return 0;
}

MktfdMsg *
MktfdMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t,
                  MDDict *d,  MDMsgMem *m ) noexcept
{
  if ( off + 2 > end ||
       ((uint8_t *) bb)[ off ] != FS_ ||
       ((uint8_t *) bb)[ end - 1 ] != FS_ )
    return NULL;

  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;

  void * ptr;
  m->alloc( sizeof( MktfdMsg ), &ptr );
  for ( ; d != NULL; d = d->next )
    if ( d->dict_type[ 0 ] == 'a' ) /* need app_a type */
      break;
  MktfdMsg *msg = new ( ptr ) MktfdMsg( bb, off, end, d, m );
  if ( msg->parse_header() != 0 )
    return NULL;
  return msg;
}

int
MktfdMsg::get_field_iter( MDFieldIter *&iter ) noexcept
{
  void * ptr;
  this->mem->alloc( sizeof( MktfdFieldIter ), &ptr );
  iter = new ( ptr ) MktfdFieldIter( *this );
  return 0;
}

inline void
MktfdFieldIter::lookup_fid( void ) noexcept
{
  if ( this->ftype == MD_NODATA ) {
    if ( this->iter_msg.dict != NULL )
      this->iter_msg.dict->lookup( this->fid, this->ftype, this->fsize,
                                   this->fnamelen, this->fname );
    if ( this->ftype == MD_NODATA ) { /* undefined fid or no dictionary */
      this->ftype    = MD_STRING;
      this->fname    = NULL;
      this->fnamelen = 0;
    }
  }
}

int
MktfdFieldIter::get_name( MDName &name ) noexcept
{
  this->lookup_fid();
  name.fid      = this->fid;
  name.fnamelen = this->fnamelen;
  name.fname    = this->fname;
  return 0;
}
#if 0
static inline bool
valid_integer( uint8_t *fptr,  size_t fsize )
{
  size_t i, digit = 0;

  if ( fptr[ 0 ] == '+' || fptr[ 0 ] == '-' )
    i = 1;
  else
    i = 0;
  for ( ; i < fsize; i++ ) {
    if ( fptr[ i ] < '0' || fptr[ i ] > '9' )
      return false;
    digit++;
  }
  return digit > 0;
}
static inline bool
valid_float( uint8_t *fptr,  size_t fsize )
{
  size_t i, dot = 0, slash = 0, space = 0, digit = 0;

  if ( fptr[ 0 ] == '+' || fptr[ 0 ] == '-' )
    i = 1;
  else
    i = 0;
  for ( ; i < fsize; i++ ) {
    switch ( fptr[ i ] ) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
                digit++; break;
      case '.': dot++; break;
      case '/': slash++; break;
      case ' ': space++; break;
      default: return false;
    }
  }
  return ( dot | slash | space ) <= 1 && digit > 0;
}
#endif
static bool
encoded_integer( uint8_t *fptr,  size_t fsize,  uint64_t &ival )
{
  ival = 0;
  for ( unsigned int i = 0; i < fsize; i++ ) {
    if ( fptr[ i ] < 0x40 || fptr[ i ] >= 0x80 ) {
      if ( fptr[ i ] == 0 )
        return true;
      return false;
    }
    ival += ( fptr[ i ] & 0x3f ) << ( 6 * i );
  }
  return true;
}

int
MktfdFieldIter::get_hint_reference( MDReference &mref ) noexcept
{
  mref.zero();
  return Err::NOT_FOUND;
}

int
MktfdFieldIter::get_enum( MDReference &mref,  MDEnum &enu ) noexcept
{
  if ( mref.ftype == MD_ENUM ) {
    if ( this->iter_msg.dict != NULL ) {
      enu.val = get_uint<uint16_t>( mref );
      if ( this->iter_msg.dict->get_enum_text( this->fid, enu.val,
                                               enu.disp, enu.disp_len ) )
        return 0;
    }
  }
  enu.zero();
  return Err::NO_ENUM;
}

int
MktfdFieldIter::get_reference( MDReference &mref ) noexcept
{
  uint8_t * buf = &((uint8_t *) this->iter_msg.msg_buf)[ this->data_off ];
  char * eos;
  mref.fendian  = md_endian;
  mref.fentrysz = 0;
  mref.fentrytp = MD_NODATA;
  this->lookup_fid(); /* determine type from dictionary, if loaded */
  mref.ftype = this->ftype;
  mref.fsize = this->field_end - this->data_off; /* length of data in chars */
  for (;;) {
    switch ( mref.ftype ) { /* type from dict */
      default:
      case MD_NODATA:
        mref.fsize = 0;
        mref.fptr  = NULL;
        return 0;

      case MD_STRING:  /* default type when no dictionary */
        mref.fptr  = buf;
        return 0;

      case MD_PARTIAL: /* could have escape sequences for partial update */
        if ( mref.fsize > 3 ) {
          if ( ( buf[ 0 ] & 0x7fU ) == ESC ) {
            size_t off = 0;
            if ( buf[ 0 ] == CSI2 ) { /* 0x80 | ESC n ` */
              scan_short( buf, off, this->position );
            }
            else if ( buf[ 1 ] == CSI1 ) { /* ESC [ n ` */
              scan_short( &buf[ 1 ], off, this->position );
              off++;
            }
            if ( off > 0 && buf[ off++ ] == HPA ) {
              mref.fptr     = &buf[ off ];
              mref.fsize   -= off;
              mref.ftype    = MD_PARTIAL;
              mref.fentrysz = this->position;

              if ( this->repeat > 0 ) {
                for ( off = mref.fsize; off > 0; )
                  if ( ( mref.fptr[ --off ] & 0x7fU ) == ESC )
                    break;
                if ( off > 0 &&
                     off + this->repeat <= sizeof( this->rep_buf ) ) {
                  ::memcpy( this->rep_buf, mref.fptr, off );
                  ::memset( &this->rep_buf[ off ],
                            this->rep_buf[ off - 1 ], this->repeat );
                  mref.fsize = off + this->repeat;
                  mref.fptr  = this->rep_buf;
                }
              }
              return 0;
            }
          }
        }
        else if ( mref.fsize == 0 ) {
          static uint8_t nullchar[ 1 ] = { 0 };
          mref.fsize = 1;
          mref.fptr  = nullchar;
          return 0;
        }
        mref.fptr = buf;
        return 0;

      case MD_TIME: /* time field hh:mm */
        if ( this->time.parse( (char *) buf, mref.fsize ) == 0 ) {
          mref.fptr  = (uint8_t *) (void *) &this->time;
          mref.fsize = sizeof( this->time );
          return 0;
        }
        break;
      case MD_DATE: /* date field dd MMM yyyy */
        if ( this->date.parse( (char *) buf, mref.fsize ) == 0 ) {
          mref.fptr  = (uint8_t *) (void *) &this->date;
          mref.fsize = sizeof( this->date );
          return 0;
        }
        break;
      case MD_OPAQUE: /* binary fields can encode an integer (REG_ID1) */
        if ( this->fsize < 7 &&
             encoded_integer( buf, mref.fsize, this->val.u64 ) ) {
          mref.ftype = MD_UINT;
          if ( this->fsize <= 5 )
            mref.fsize = 2;
          else
            mref.fsize = 4;
          /* XXX little endian, no need to convert union with u16 / u32 */
          mref.fptr = (uint8_t *) (void *) &this->val;
        }
        else {
          mref.fptr = buf;
        }
        return 0;

      case MD_INT: /* MSG_TYPE and SASS header fields use ints */
        this->val.i64 = parse_int<int64_t>( (char *) buf, &eos );
        if ( eos > (char *) buf ) {
          mref.fsize = this->fsize;
          mref.fptr  = (uint8_t *) (void *) &this->val;
          return 0;
        }
        break;

      case MD_ENUM: /* field defined in enumtype.def */
        mref.fptr = NULL;
        if ( mref.fsize > 0 && buf[ 0 ] >= '0' && buf[ 0 ] <= '9' ) {
          char *eos;
          this->val.u64 = parse_uint<uint64_t>( (char *) buf, &eos );
          if ( (size_t) ( eos - (char *) buf ) == mref.fsize ) {
            mref.fsize = 2;
            mref.fptr  = (uint8_t *) (void *) &this->val;
          }
        }
        if ( mref.fptr == NULL ) {
          mref.ftype = MD_STRING;
          mref.fptr  = buf;
        }
        return 0;

      case MD_UINT: /* PROD_PERM, RECORDTYPE, RDNDISPLAY */
        this->val.u64 = parse_uint<uint64_t>( (char *) buf, &eos );
        if ( eos > (char *) buf ) {
          mref.fsize = this->fsize;
          mref.fptr  = (uint8_t *) (void *) &this->val;
          return 0;
        }
        if ( encoded_integer( buf, mref.fsize, this->val.u64 ) ) {
          mref.ftype = MD_UINT;
          if ( mref.fsize <= 5 )
            mref.fsize = 2;
          else
            mref.fsize = 4;
          mref.fptr = (uint8_t *) (void *) &this->val;
          return 0;
        }
        break;

      case MD_DECIMAL: /* most all prices, volumes and sizes */
        if ( this->dec.parse( (char *) buf, mref.fsize ) == 0 ) {
          mref.fsize = sizeof( this->dec );
          mref.fptr  = (uint8_t *) (void *) &this->dec;
          return 0;
        }
        break;
    }
    if ( mref.ftype != MD_STRING ) /* if type failed to parse, use string */
      mref.ftype = MD_STRING;
    else
      mref.ftype = MD_NODATA; /* shouldn't occur */
  }
  return 0;
}

int
MktfdFieldIter::find( const char *name,  size_t name_len,
                      MDReference &mref ) noexcept
{
  if ( this->iter_msg.dict == NULL )
    return Err::NO_DICTIONARY;

  int status = Err::NOT_FOUND;
  if ( name_len > 0 ) {
    MDFid    fid;
    MDType   ftype;
    uint32_t fsize;
    if ( this->iter_msg.dict->get( name, name_len, fid, ftype, fsize ) ) {
      if ( (status = this->first()) == 0 ) {
        do {
          if ( this->fid == fid )
            return this->get_reference( mref );
        } while ( (status = this->next()) == 0 );
      }
    }
  }
  return status;
}

int
MktfdFieldIter::first( void ) noexcept
{
  this->field_start = ((MktfdMsg &) this->iter_msg).data_start;
  this->field_end   = ((MktfdMsg &) this->iter_msg).data_end;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  return this->unpack();
}

int
MktfdFieldIter::next( void ) noexcept
{
  this->field_start = this->field_end;
  this->field_end   = ((MktfdMsg &) this->iter_msg).data_end;
  if ( this->field_start >= this->field_end )
    return Err::NOT_FOUND;
  return this->unpack();
}

int
MktfdFieldIter::unpack( void ) noexcept
{
  uint8_t * buf = (uint8_t *) this->iter_msg.msg_buf;
  size_t    i   = this->field_start;
  uint8_t   c;
  
  this->repeat = 0;
  c = scan_esc_sep( buf, i, this->repeat );
  if ( c == RS_ ) {
    this->ftype = MD_NODATA;
    scan_int( buf, i, this->fid );
    c = scan_sep( buf, i );
    if ( c == US_ ) {
      this->data_off = ++i;
      if ( ( buf[ this->data_off ] & 0x7fU ) == ESC ) {
        i++;
        scan_esc_sep( buf, i, this->repeat );
      }
      else {
        scan_field_sep( buf, i );
      }
      this->field_end = i;
      return 0;
    }
  }
  else if ( ( c & 0x7fU ) == ESC ) {
    this->data_off = i++;
    scan_esc_sep( buf, i, this->repeat );
    this->field_end = i;
    /*this->savref.fptr = NULL;*/
    return 0;
  }
  return Err::BAD_FIELD;
}

