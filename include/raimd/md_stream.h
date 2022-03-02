#ifndef __rai_raimd__md_stream_h__
#define __rai_raimd__md_stream_h__

#include <raimd/md_list.h>

namespace rai {
namespace md {

enum StreamStatus {
  STRM_OK        = LIST_OK,
  STRM_NOT_FOUND = LIST_NOT_FOUND,
  STRM_FULL      = LIST_FULL,
  STRM_BAD_ID    = 3
};

struct StreamGroupQuery {
  size_t first,
         count,
         pos,
       * idx;
  void zero( void ) {
    this->first = 0;
    this->count = 0;
    this->pos   = 0;
    this->idx   = NULL;
  }
};

static inline const char *
md_rev_get_uint( const char *p,  const char *start,  uint64_t &v )
{
  #define U( f ) (uint64_t) f
  static const uint64_t tens[] = {
    1, 10, 100, 1000, U( 1e4 ), U( 1e5 ), U( 1e6 ), U( 1e7 ),
    U( 1e8 ), U( 1e9 ), U( 1e10 ), U( 1e11 ), U( 1e12 ), U( 1e13 ),
    U( 1e14 ), U( 1e15 ), U( 1e16 ), U( 1e17 ), U( 1e18 )
  };
  #undef U
  uint64_t val = 0, val2;
  size_t n = 0;
  v = 0;
  for (;;) {
    if ( --p < start || p[ 0 ] < '0' || p[ 0 ] > '9' ) {
      v = val;
      if ( n == 0 )
        return NULL;
      return p;
    }
    if ( n < 18 )
      val += tens[ n++ ] * (uint64_t) ( p[ 0 ] - '0' );
    else {
      val2 = (uint64_t) ( p[ 0 ] - '0' );
      n = 1;
      for (;;) {
        if ( --p < start || p[ 0 ] < '0' || p[ 0 ] > '9' ) {
          if ( val2 > 18 || ( val2 == 18 && val > 446744073709551615ULL ) )
            return NULL;
          v = val + val2 * tens[ 18 ];
          return p;
        }
        if ( n >= 2 )
          return NULL;
        val2 += tens[ n++ ] * (uint64_t) ( p[ 0 ] - '0' );
      }
    }
  }
}

static inline size_t
md_uint_digits( uint64_t v )
{
  for ( size_t n = 1; ; n += 4 ) {
    if ( v < 10 )    return n;
    if ( v < 100 )   return n + 1;
    if ( v < 1000 )  return n + 2;
    if ( v < 10000 ) return n + 3;
    v /= 10000;
  }
}

static inline size_t
md_uint_to_str( uint64_t v,  char *buf,  size_t len ) {
  for ( size_t pos = len; v >= 10; ) {
    const uint64_t q = v / 10, r = v % 10;
    buf[ --pos ] = '0' + (char) r;
    v = q;
  }
  buf[ 0 ] = '0' + (char) v;
  return len;
}

struct StreamId {
  const char * id;     /* ths string id */
  size_t       idlen;  /* length of id[] */
  uint64_t     x, y;   /* x = first part, utc millisecs, y = serial number */
  StreamId() : id( 0 ), idlen( 0 ), x( 0 ), y( 0 ) {}

  bool str_to_id( const char *id,  size_t idlen ) {
    const char * p;
    uint64_t     val;

    this->id    = id;
    this->idlen = idlen;
    this->x     = 0;
    this->y     = 0;
    p = md_rev_get_uint( &id[ idlen ], id, val );
    if ( p == NULL )
      return false;
    if ( p < id ) { /* if only one number, use zero as serial */
      this->x = val;
      return true;
    }
    if ( p[ 0 ] == '-' && p > id ) {
      this->y = val;
      p = md_rev_get_uint( p, id, val );
      if ( p == NULL )
        return false;
      if ( p < id ) { /* two numbers */
        this->x = val;
        return true;
      }
    }
    return false;
  }

  void xy_to_id( uint64_t ms,  uint64_t ser,  MDMsgMem &tmp ) {
    size_t ms_digs  = ( ms >= (uint64_t) 1e12 && /* ex: 1574538280439 */
                        ms <  (uint64_t) 1e13 ) ? 13 : md_uint_digits( ms ),
           ser_digs = md_uint_digits( ser );
    char * idval;
    tmp.alloc( ms_digs + ser_digs + 2, &idval );
    this->id    = idval;
    this->idlen = ms_digs + ser_digs + 1;
    this->x     = ms;
    this->y     = ser;
    md_uint_to_str( ms, idval, ms_digs );
    idval[ ms_digs ] = '-';
    md_uint_to_str( ser, &idval[ ms_digs + 1 ], ser_digs );
    idval[ ms_digs + 1 + ser_digs ] = '\0';
  }

  bool str_to_id( ListVal &lv,  MDMsgMem &tmp ) {
    if ( lv.sz2 == 0 )
      return this->str_to_id( (const char *) lv.data, lv.sz );
    char * id;
    size_t sz = lv.dup( tmp, &id );
    return this->str_to_id( id, sz );
  }
  /* return 0 if eq, -1 if this > val, 1 if this < val */
  int compare( const StreamId &val ) const {
    if ( this->x == val.x )
      return ( this->y == val.y ) ? 0 :
             ( this->y < val.y ) ? -1 : 1;
    return ( this->x < val.x ) ? -1 : 1;
  }
  ListData *construct_entry( size_t idxsz,  size_t sz,  MDMsgMem &tmp ) {
    idxsz += 1;
    sz    += this->idlen;
    size_t asz = ListData::alloc_size( idxsz, sz );
    void * mem;
    tmp.alloc( sizeof( ListData ) + asz, &mem );
    ::memset( mem, 0, sizeof( ListData ) + asz );
    void     * p  = &((char *) mem)[ sizeof( ListData ) ];
    ListData * xl = new ( mem ) ListData( p, asz );
    xl->init( idxsz, sz );
    xl->rpush( this->id, this->idlen );
    return xl;
  }
};

/* XCLAIM args:  */
struct StreamClaim {
  uint64_t ns;         /* current time nanos */
  int64_t  min_idle,   /* XCLAIM key group consumer min-idle-time id */
           idle,       /* [IDLE ms] */
           time,       /* [TIME ms-unix-time] */
           retrycount; /* [RETRYCOUNT count] */
  bool     force,      /* [FOURCE] */
           justid;     /* [JUSTID] */
  StreamClaim() { this->zero(); }

  void zero( void ) {
    ::memset( this, 0, sizeof( *this ) );
  }
};

/* Stream tupple:  group, consumer, identifier */
struct StreamArgs {
  const char * gname,    /* group name */
             * cname,    /* consumer name */
             * idval;    /* identifier for message, utc-millsecs-serial */
  size_t       glen,     /* length of gname */
               clen,     /* length of cname */
               idlen,    /* length of idval */
               alloc_sz; /* size of mem allocated for pending record */
  void       * mem;      /* mem reused for pending, group */
  uint64_t     ns;       /* time claimed by client */
  uint32_t     cnt;      /* number of times claimed */
  bool         is_id_next; /* idval == '>' */
  StreamArgs() { this->zero(); }

  void zero( void ) {
    ::memset( this, 0, sizeof( *this ) );
  }

  void reuse( MDMsgMem &tmp ) {
    this->mem      = NULL;
    this->alloc_sz = 0;
    tmp.reuse();
  }

  ListData *construct_group( MDMsgMem &tmp ) {
    size_t idxsz = 2,
           sz    = this->glen + this->idlen,
           asz   = ListData::alloc_size( idxsz, sz );
    if ( asz > this->alloc_sz ) {
      tmp.alloc( sizeof( ListData ) + asz, &this->mem );
      this->alloc_sz = asz;
    }
    ::memset( this->mem, 0, sizeof( ListData ) + asz );
    void     * p  = &((char *) this->mem)[ sizeof( ListData ) ];
    ListData * xl = new ( this->mem ) ListData( p, asz );
    xl->init( idxsz, sz );
    xl->rpush( this->gname, this->glen );
    xl->rpush( this->idval, this->idlen );
    return xl;
  }

  ListData *construct_pending( MDMsgMem &tmp ) {
    /*if ( this->ns == 0 )
      this->ns = kv::current_realtime_coarse_ns();*/
    if ( this->cnt == 0 )
      this->cnt = 1;
    size_t idxsz = 5,
           sz    = this->glen + this->clen + this->idlen + 8 + 4,
           asz   = ListData::alloc_size( idxsz, sz );
    if ( asz > this->alloc_sz ) {
      tmp.alloc( sizeof( ListData ) + asz, &this->mem );
      this->alloc_sz = asz;
    }
    ::memset( this->mem, 0, sizeof( ListData ) + asz );
    void     * p  = &((char *) this->mem)[ sizeof( ListData ) ];
    ListData * xl = new ( this->mem ) ListData( p, asz );
    xl->init( idxsz, sz );
    xl->rpush( this->idval, this->idlen );
    xl->rpush( this->gname, this->glen );
    xl->rpush( this->cname, this->clen );
    xl->rpush( &this->ns, 8 );
    xl->rpush( &this->cnt, 4 );
    return xl;
  }

  ListData &list_data( void ) {
    return *(ListData *) this->mem;
  }
};

/* index of pending elements: { id, group, consumer, idle-ns, deliver-count } */
static const size_t P_ID  = 0, /* identifier: 1576665850070-0 */
                    P_GRP = 1, /* group name */
                    P_CON = 2, /* consumer name */
                    P_NS  = 3, /* uint64 representing nanos-UTC */
                    P_CNT = 4; /* uint32 delivery counter  */
/* Stream message, group, pending lists */
struct StreamData {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  /* These are lists of lists */
  ListData stream,  /* List of { id, field pairs }, ordered by id */
           group,   /* List of { gname, last-id-sent } */
           pending; /* List of { id, gname, cname, ns, count } */
  static const uint16_t str8_sig  = 0xf7e9U;
  static const uint32_t str16_sig = 0xddbe7ae9UL;
  static const uint64_t str32_sig = 0xa5f5ff85c9f6c3e9ULL;

  StreamData() {}
  StreamData( void *p,  size_t sz,
              void *p2, size_t sz2,
              void *p3, size_t sz3 )
    : stream( p, sz ), group( p2, sz2 ), pending( p3, sz3 ) {}

  void init( size_t count,  size_t data_len,
             size_t group_count,  size_t group_len,
             size_t pend_count,  size_t pend_len ) {
    this->stream.init_sig( count, data_len, str8_sig, str16_sig, str32_sig );
    this->group.init_sig( group_count, group_len,
                          str8_sig, str16_sig, str32_sig );
    this->pending.init_sig( pend_count, pend_len,
                            str8_sig, str16_sig, str32_sig );
  }
  void open( const void *oob = NULL,  size_t loob = 0 ) {
    this->stream.open( oob, loob );
    this->group.open( NULL, 0 );
    this->pending.open( NULL, 0 );
  }
  void copy( StreamData &x ) {
    this->stream.copy( x.stream );
    this->group.copy( x.group );
    this->pending.copy( x.pending );
  }
  StreamStatus add( ListData &lst ) {
    return (StreamStatus) this->stream.rpush( lst.listp, lst.size );
  }
  /* return the list at position */
  static StreamStatus sindex( ListData &lst,  size_t pos,  ListData &ld,
                              MDMsgMem &tmp ) {
    ListVal    lv;
    ListStatus lstat;
    if ( (lstat = lst.lindex( pos, lv )) == LIST_OK ) {
      lv.unite( tmp );
      new ( &ld ) ListData( (void *) lv.data, lv.sz );
      ld.open( (void *) lv.data, lv.sz );
      return STRM_OK;
    }
    return (StreamStatus) lstat;
  }
  /* return the first element of the list at pos */
  StreamStatus sindex_id( ListData &lst,  size_t pos,  ListVal &lv,
                          MDMsgMem &tmp ) {
    ListData     ld;
    ListStatus   lstat;
    StreamStatus xstat;
    if ( (xstat = this->sindex( lst, pos, ld, tmp )) == STRM_OK ) {
      if ( (lstat = ld.lindex( 0, lv )) == LIST_OK )
        return STRM_OK;
      return (StreamStatus) lstat;
    }
    return xstat;
  }
  /* return the id of the last element in the list */
  StreamStatus sindex_id_last( ListData &lst,  const char *&id,  size_t &idlen,
                               MDMsgMem &tmp ) {
    size_t       cnt = lst.count();
    ListVal      lv;
    StreamStatus xstat = STRM_OK;
    if ( cnt == 0 ) {
      idlen = 1;
      id    = "0";
    }
    else {
      if ( (xstat = this->sindex_id( lst, cnt - 1, lv, tmp )) == STRM_OK )
        idlen = lv.dup( tmp, &id );
    }
    return xstat;
  }
  /* bsearch find id in an ordered list, gt=true is the end of the list */
  ssize_t bsearch_str( ListData &lst,  const char *id,  size_t idlen,
                         bool gt,  MDMsgMem &tmp ) {
    size_t cnt = lst.count();
    if ( cnt == 0 )
      return 0;
    if ( idlen == 1 ) {
      if ( id[ 0 ] == '-' )
        return 0 + ( gt ? 1 : 0 );
      if ( id[ 0 ] == '+' || id[ 0 ] == '$' || id[ 0 ] == '>' )
        return cnt - ( gt ? 0 : 1 );
    }
    StreamId srch, val;
    ListVal  lv;
    size_t   size = cnt,
             piv,
             pos = 0;
    /* search pos=0 -> count - pos */
    if ( ! srch.str_to_id( id, idlen ) )
      return -1;
    for (;;) {
      if ( size == 0 )
        return pos;
      piv = size / 2;
      if ( this->sindex_id( lst, pos + piv, lv, tmp ) != STRM_OK )
        return -1;
      if ( ! val.str_to_id( lv, tmp ) )
        return -1;
      switch ( srch.compare( val ) ) {
        case 0:  return pos + piv + ( gt ? 1 : 0 );
        case 1:  size -= piv + 1; pos += piv + 1; break;
        case -1: size = piv; break;
      }
    }
  }
  /* find elem eq srch */
  ssize_t bsearch_eq( ListData &lst,  StreamId &srch,  MDMsgMem &tmp,
                      size_t size ) {
    ListVal  lv;
    StreamId val;
    size_t   piv,
             pos = 0,
             osz;
    for (;;) {
      piv = size / 2;
      osz = size;
      if ( this->sindex_id( lst, pos + piv, lv, tmp ) != STRM_OK )
        return -1;
      if ( ! val.str_to_id( lv, tmp ) )
        return -1;
      switch ( srch.compare( val ) ) {
        case 0: return pos + piv;
        case 1: size -= piv + 1; pos += piv + 1; break;
        case -1: size = piv; break;
      }
      if ( osz == 0 )
        return -1;
    }
  }
  /* same, with - and + edges */
  ssize_t bsearch_eq( ListData &lst,  const char *id,  size_t idlen,
                      MDMsgMem &tmp ) {
    size_t size = lst.count();
    if ( size == 0 )
      return -1;
    if ( idlen == 1 ) {
      if ( id[ 0 ] == '-' )
        return 0;
      if ( id[ 0 ] == '+' )
        return size - 1;
    }
    StreamId srch;
    if ( ! srch.str_to_id( id, idlen ) )
      return -1;
    return this->bsearch_eq( lst, srch, tmp, size );
  }
  /* linear scan the list for id */
  ssize_t scan( ListData &lst,  const char *id,  size_t idlen,  ListData &ld,
                MDMsgMem &tmp ) {
    ListVal lv;
    size_t  count = lst.count();
    for ( size_t pos = 0; pos < count; pos++ ) {
      if ( this->sindex( lst, pos, ld, tmp ) != STRM_OK )
        return -1;
      if ( ld.lindex( 0, lv ) != LIST_OK )
        return -1;
      if ( lv.cmp_key( id, idlen ) == 0 )
        return pos;
    }
    return -1;
  }
  /* vheck if group exists */
  bool group_exists( StreamArgs &sa,  MDMsgMem &tmp ) {
    ListData ld;
    return this->scan( this->group, sa.gname, sa.glen, ld, tmp ) != -1;
  }
  /* update or add a group with last-sent-id = id */
  StreamStatus update_group( StreamArgs &sa,  MDMsgMem &tmp ) {
    ListData * xl, ld;
    ssize_t    off;

    off = this->scan( this->group, sa.gname, sa.glen, ld, tmp );
    /* if it exists, update id */
    if ( off != -1 ) {
      /* if group data is contained by group, not split and reallocated */
      if ( this->group.in_mem_region( ld.listp, ld.size ) &&
           ld.lset( 1, sa.idval, sa.idlen ) == LIST_OK ) /* update id */
        return STRM_OK;
      this->group.lrem( off ); /* reappend group */
    }
    /* add a new group */
    xl = sa.construct_group( tmp );
    /* add group data */
    return (StreamStatus) this->group.rpush( xl->listp, xl->size );
  }
  /* remove group if it exists */
  StreamStatus remove_group( StreamArgs &sa,  MDMsgMem &tmp ) {
    ListData ld;
    ssize_t  off = this->scan( this->group, sa.gname, sa.glen, ld, tmp );
    if ( off != -1 ) {
      this->group.lrem( off );
      this->remove_pending( sa, tmp );
      sa.cnt = 1;
    }
    else {
      sa.cnt = 0;
    }
    return STRM_OK;
  }
  /* locate pending elem by gname, cname and remove it */
  StreamStatus remove_pending( StreamArgs &sa,  MDMsgMem &tmp ) {
    ListData     ld;
    size_t       pos,
                 count = this->pending.count();
    sa.cnt = 0;
    for ( pos = 0; pos < count; ) {
      StreamStatus xstat = this->sindex( this->pending, pos, ld, tmp );
      if ( xstat != STRM_OK )
        return xstat;
      /* if group and consumer match */
      if ( ( sa.clen == 0 ||
             ld.lindex_cmp_key( P_CON, sa.cname, sa.clen ) == LIST_OK ) &&
            ld.lindex_cmp_key( P_GRP, sa.gname, sa.glen ) == LIST_OK ) {
        this->pending.lrem( pos );
        count--;
        sa.cnt++;
      }
      else {
        pos++;
      }
    }
    return STRM_OK;
  }
  /* locate pending elem by gname, id and remove it */
  StreamStatus ack( StreamArgs &sa,  MDMsgMem &tmp ) {
    ListData ld;
    size_t   pos,
             count = this->pending.count();

    for ( pos = 0; pos < count; pos++ ) {
      StreamStatus xstat = this->sindex( this->pending, pos, ld, tmp );
      if ( xstat != STRM_OK )
        return xstat;
      /* match id and group-name */
      if ( ld.lindex_cmp_key( P_ID, sa.idval, sa.idlen ) == LIST_OK &&
           ld.lindex_cmp_key( P_GRP, sa.gname, sa.glen ) == LIST_OK ) {
        this->pending.lrem( pos );
        return STRM_OK;
      }
    }
    return STRM_NOT_FOUND;
  }
  /* locate pending elem by gname, id and rename it */
  StreamStatus claim( StreamArgs &sa,  StreamClaim &cl,  MDMsgMem &tmp ) {
    ListData ld;
    size_t   pos,
             count = this->pending.count();

    for ( pos = 0; pos < count; pos++ ) {
      StreamStatus xstat = this->sindex( this->pending, pos, ld, tmp );
      if ( xstat != STRM_OK )
        return xstat;
      /* match id and group-name */
      if ( ld.lindex_cmp_key( P_ID, sa.idval, sa.idlen ) == LIST_OK &&
           ld.lindex_cmp_key( P_GRP, sa.gname, sa.glen ) == LIST_OK )
        break;
    }
    if ( pos == count )
      return STRM_NOT_FOUND;

    ListVal    lv;
    ListData * xl;
    uint64_t   ns,
               cur;
    ListStatus lstat;

    lstat = ld.lindex( P_NS, lv ); /* ns */
    if ( lstat != LIST_OK )
      return (StreamStatus) lstat;
    ns  = lv.u64();
    cur = cl.ns;
    if ( cl.min_idle != 0 ) {
      if ( cl.min_idle * 1000000 + ns >= cur )
        return STRM_NOT_FOUND;
    }
    lstat = ld.lindex( P_CNT, lv ); /* cnt */
    if ( lstat != LIST_OK )
      return (StreamStatus) lstat;
    if ( cl.time != 0 ) /* set delivery time */
      sa.ns = cl.time * 1000000;
    else if ( cl.idle != 0 )
      sa.ns = cur - cl.idle * 100000;
    else
      sa.ns = cur;
    if ( cl.retrycount != 0 ) /* set delivery count */
      sa.cnt = (uint32_t) cl.retrycount;
    else /* no incrment on justid */
      sa.cnt = lv.u32() + ( cl.justid ? 0 : 1 );
    xl = sa.construct_pending( tmp );
    lstat = this->pending.lset( pos, xl->listp, xl->size );
    return (StreamStatus) lstat;
  }

  StreamStatus group_query( StreamArgs &sa,  size_t maxcnt,
                            StreamGroupQuery &grp,  MDMsgMem &tmp ) {
    size_t       strm_cnt = this->stream.count();
    ListData     ld;
    ListVal      lv;
    ssize_t      off;
    ListStatus   lstat;
    StreamStatus xstat;

    grp.zero();
    off = this->scan( this->group, sa.gname, sa.glen, ld, tmp );
    /* if it exists, update id */
    if ( off < 0 )
      return STRM_NOT_FOUND;
    grp.pos = off;

    /* get the latest ids after the last id consumed */
    if ( sa.is_id_next ) {
      char *s;
      lstat = ld.lindex( 1, lv ); /* the last id consumed */
      if ( lstat != LIST_OK )
        return (StreamStatus) lstat;

      sa.idlen = lv.dup( tmp, &s );
      sa.idval = s;
      off = this->bsearch_str( this->stream, sa.idval, sa.idlen, true, tmp );
      if ( off < 0 )
        return STRM_BAD_ID;
      grp.first = off;
      /* the number of records available up to maxcnt */
      if ( maxcnt == 0 || maxcnt + off > strm_cnt )
        maxcnt = strm_cnt - off;
      grp.count = maxcnt;
    }
    /* get the ids >= id belonging to the consumer */
    else {
      StreamId srch, val;
      size_t   pend_cnt = this->pending.count(),
               pos;
      if ( ! srch.str_to_id( sa.idval, sa.idlen ) )
        return STRM_BAD_ID;
      /* scan pending list, find the ids and index them to stream */
      for ( pos = 0; pos < pend_cnt; pos++ ) {
        xstat = this->sindex( this->pending, pos, ld, tmp );
        if ( xstat != STRM_OK )
          continue;
        /* match group consumer */
        if ( ld.lindex_cmp_key( P_CON, sa.cname, sa.clen ) != LIST_OK ||
             ld.lindex_cmp_key( P_GRP, sa.gname, sa.glen ) != LIST_OK )
          continue;
        if ( ld.lindex( P_ID, lv ) != LIST_OK )
          continue;
        /* if id from pending equal to search val */
        if ( val.str_to_id( lv, tmp ) && srch.compare( val ) <= 0 ) {
          /* find the record in the stream */
          off = this->bsearch_eq( this->stream, val, tmp, strm_cnt );
          if ( off >= 0 ) {
            if ( ( grp.count & 7 ) == 0 )
              tmp.extend( sizeof( grp.idx[ 0 ] ) * grp.count,
                          sizeof( grp.idx[ 0 ] ) * ( grp.count + 8 ),
                          &grp.idx );
            grp.idx[ grp.count++ ] = (size_t) off;
          }
        }
      }
    }
    return STRM_OK;
  }
};

struct StreamGeom {
  struct ListGeom {
    size_t count, data_len, asize;
    ListGeom() : count( 0 ), data_len( 0 ), asize( 0 ) {}

    void add( ListData *list,  size_t add_len,  size_t add_cnt ) {
      if ( add_len != 0 ) {
        if ( list != NULL ) {
          this->data_len  = add_len + list->data_len();
          this->data_len += this->data_len / 2 + 2;
          this->count     = add_cnt + list->count();
          this->count    += this->count / 2 + 2;
        }
        else {
          this->count    = add_cnt + 1;
          this->data_len = add_len + 1;
        }
        this->asize = ListData::alloc_size( this->count, this->data_len );
      }
      else {
        this->count    = list->index_size();
        this->data_len = list->data_size();
        this->asize    = list->size;
      }
    }
    void print( const char *nm ) {
      printf( "%s_asize %u, count %u, data_len %u\n", nm,
               (uint32_t) this->asize, (uint32_t) this->count,
               (uint32_t) this->data_len );
    }
  };
  ListGeom stream, group, pending;

  void add( StreamData *curr,  size_t add_str,  size_t cnt_str,  size_t add_grp,
            size_t cnt_grp, size_t add_pnd,  size_t cnt_pnd ) {
    if ( curr != NULL ) {
      this->stream.add( &curr->stream, add_str, cnt_str );
      this->group.add( &curr->group, add_grp, cnt_grp );
      this->pending.add( &curr->pending, add_pnd, cnt_pnd );
    }
    else {
      this->stream.add( NULL, add_str, cnt_str );
      this->group.add( NULL, add_grp, cnt_grp );
      this->pending.add( NULL, add_pnd, cnt_pnd );
    }
  }

  StreamData *make_new( void *mem,  void *data ) {
    void *p  = data,
         *p2 = &((char *) p)[ this->stream.asize ],
         *p3 = &((char *) p2)[ this->group.asize ];
    StreamData *newbe = new ( mem ) StreamData( p, this->stream.asize,
                                                p2, this->group.asize,
                                                p3, this->pending.asize );
    newbe->init( this->stream.count, this->stream.data_len,
                 this->group.count, this->group.data_len,
                 this->pending.count, this->pending.data_len );
    return newbe;
  }

  static bool
  get_stream_segments( const uint8_t *buf,  size_t len,
                       size_t &slen,  size_t &glen,  size_t &plen )
  {
    slen = glen = plen = 0;
    slen = ListData::mem_size( buf, len, StreamData::str8_sig,
                               StreamData::str16_sig, StreamData::str32_sig );
    if ( slen != 0 && slen < len ) {
      buf  = &buf[ slen ];
      len -= slen;
      glen = ListData::mem_size( buf, len, StreamData::str8_sig,
                                 StreamData::str16_sig, StreamData::str32_sig );
      if ( glen != 0 && glen < len ) {
        buf  = &buf[ glen ];
        len -= glen;
        plen = ListData::mem_size( buf, len, StreamData::str8_sig,
                                 StreamData::str16_sig, StreamData::str32_sig );
      }
    }
    return plen > 0;
  }

  static StreamData *make_existing( void *mem,  void *data,  size_t datalen ) {
    size_t slen, glen, plen;
    if ( ! get_stream_segments( (uint8_t *) data, datalen, slen, glen, plen ) )
      return NULL;
    void * p2 = &((uint8_t *) data)[ slen ],
         * p3 = &((uint8_t *) data)[ slen + glen ];
    return new ( mem ) StreamData( data, slen, p2, glen, p3, plen );
  }

  void print( void ) {
    this->stream.print( "stream" );
    this->group.print( "group" );
    this->pending.print( "pending" );
  }

  size_t asize( void ) {
    return this->stream.asize + this->group.asize + this->pending.asize;
  }
};

struct StreamMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }
  size_t slen, glen, plen;

  StreamMsg( void *bb,  size_t off,  size_t len,  MDDict *d,
             MDMsgMem *m ) noexcept;

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;

  static bool is_streammsg( void *bb,  size_t off,  size_t len,
                            uint32_t h ) noexcept;
  static MDMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                        MDDict *d,  MDMsgMem *m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

struct StreamFieldIter : public MDFieldIter {
  StreamData strm;
  void * operator new( size_t, void *ptr ) { return ptr; }
  StreamFieldIter( StreamMsg &m ) : MDFieldIter( m ),
    strm( &((uint8_t *) m.msg_buf)[ m.msg_off ], m.slen,
          &((uint8_t *) m.msg_buf)[ m.msg_off + m.slen ], m.glen,
          &((uint8_t *) m.msg_buf)[ m.msg_off + m.slen + m.glen ], m.plen ) {
    this->strm.open();
  }
  virtual int get_name( MDName &name ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
};

}
}

#endif
