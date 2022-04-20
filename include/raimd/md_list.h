#ifndef __rai_raimd__md_list_h__
#define __rai_raimd__md_list_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

enum ListStatus {
  LIST_OK        = 0,
  LIST_NOT_FOUND = 1,
  LIST_FULL      = 2
};

/* max size is hdr of ListStorage<uint64_t, uint32_t> */
static const size_t LIST_HDR_OOB_SIZE =
  sizeof( uint64_t ) + sizeof( uint32_t ) * 2;

struct ListHeader {
  size_t sig;
  size_t index_mask, data_mask;
  void * blobp;

  size_t index_size( void ) const { return this->index_mask+1; }
  size_t data_size( void )  const { return this->data_mask+1; }
  size_t max_count( void )  const { return this->index_mask; }
  size_t index( size_t i )  const { return i & this->index_mask; }
  size_t length( size_t l ) const { return l & this->data_mask; }
  /* wrap len around */
  size_t data_offset( size_t start,  ssize_t len ) const {
    return ( start + len ) & this->data_mask;
  }
  void * blob( size_t off ) const {
    return &((uint8_t *) this->blobp)[ off ];
  }
  /* compare mem with blob region */
  bool equals( size_t start,  const void *mem,  size_t len ) const {
    size_t sz = this->data_size();
    if ( start + len <= sz )
      return ::memcmp( this->blob( start ), mem, len ) == 0;
    size_t part = sz - start;
    return ::memcmp( this->blob( start ), mem, part ) == 0 &&
           ::memcmp( this->blob( 0 ), &((const char *) mem)[ part ],
                     len - part );
  }
  /* compare concatenated mem1 + mem2 with blob region */
  bool equals( size_t start,  const void *mem1,  size_t len1,
                              const void *mem2,  size_t len2 ) const {
    return ( len1 == 0 || this->equals( start, mem1, len1 ) ) &&
           ( len2 == 0 ||
             this->equals( this->data_offset( start, len1 ), mem2, len2 ) );
  }
  /* copy from blob region to dest */
  void copy( void *dest,  size_t off,  size_t len ) const {
    size_t sz = this->data_size();
    if ( off + len <= sz )
      ::memcpy( dest, this->blob( off ), len );
    else {
      size_t part = sz - off;
      ::memcpy( dest, this->blob( off ), part );
      ::memcpy( &((char *) dest)[ part ], this->blob( 0 ), len - part );
    }
  }
  /* copy from src into blob region */
  void copy2( size_t off,  const void *src,  size_t len ) const {
    size_t sz = this->data_size();
    if ( off + len <= sz )
      ::memcpy( this->blob( off ), src, len );
    else {
      size_t part = sz - off;
      ::memcpy( this->blob( off ), src, part );
      ::memcpy( this->blob( 0 ), &((char *) src)[ part ], len - part );
    }
  }
};

static inline size_t sz_min( size_t a,  size_t b ) { return a < b ? a : b; }
static inline size_t sz_max( size_t a,  size_t b ) { return a > b ? a : b; }

struct ListVal {
  const void *data, *data2;
  size_t sz, sz2;
  void zero( void ) {
    this->data = this->data2 = NULL;
    this->sz = this->sz2 = 0;
  }
  size_t length( void ) const {
    return this->sz + this->sz2;
  }
  /* concatenate data and data2 */
  size_t concat( void *out,  size_t out_sz ) const {
    size_t y, x = sz_min( this->sz, out_sz );
    if ( x != 0 ) {
      ::memcpy( out, this->data, x );
      if ( x == out_sz )
        return out_sz;
    }
    y = sz_min( this->sz2, out_sz - x );
    if ( y != 0 ) {
      ::memcpy( &((char *) out)[ x ], this->data2, y );
    }
    return x + y;
  }
  uint64_t u64( void ) const {
    uint64_t val = 0;
    this->concat( &val, 8 );
    return val;
  }
  uint32_t u32( void ) const {
    uint32_t val = 0;
    this->concat( &val, 4 );
    return val;
  }
  /* get data pointer, with copy or malloc if region is split into data/data2 */
  size_t unitary( void *&out,  void *buf,  size_t buf_sz,  bool &is_a ) const {
    const size_t len = this->length();
    if ( len == this->sz ) {
      out = (void *) this->data;
      return len;
    }
    /* if sz != 0 and sz2 != 0 */
    if ( buf_sz < len ) {
      buf = ::malloc( len );
      if ( buf == NULL ) {
        out = NULL;
        return 0;
      }
      is_a = true;
    }
    out = buf;
    ::memcpy( buf, this->data, this->sz );
    ::memcpy( &((char *) buf)[ this->sz ], this->data2, this->sz2 );
    return len;
  }
  /* get data pointer, with copy or alloc if region is split into data/data2 */
  size_t unitary( void *&out,  MDMsgMem &mem,
                  void *buf = NULL,  size_t buf_sz = 0 ) const {
    const size_t len = this->length();
    if ( len == this->sz ) {
      out = (void *) this->data;
      return len;
    }
    if ( buf_sz < len )
      mem.alloc( len, &buf );
    out = buf;
    ::memcpy( buf, this->data, this->sz );
    ::memcpy( &((char *) buf)[ this->sz ], this->data2, this->sz2 );
    return len;
  }
  size_t dup( MDMsgMem &mem,  void *buf ) const {
    mem.alloc( this->length(), buf );
    char * ptr = *(char **) buf;
    ::memcpy( ptr, this->data, this->sz );
    if ( this->sz2 != 0 )
      ::memcpy( &ptr[ this->sz ], this->data2, this->sz2 );
    return this->length();
  }
  void unite( MDMsgMem &mem ) {
    size_t len = this->length();
    if ( len == this->sz )
      return;
    void * buf;
    mem.alloc( len, &buf );
    ::memcpy( buf, this->data, this->sz );
    ::memcpy( &((char *) buf)[ this->sz ], this->data2, this->sz2 );
    this->data  = buf;
    this->data2 = NULL;
    this->sz    = len;
    this->sz2   = 0;
  }
  /* does memcmp:  key - lv == lv.cmp_key( key, keylen ) */
  int cmp_key( const void *key,  size_t keylen ) const {
    size_t len = sz_min( keylen, this->sz );
    int    cmp = ::memcmp( key, this->data, len );
    if ( cmp == 0 ) {
      if ( keylen < this->sz )
        cmp = -1;
      else if ( this->sz2 == 0 ) {
        if ( keylen > this->sz )
          cmp = 1;
      }
      else {
        key     = &((const char *) key)[ this->sz ];
        keylen -= this->sz;
        len     = sz_min( keylen, this->sz2 );
        cmp     = ::memcmp( key, this->data2, len );
        if ( cmp == 0 ) {
          if ( keylen < this->sz2 )
            cmp = -1;
          else if ( keylen > this->sz2 )
            cmp = 1;
        }
      }
    }
    return cmp;
  }
  int cmp_key( const ListVal &lv ) {
    if ( lv.sz2 == 0 )
      return cmp_key( lv.data, lv.sz );
    for ( size_t i = 0; ; i++ ) {
      uint8_t b, c;
      if ( i < this->sz )
        b = ((uint8_t *) this->data)[ i ];
      else if ( i < this->sz + this->sz2 )
        b = ((uint8_t *) this->data2)[ i - this->sz ];
      else { /* i == this->sz + this->sz2 */
        if ( i == lv.sz + lv.sz2 ) /* this == lv */
          return 0;
        return -1; /* this < lv */
      }
      if ( i < lv.sz )
        c = ((uint8_t *) lv.data)[ i ];
      else if ( i < lv.sz + lv.sz2 )
        c = ((uint8_t *) lv.data2)[ i - lv.sz ];
      else /* i == lv.sz + lv.sz2 */
        return 1; /* this > lv */
      if ( b != c )
        return ( b < c ) ? -1 : 1;
    }
  }
  size_t copy_out( void *dest,  size_t off,  size_t len ) const {
    size_t i = 0;
    while ( off < this->sz ) {
      ((uint8_t *) dest)[ i++ ] = ((const uint8_t *) this->data)[ off++ ];
      if ( --len == 0 )
        return i;
    }
    off -= this->sz;
    while ( off < this->sz2 ) {
      ((uint8_t *) dest)[ i++ ] = ((const uint8_t *) this->data2)[ off++ ];
      if ( --len == 0 )
        break;
    }
    return i;
  }
  size_t copy_in( const void *src,  size_t off,  size_t len ) {
    size_t i = 0;
    while ( off < this->sz ) {
      ((uint8_t *) this->data)[ off++ ] = ((const uint8_t *) src)[ i++ ];
      if ( --len == 0 )
        return i;
    }
    off -= this->sz;
    while ( off < this->sz2 ) {
      ((uint8_t *) this->data)[ off++ ] = ((const uint8_t *) src)[ i++ ];
      if ( --len == 0 )
        break;
    }
    return i;
  }
  uint8_t get_byte( size_t off ) const {
    if ( off < this->sz )
      return ((const uint8_t *) this->data)[ off ];
    return ((const uint8_t *) this->data2)[ off - this->sz ];
  }
  void put_byte( size_t off,  uint8_t b ) {
    if ( off < this->sz )
      ((uint8_t *) this->data)[ off ] = b;
    else
      ((uint8_t *) this->data2)[ off - this->sz ] = b;
  }
  int cmp( const void *blob,  size_t off,  size_t len ) const {
    size_t i = 0;
    while ( off < this->sz ) {
      int8_t j = ((const int8_t *) blob)[ i++ ] -
                 ((const int8_t *) this->data)[ off++ ];
      if ( j != 0 || --len == 0 )
        return j;
    }
    off -= this->sz;
    while ( off < this->sz2 ) {
      int8_t j = ((const int8_t *) blob)[ i++ ] -
                 ((const int8_t *) this->data)[ off++ ];
      if ( j != 0 || --len == 0 )
        return j;
    }
    return 1; /* len > sz + sz2 */
  }
};

static const uint8_t mt_list[] = {0xe4,0xf7,3,3,0,0,0,0,0,0,0,0,0,0,0,0};

#if __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
template <class UIntSig, class UIntType>
struct ListStorage {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  const UIntSig  _list_sig;       /* copied to ListHeader on dereference */
  const UIntType _list_index_mask,
                 _list_data_mask;

  UIntType first,     /* hd list: idx[ first ] */
           count,     /* tl list: idx[ (first + count) % index_mask ] */
           data_start,/* hd data; blob[ data_start ] */
           data_len;  /* tl data; blob[ (data_start + data_len) % data_mask ] */
  /* vector of offsets for each element */
  UIntType &idx( size_t x ) const {
    return ((UIntType *) (void *) &this[ 1 ])[ x ];
  }
  ListStorage() {}
  ListStorage( const UIntSig sig,  const size_t idx_size,
               const size_t dat_size ) :
    _list_sig( sig ), _list_index_mask( (UIntType) ( idx_size - 1 ) ),
    _list_data_mask( (UIntType) ( dat_size - 1 ) ) {}

  void init( ListHeader &hdr ) {
    this->first      = 0;
    this->count      = 0;
    this->data_start = 0;
    this->data_len   = 0;
    this->idx( 0 )   = 0;
    this->open( hdr );
 }
 void open( ListHeader &hdr,  const void *oob = NULL,  size_t loob = 0 ) const {
    /* Out of band header to defang mutations.  The hdr controls the sandbox
     * of the list, derefs are restricted to the areas within the masks */
    if ( loob >= sizeof( this->_list_sig ) +
                 sizeof( this->_list_index_mask ) +
                 sizeof( this->_list_data_mask ) ) {
      const ListStorage<UIntSig, UIntType> & p =
        *(const ListStorage<UIntSig, UIntType> *) oob;
      hdr.sig        = p._list_sig;
      hdr.index_mask = p._list_index_mask;
      hdr.data_mask  = p._list_data_mask;
    }
    else {
      hdr.sig        = this->_list_sig;
      hdr.index_mask = this->_list_index_mask;
      hdr.data_mask  = this->_list_data_mask;
    }
    hdr.blobp = (void *) &this->idx( hdr.index_size() );
  }
  bool empty( void ) const {
    return this->count == 0;
  }
  bool data_full( const ListHeader &hdr,  size_t size ) const {
    return size + (size_t) this->data_len >
           hdr.data_mask; /* can't store data_mask + 1 */
  }
  bool full( const ListHeader &hdr,  size_t size ) const {
    /* idx[] needs at least one empty slot for start and end */
    return (size_t) this->count >= hdr.max_count() ||
           this->data_full( hdr, size );
  }
  /* copy into circular buffer */
  void copy_into( const ListHeader &hdr,  const void *data,  size_t size,
                  size_t start ) const {
    hdr.copy2( start, data, size );
  }
  /* return a reference to the nth element starting from off */
  UIntType &index_ref( const ListHeader &hdr,  size_t n ) {
    return this->idx( ( (size_t) this->first + n ) & hdr.index_mask );
  }
  /* push size at tail */
  ListStatus rpush_size( const ListHeader &hdr,  size_t size,  size_t &start ) {
    size_t end;
    if ( this->full( hdr, size ) )
      return LIST_FULL;
    start = this->get_offset( hdr, this->count );
    end   = hdr.data_offset( start, size );
    this->index_ref( hdr, ++this->count ) = (UIntType) end;
    this->data_len += (UIntType) size;
    return LIST_OK;
  }
  /* push data/size at tail */
  ListStatus rpush( const ListHeader &hdr,  const void *data,  size_t size ) {
    size_t start;
    ListStatus lstat = this->rpush_size( hdr, size, start );
    if ( lstat == LIST_OK )
      this->copy_into( hdr, data, size, start );
    return lstat;
  }
  /* push data/size at tail */
  ListStatus rpush( const ListHeader &hdr,  const ListVal &lv ) {
    size_t start;
    ListStatus lstat = this->rpush_size( hdr, lv.length(), start );
    if ( lstat == LIST_OK ) {
      if ( lv.sz > 0 )
        this->copy_into( hdr, lv.data, lv.sz, start );
      if ( lv.sz2 > 0 ) {
        start = hdr.data_offset( start, lv.sz );
        this->copy_into( hdr, lv.data2, lv.sz2, start );
      }
    }
    return lstat;
  }
  /* push size at head */
  ListStatus lpush_size( const ListHeader &hdr,  size_t size,  size_t &start ) {
    size_t end;
    if ( this->full( hdr, size ) )
      return LIST_FULL;
    end   = this->get_offset( hdr, 0 ),
    start = hdr.data_offset( end, -(ssize_t) size );
    this->first = ( this->first - 1 ) & hdr.index_mask;
    this->count++;
    this->data_start = (UIntType) start;
    this->index_ref( hdr, 0 ) = (UIntType) start;
    this->data_len += (UIntType) size;
    return LIST_OK;
  }
  /* push data/size at tail */
  ListStatus lpush( const ListHeader &hdr,  const void *data,  size_t size ) {
    size_t start;
    ListStatus lstat = this->lpush_size( hdr, size, start );
    if ( lstat == LIST_OK )
      this->copy_into( hdr, data, size, start );
    return lstat;
  }
  /* return data offset at idx, if end is zero then return data_size */
  size_t get_offset( const ListHeader &hdr,  size_t i,
                     bool end = false ) const {
    i = ( this->first + i ) & hdr.index_mask;
    size_t j = this->idx( i );
    if ( ! end ||  /* if not end of segment, then 0 is the start offset    */
         j != 0 || /* if j == 0, it is both the start and the end offset   */
         i == this->first ||  /* an elem cannot span the whole data_size() */
         this->idx( ( i - 1 ) & hdr.index_mask ) == 0 ) /* check prev is 0 */
      return j;
    return hdr.data_size(); /* change 0 to data_size(), it is at the end */
  }
  /* fetch nth item, if wrapped around buffer, p2/sz2 will be set */
  ListStatus lindex( const ListHeader &hdr,  size_t n,  ListVal &lv ) const {
    lv.zero();
    if ( n >= (size_t) this->count )
      return LIST_NOT_FOUND;
    size_t start = this->get_offset( hdr, n ),
           end   = this->get_offset( hdr, n+1, true );
    lv.data = hdr.blob( start );
    if ( end >= start ) { /* not wrapped */
      lv.sz = end - start;
      return LIST_OK;
    }
    /* wrapped, head is at data end, tail is at data start */
    size_t len = hdr.data_size() - start;
    lv.sz    = len;
    lv.data2 = hdr.blob( 0 );
    lv.sz2   = end;
    return LIST_OK;
  }
  /* scan tail to head to find data, pre decrement */
  ListStatus scan_rev( const ListHeader &hdr,  const void *data,  size_t size,
                       size_t &pos ) const {
    size_t start, end, len;
    for ( size_t n = pos; n > 0; ) {
      len = this->get_size( hdr, --n, start, end );
      if ( len == size && hdr.equals( start, data, size ) ) {
        pos = n;
        return LIST_OK;
      }
    }
    return LIST_NOT_FOUND;
  }
  /* scan head to tail to find data, post increment */
  ListStatus scan_fwd( const ListHeader &hdr,  const void *data,  size_t size,
                       size_t &pos ) const {
    size_t start, end, len, cnt = hdr.index( this->count );
    for ( size_t n = pos; n < cnt; n++ ) {
      len = this->get_size( hdr, n, start, end );
      if ( len == size && hdr.equals( start, data, size ) ) {
        pos = n;
        return LIST_OK;
      }
    }
    return LIST_NOT_FOUND;
  }
  /* find an piv in the list and insert data before/after it */
  ListStatus linsert( const ListHeader &hdr,  const void *piv,  size_t pivlen,
                      const void *data,  size_t size,  bool after ) {
    size_t n = 0;
    if ( this->scan_fwd( hdr, piv, pivlen, n ) == LIST_NOT_FOUND )
      return LIST_NOT_FOUND;
    if ( after ) {
      if ( n == (size_t) this->count )
        return this->rpush( hdr, data, size );
    }
    else {
      if ( n == 0 )
        return this->lpush( hdr, data, size );
      n -= 1;
    }
    return this->insert( hdr, n, data, size );
  }
  /* insert after data n */
  ListStatus insert( const ListHeader &hdr,  size_t n,  const void *data,
                     size_t size ) {
    size_t start;
    ListStatus lstat = this->insert_space( hdr, n, size, start );
    if ( lstat == LIST_OK )
      this->copy_into( hdr, data, size, start );
    return lstat;
  }
  /* inserts space *after* n, 0 <= n < count */
  ListStatus insert_space( const ListHeader &hdr,  size_t n,  size_t size,
                           size_t &start ) {
    if ( this->full( hdr, size ) )
      return LIST_FULL;
    this->move_tail( hdr, n, size );
    this->adjust_tail( hdr, n, size );
    for ( size_t end = ++this->count; end > n+1; end-- )
      this->index_ref( hdr, end ) = this->index_ref( hdr, end-1 );
    UIntType &j = this->index_ref( hdr, n+1 );
    j = (UIntType) hdr.data_offset( this->index_ref( hdr, n+2 ),
                                    -(ssize_t) size );
    start = j;
    this->data_len += (UIntType) size;
    return LIST_OK;
  }
  /* calculate new length after trimming */
  void trim_size( const ListHeader &hdr ) {
    size_t start = this->get_offset( hdr, 0 ),
           end   = this->get_offset( hdr, this->count, true );
    this->data_len = (UIntType) ( ( end >= start ) ? end - start :
                                  hdr.data_size() - start + end );
  }
  /* trim left */
  void ltrim( const ListHeader &hdr,  size_t n ) {
    if ( n > this->count )
      n = this->count;
    this->count -= (UIntType) n;
    this->first  = (UIntType) ( ( this->first + n ) & hdr.index_mask );
    this->trim_size( hdr );
  }
  /* trim right */
  void rtrim( const ListHeader &hdr,  size_t n ) {
    if ( n > this->count )
      n = this->count;
    this->count -= (UIntType) n;
    this->trim_size( hdr );
  }
  /* pop data item at head */
  ListStatus lpop( const ListHeader &hdr,  ListVal &lv ) {
    ListStatus n;
    n = this->lindex( hdr, 0, lv );
    if ( n != LIST_NOT_FOUND ) {
      this->first = ( this->first + 1 ) & hdr.index_mask;
      this->count -= 1;
      this->data_len -= (UIntType) ( lv.length() );
    }
    return n;
  }
  /* pop data item at tail */
  ListStatus rpop( const ListHeader &hdr,  ListVal &lv ) {
    ListStatus n;
    n = this->lindex( hdr, this->count - 1, lv );
    if ( n != LIST_NOT_FOUND ) {
      this->count -= 1;
      this->data_len -= (UIntType) ( lv.length() );
    }
    return n;
  }
  /* get the size of nth item */
  size_t get_size( const ListHeader &hdr,  size_t n,  size_t &start,
                   size_t &end ) const {
    start = this->get_offset( hdr, n );
    end   = this->get_offset( hdr, n+1, true );
    return ( end >= start ? end - start : hdr.data_size() - start + end );
  }
  size_t get_size( const ListHeader &hdr,  size_t n ) {
    size_t start, end;
    return this->get_size( hdr, n, start, end );
  }
  /* remove the nth item */
  ListStatus lrem( const ListHeader &hdr,  size_t n ) {
    size_t size = this->get_size( hdr, n );
    if ( n < this->count ) {
      if ( n == 0 || n + 1 == this->count ) {
        if ( n + 1 != this->count ) /* if not at tail, then at head */
          this->first = ( this->first + 1 ) & hdr.index_mask;
        this->count -= 1;
        this->data_len -= (UIntType) size;
        return LIST_OK;
      }
      if ( size > 0 )
        this->move_tail( hdr, n, -(ssize_t) size );
      for ( n = n + 1; n < this->count; n++ ) {
        UIntType &j = this->index_ref( hdr, n );
        j = (UIntType) hdr.data_offset( this->index_ref( hdr, n + 1 ),
                                        -(ssize_t) size );
      }
      this->count -= 1;
      this->data_len -= (UIntType) size;
      return LIST_OK;
    }
    return LIST_NOT_FOUND;
  }
  /* replace nth data item, which could expand or contract data buffer */
  ListStatus lset( const ListHeader &hdr,  size_t n,  const void *data,
                   size_t size ) {
    if ( n >= (size_t) this->count ) /* if n doesn't exist */
      return LIST_NOT_FOUND;
    size_t cur_size = this->get_size( hdr, n );
    ssize_t amt = (ssize_t) size - (ssize_t) cur_size;
    if ( amt > 0 ) { /* expand nth data item */
      if ( this->data_full( hdr, amt ) )
        return LIST_FULL;
    }
    if ( amt != 0 ) {
      if ( n < this->count / 2 ) {
        this->move_head( hdr, n, amt );
        this->adjust_head( hdr, n, amt );
      }
      else {
        this->move_tail( hdr, n, amt );
        this->adjust_tail( hdr, n, amt );
      }
      this->data_len += (UIntType) amt;
    }
    /* replace nth item */
    this->copy_into( hdr, data, size, this->get_offset( hdr, n ) );
    return LIST_OK;
  }
  /* expand or contract upto nth item, doesn't check for space available */
  void move_head( const ListHeader &hdr,  size_t n,  ssize_t amt ) {
    if ( n == 0 ) return; /* alter after head item */
    /* in the middle, moves the head backward amt bytes */
    size_t start     = this->get_offset( hdr, 0 ),
           end       = this->get_offset( hdr, n, true ),
           new_start = hdr.data_offset( start, -amt );
    if ( start <= end ) /* not wrapped around buffer */
      this->copy_move( hdr, start, end - start, new_start );
    else if ( amt < 0 ) {
      size_t len = hdr.data_size() - start;
      this->copy_move( hdr, 0, end, -amt );
      this->copy_move( hdr, start, len, new_start );
    }
    else {
      size_t len = hdr.data_size() - start;
      this->copy_move( hdr, start, len, new_start );
      this->copy_move( hdr, 0, end, new_start + len );
    }
  }
  void adjust_head( const ListHeader &hdr,  size_t n,  ssize_t amt ) {
    /* move index pointers */
    for ( ; ; n-- ) {
      UIntType &j = this->index_ref( hdr, n );
      j = (UIntType) hdr.data_offset( (size_t) j, -amt );
      if ( n == 0 )
        break;
    }
  }
  /* expand or contract after nth item, doesn't check for space available */
  void move_tail( const ListHeader &hdr,  size_t n,  ssize_t amt ) {
    if ( n == (size_t) this->count - 1 ) return; /* alter before tail item */
    /* in the middle, moves the tail forward amt bytes */
    size_t start     = this->get_offset( hdr, n+1 ),
           end       = this->get_offset( hdr, this->count, true ),
           new_start = hdr.data_offset( start, amt );
    if ( start <= end ) /* not wrapped around buffer */
      this->copy_move( hdr, start, end - start, new_start );
    else if ( amt > 0 ) {
      size_t len = hdr.data_size() - start;
      this->copy_move( hdr, 0, end, amt );
      this->copy_move( hdr, start, len, new_start );
    }
    else {
      size_t len = hdr.data_size() - start;
      this->copy_move( hdr, start, len, new_start );
      this->copy_move( hdr, 0, end, new_start + len );
    }
  }
  void adjust_tail( const ListHeader &hdr,  size_t n,  ssize_t amt ) {
    /* move index pointers */
    for ( n = n + 1; n <= this->count; n++ ) {
      UIntType &j = this->index_ref( hdr, n );
      j = (UIntType) hdr.data_offset( (size_t) j, amt );
    }
  }
  /* careful not to overwrite the buffer being moved, source is contiguous */
  void copy_move( const ListHeader &hdr,  size_t source,  size_t size,
                  size_t dest ) {
    if ( size == 0 )
      return;
    if ( dest + size <= hdr.data_size() )
      ::memmove( hdr.blob( dest ), hdr.blob( source ), size );
    else {
      char         tmp[ 256 ];
      size_t       len  = hdr.data_size() - dest; /* dest is split */
      void       * p    = NULL;
      const void * data = hdr.blob( source ); /* source is contiguous */ 
      void       * p0   = hdr.blob( 0 ),
                 * pdst = hdr.blob( dest );
      const void * dend = &((char *) data)[ len ]; /* split point in source */
      /* if splitting the buf cuts into the source data */
      if ( dest >= source || size - len > source + size ) {
        if ( size <= sizeof( tmp ) ) {
          ::memcpy( tmp, data, size );
          data = tmp;
          dend = &tmp[ len ];
        }
        else {
          p = ::malloc( size );
          ::memcpy( p, data, size );
          data = p;
          dend = &((char *) data)[ len ];
        }
      }
      size -= len;
      ::memmove( pdst, data, len );
      ::memmove( p0, dend, size );
      if ( p != NULL )
        ::free( p );
    }
  }
  int lverify( const ListHeader &hdr ) const {
    if ( hdr.index_mask != this->_list_index_mask )
      return -10;
    if ( hdr.data_mask != this->_list_data_mask )
      return -11;
    if ( hdr.blobp != (void *) &this->idx( hdr.index_size() ) )
      return -12;
    if ( this->count != hdr.index( this->count ) )
      return -13;
    size_t total = 0;
    for ( size_t i = 0; i < this->count; i++ ) {
      size_t start, end, size = this->get_size( hdr, i, start, end );
      if ( start >= hdr.data_size() )
        return -14;
      if ( end > hdr.data_size() )
        return -15;
      if ( size >= hdr.data_size() )
        return -16;
      total += size;
    }
    if ( total != this->data_len )
      return -17;
    return 0;
  }
  void lprint( const ListHeader &hdr ) const {
    printf( "idx_mask = %x ", (uint32_t) this->_list_index_mask );
    printf( "data_mask = %x\n", (uint32_t) this->_list_data_mask );
    for ( size_t i = 0; i < this->count; i++ ) {
      size_t start, end, size = this->get_size( hdr, i, start, end );
      printf( " %u -> %u : %u\n", (uint32_t) start, (uint32_t) end,
              (uint32_t) size );
    }
  }
  /* copy buffer, when start == end && count > 0, data part is full */
  size_t copy_data( const ListHeader &hdr,  void *data ) const {
    size_t size = 0;
    if ( this->count > 0 ) {
      size_t start = this->get_offset( hdr, 0 ),
             end   = this->get_offset( hdr, this->count, true );
      if ( start <= end ) {
        size = end - start;
        ::memcpy( data, hdr.blob( start ), size );
      }
      else {
        size = hdr.data_size() - start;
        ::memcpy( data, hdr.blob( start ), size );
        ::memcpy( &((char *) data)[ size ], hdr.blob( 0 ), end );
        size += end;
      }
    }
    return size;
  }
  /* copy data from a uint8_t index buffer */
  template<class T, class U>
  void copy( const ListHeader &myhdr,  const ListHeader &chdr,
             ListStorage<T, U> &cp ) const {
    size_t tmp_cnt = (size_t) this->count,
           tmp_len = (size_t) this->data_len;
    cp.count    = (U) tmp_cnt;
    cp.data_len = (U) tmp_len;
    if ( this->count > 0 ) {
      size_t start, end, size;
      this->copy_data( myhdr, chdr.blob( 0 ) );
      end = this->get_offset( myhdr, 0, true );
      for ( size_t n = 0; n < this->count; n++ ) {
        start = ( end == myhdr.data_size() ) ? 0 : end;
        end   = this->get_offset( myhdr, n + 1, true );
        size  = ( start <= end ? end - start : myhdr.data_size() - start + end);
        cp.idx( n + 1 ) = cp.idx( n ) + (U) size;
      }
    }
  }
};
#if __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif

typedef ListStorage<uint16_t, uint8_t>  ListStorage8;
typedef ListStorage<uint32_t, uint16_t> ListStorage16;
typedef ListStorage<uint64_t, uint32_t> ListStorage32;

struct ListData : public ListHeader {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  void       * listp;
  const size_t size;
  ListData() : listp( 0 ), size( 0 ) {}
  ListData( void *l,  size_t sz ) : listp( l ), size( sz ) {}

  /* test data storage used, alloc_size = the sum of idx_size + dat_size */
  static bool is_uint8( size_t alloc_size ) {
    return alloc_size < ( 0x100 << 1 );
  }
  static bool is_uint16( size_t alloc_size ) {
    return alloc_size < ( 0x10000 << 1 );
  }
  /* power of 2 larger or equal to sz */
  static size_t pow2size( size_t sz ) {
    if ( ( sz & ( sz - 1 ) ) != 0 )
      sz = (size_t) 1 << ( 64 - md_clzl( sz ) );
    return sz;
  }
  /* calc word size appropriate for count idx of data size items */
  static size_t alloc_size( size_t &idx_size,  size_t &dat_size ) {
    size_t sz, tz, lst_size;
    idx_size = ( idx_size < 2 ? 4 : ListData::pow2size( idx_size + 1 ) );
    dat_size = ( dat_size < 4 ? 8 : ListData::pow2size( dat_size + 3 ) );

    for ( int i = 0; ; i++ ) {
      /* choose the word size that can address the range offsets needed */
      if ( ( idx_size | ( dat_size - 1 ) ) <= 0xff ) {
        sz = sizeof( ListStorage8 ); /* byte size list */
        tz = sizeof( uint8_t );
      }
      else if ( ( idx_size | ( dat_size - 1 ) ) <= 0xffff ) {
        sz = sizeof( ListStorage16 ); /* short size list */
        tz = sizeof( uint16_t );
      }
      else {
        sz = sizeof( ListStorage32 ); /* int size list */
        tz = sizeof( uint32_t );
      }
      lst_size = sz + tz * idx_size + dat_size; /* total size of idx + data */
      /* there is a point at the edges where is_uintXX() doesn't work,
       * check that it is valid for the current word size */
      if ( ListData::is_uint8( lst_size ) ) { /* check that is_uint8() */
        if ( tz == sizeof( uint8_t ) )        /* works with lst_size */
          return lst_size;
      }
      else if ( ListData::is_uint16( lst_size ) ) { /* check is_uint16() */
        if ( tz == sizeof( uint16_t ) )
          return lst_size;
      }
      else {
        if ( tz == sizeof( uint32_t ) ) /* check is_uint32() */
          return lst_size;
      }
      /* a size is to small, increment it to the next power of 2 */
      if ( ( i & 1 ) == 0 )
        dat_size = ListData::pow2size( dat_size + 3 );
      else
        idx_size = ListData::pow2size( idx_size + 1 );
    }
  }
  /* this infers the amount of storange used by a blob by the header:
   * test signature = sig8 | sig16 | sig32
   * test length is larger than the header size
   * test that the index mask and the data masks are pow2 - 1
   * return the storage used by summing hdr + index + data size */
  static size_t mem_size( const void *ptr,  size_t len,  uint16_t sig8,
                          uint32_t sig16,  uint64_t sig32 ) {
    const uint8_t * buf = (const uint8_t *) ptr;
    if ( to_ival<uint16_t>( buf ) == sig8 && len > sizeof( ListStorage8 ) ) {
      const ListStorage8 &hdr = *(const ListStorage8 *) ptr;
      if ( is_mask( hdr._list_index_mask ) && is_mask( hdr._list_data_mask ) )
        return sizeof( ListStorage8 ) +
          ( hdr._list_index_mask + 1 ) + ( hdr._list_data_mask + 1 );
    }
    if ( to_ival<uint32_t>( buf ) == sig16 && len > sizeof( ListStorage16 ) ) {
      const ListStorage16 &hdr = *(const ListStorage16 *) ptr;
      if ( is_mask( hdr._list_index_mask ) && is_mask( hdr._list_data_mask ) )
        return sizeof( ListStorage16 ) +
          ( hdr._list_index_mask + 1 ) * 2 + ( hdr._list_data_mask + 1 );
    }
    if ( to_ival<uint64_t>( buf ) == sig32 && len > sizeof( ListStorage32 ) ) {
      const ListStorage32 &hdr = *(const ListStorage32 *) ptr;
      if ( is_mask( hdr._list_index_mask ) && is_mask( hdr._list_data_mask ) )
        return sizeof( ListStorage32 ) +
          ( hdr._list_index_mask + 1 ) * 4 + ( hdr._list_data_mask + 1 );
    }
    return 0; /* not recognized */
  }
  /* add idx and data to current size * 1.5 */
  size_t resize_size( size_t &idx_size,  size_t &dat_size ) {
    dat_size += this->data_len();
    dat_size += dat_size / 2 + 2;
    idx_size += this->count();
    idx_size += idx_size / 2 + 2;
    return alloc_size( idx_size, dat_size );
  }
  /* return the amount of storage used */
  size_t used_size( size_t &idx_size,  size_t &dat_size ) const {
    dat_size = this->data_len();
    idx_size = this->count();
    return alloc_size( idx_size, dat_size );
  }
  /* magic numbers for the headers */
  static const uint16_t lst8_sig  = 0xf7e4U;
  static const uint32_t lst16_sig = 0xddbe7ae4UL;
  static const uint64_t lst32_sig = 0xa5f5ff85c9f6c3e4ULL;
#define LIST_CALL( GOTO ) \
  ( is_uint8( this->size ) ? ((ListStorage8 *) this->listp)->GOTO : \
    is_uint16( this->size ) ? ((ListStorage16 *) this->listp)->GOTO : \
                              ((ListStorage32 *) this->listp)->GOTO )
  /* after allocation, initialize the header of list */
  void init_sig( size_t count,  size_t data_len,  uint16_t sig8,
                 uint32_t sig16,  uint64_t sig32 ) {
    if ( is_uint8( this->size ) )
      (new ( this->listp )
        ListStorage8( sig8, count, data_len ))->init( *this );
    else if ( is_uint16( this->size ) )
      (new ( this->listp )
        ListStorage16( sig16, count, data_len ))->init( *this );
    else
      (new ( this->listp )
        ListStorage32( sig32, count, data_len ))->init( *this );
  }
  /* uses lst magics above */
  void init( size_t count,  size_t data_len ) {
    this->init_sig( count, data_len, lst8_sig, lst16_sig, lst32_sig );
  }
  bool in_mem_region( const void *m,  size_t sz ) const {
    if ( (const char *) m >= (const char *) this->listp ) {
      if ( &((const char *) m)[ sz ] <=
           &((const char *) this->listp)[ this->size ] )
        return true;
    }
    return false;
  }
  /* open uses the listp as the list, with a out of band header (optional) */
  /* the oob hdr is needed in the case of reading a mutating structure */
  void open( const void *oob = NULL,  size_t loob = 0 ) {
    LIST_CALL( open( *this, oob, loob ) );
  }
  /* return the number of list elems */
  size_t count( void ) const {
    return this->index( LIST_CALL( count ) );
  }
  /* the total length of data */
  size_t data_len( void ) const {
    return this->length( LIST_CALL( data_len ) );
  }

  static bool sig_eq( uint64_t x,  uint64_t y ) {
    return ( x >> 4 ) == ( y >> 4 ); /* lower 4 bits differ based on type */
  }
  /* debug for list verification, checks header sig and iterates the list */
  int lverify( void ) const {
    if ( is_uint8( this->size ) ) {
      if ( ! sig_eq( ((ListStorage8 *) this->listp)->_list_sig, lst8_sig ) )
        return -1;
    }
    else if ( is_uint16( this->size ) ) {
      if ( ! sig_eq( ((ListStorage16 *) this->listp)->_list_sig, lst16_sig ) )
        return -2;
    }
    else {
      if ( ! sig_eq( ((ListStorage32 *) this->listp)->_list_sig, lst32_sig ) )
        return -3;
    }
    return LIST_CALL( lverify( *this ) );
  }
  void lprint( void ) const {
    return LIST_CALL( lprint( *this ) );
  }
  /* template to copy one list to another */
  template<class T, class U>
  void copy( ListData &list ) const {
    if ( is_uint8( this->size ) )
      ((ListStorage8 *) this->listp)->copy<T, U>( *this, list,
                                            *(ListStorage<T, U> *) list.listp );
    else if ( is_uint16( this->size ) )
      ((ListStorage16 *) this->listp)->copy<T, U>( *this, list,
                                            *(ListStorage<T, U> *) list.listp );
    else
      ((ListStorage32 *) this->listp)->copy<T, U>( *this, list,
                                            *(ListStorage<T, U> *) list.listp );
  }
  /* use template above based on size */
  void copy( ListData &list ) const {
    if ( is_uint8( list.size ) )
      this->copy<uint16_t, uint8_t>( list );
    else if ( is_uint16( list.size ) )
      this->copy<uint32_t, uint16_t>( list );
    else
      this->copy<uint64_t, uint32_t>( list );
  }
  size_t offset( size_t n ) const {
    return LIST_CALL( get_offset( *this, n ) );
  }
  /* the list operations routed to the list storage used */
  ListStatus rpush( const void *data,  size_t size ) {
    return LIST_CALL( rpush( *this, data, size ) );
  }
  ListStatus rpush( const ListVal &lv ) {
    return LIST_CALL( rpush( *this, lv ) );
  }
  ListStatus lpush( const void *data,  size_t size ) {
    return LIST_CALL( lpush( *this, data, size ) );
  }
  ListStatus lindex( size_t n,  ListVal &lv ) const {
    return LIST_CALL( lindex( *this, n, lv ) );
  }
  ListStatus lindex_cmp_key( size_t n,  ListVal &key ) const {
    ListVal lv;
    if ( this->lindex( n, lv ) == LIST_OK && lv.cmp_key( key ) == 0 )
      return LIST_OK;
    return LIST_NOT_FOUND;
  }
  ListStatus lindex_cmp_key( size_t n,  const char *key,  size_t keylen) const {
    ListVal lv;
    if ( this->lindex( n, lv ) == LIST_OK && lv.cmp_key( key, keylen ) == 0 )
      return LIST_OK;
    return LIST_NOT_FOUND;
  }
  void ltrim( size_t n ) {
    LIST_CALL( ltrim( *this, n ) );
  }
  void rtrim( size_t n ) {
    LIST_CALL( rtrim( *this, n ) );
  }
  ListStatus lpop( ListVal &lv ) {
    return LIST_CALL( lpop( *this, lv ) );
  }
  ListStatus rpop( ListVal &lv ) {
    return LIST_CALL( rpop( *this, lv ) );
  }
  ListStatus lset( size_t n,  const void *data,  size_t size ) {
    return LIST_CALL( lset( *this, n, data, size ) );
  }
  ListStatus linsert( const void *piv,  size_t pivlen,  const void *data,
                      size_t size,  bool after ) {
    return LIST_CALL( linsert( *this, piv, pivlen, data, size, after ) );
  }
  ListStatus scan_fwd( const void *data,  size_t size,  size_t &pos ) {
    return LIST_CALL( scan_fwd( *this, data, size, pos ) );
  }
  ListStatus scan_rev( const void *data,  size_t size,  size_t &pos ) {
    return LIST_CALL( scan_rev( *this, data, size, pos ) );
  }
  ListStatus lrem( size_t n ) {
    return LIST_CALL( lrem( *this, n ) );
  }
};

struct ListMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }

  ListMsg( void *bb,  size_t off,  size_t len,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, len, d, m ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;

  static bool is_listmsg( void *bb,  size_t off,  size_t len,
                          uint32_t h ) noexcept;
  static MDMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                        MDDict *d,  MDMsgMem *m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

}
}
#endif
