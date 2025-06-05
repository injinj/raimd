#ifndef __rai_raimd__md_set_h__
#define __rai_raimd__md_set_h__

#include <raimd/md_hash.h>

#ifdef __cplusplus
extern "C" {
#endif
  
#ifdef __cplusplus
}   
namespace rai {
namespace md {

enum SetStatus {
  SET_OK        = HASH_OK,        /* success and/or new key/value */
  SET_NOT_FOUND = HASH_NOT_FOUND, /* key not found */
  SET_FULL      = HASH_FULL,      /* no room, needs resize */
  SET_UPDATED   = HASH_UPDATED,   /* success, replaced existing key/value */
  SET_EXISTS    = HASH_EXISTS,    /* found existing item, not updated */
  SET_BAD       = HASH_BAD        /* data corrupt */
};

template <class UIntSig, class UIntType>
struct SetStorage : public HashStorage<UIntSig, UIntType> {

  bool match_member( const ListHeader &hdr,  const void *key,  size_t keylen,
                     size_t pos ) const {
    size_t start, end, len;
    if ( pos < hdr.index( this->count ) ) {
      len = this->get_size( hdr, pos, start, end );
      if ( len == keylen )
        return hdr.equals( start, key, keylen );
    }
    return false;
  }

  bool match_member( const ListHeader &hdr,  const ListVal &lv,
                     size_t pos ) const {
    size_t start, end, len;
    if ( pos < hdr.index( this->count ) ) {
      len = this->get_size( hdr, pos, start, end );
      if ( len == lv.sz + lv.sz2 )
        return hdr.equals( start, lv.data, lv.sz, lv.data2, lv.sz2 );
    }
    return false;
  }

  SetStatus sismember( const ListHeader &hdr,  const void *key,  size_t keylen,
                       HashPos &pos ) const {
    for ( ; ; pos.i++ ) {
      if ( ! this->hash_find( hdr, pos ) )
        return SET_NOT_FOUND;
      if ( this->match_member( hdr, key, keylen, pos.i ) )
        return SET_OK;
    }
  }

  SetStatus sismember( const ListHeader &hdr,  const ListVal &lv,
                       HashPos &pos ) const {
    for ( ; ; pos.i++ ) {
      if ( ! this->hash_find( hdr, pos ) )
        return SET_NOT_FOUND;
      if ( this->match_member( hdr, lv, pos.i ) )
        return SET_OK;
    }
  }

  SetStatus sappend( const ListHeader &hdr,  const void *key,  size_t keylen,
                     const HashPos &pos ) {
    SetStatus sstat = (SetStatus) this->hash_append( hdr, pos );
    if ( sstat == SET_OK ) {
      sstat = (SetStatus) this->rpush( hdr, key, keylen );
      if ( sstat == SET_OK )
        return SET_UPDATED;
    }
    return sstat;
  }

  SetStatus sappend( const ListHeader &hdr,  const ListVal &lv,
                     const HashPos &pos ) {
    SetStatus sstat = (SetStatus) this->hash_append( hdr, pos );
    if ( sstat == SET_OK ) {
      sstat = (SetStatus) this->rpush( hdr, lv );
      if ( sstat == SET_OK )
        return SET_UPDATED;
    }
    return sstat;
  }

  SetStatus sadd( const ListHeader &hdr,  const void *key,  size_t keylen,
                  HashPos &pos ) {
    SetStatus sstat = this->sismember( hdr, key, keylen, pos );
    if ( sstat == SET_OK )
      return SET_OK;
    return this->sappend( hdr, key, keylen, pos );
  }

  SetStatus sadd( const ListHeader &hdr,  const ListVal &lv,  HashPos &pos ) {
    SetStatus sstat = this->sismember( hdr, lv, pos );
    if ( sstat == SET_OK )
      return SET_OK;
    return this->sappend( hdr, lv, pos );
  }

  SetStatus srem( const ListHeader &hdr,  const void *key,  size_t keylen,
                  HashPos &pos ) {
    SetStatus sstat = this->sismember( hdr, key, keylen, pos );
    if ( sstat == SET_NOT_FOUND )
      return SET_NOT_FOUND;
    sstat = (SetStatus) this->lrem( hdr, pos.i );
    if ( sstat == SET_OK )
      this->hash_delete( hdr, pos.i );
    return sstat;
  }

  SetStatus spopn( const ListHeader &hdr,  size_t n ) {
    SetStatus sstat = (SetStatus) this->lrem( hdr, n );
    if ( sstat == SET_OK )
      this->hash_delete( hdr, n );
    return sstat;
  }

  SetStatus spopall( const ListHeader & ) {
    if ( this->count > 1 )
      this->count = 1;
    return SET_OK;
  }

  int sverify( const ListHeader &hdr ) const {
    if ( this->count == 0 )
      return 0;
    size_t          start,
                    end,
                    len,
                    sz;
    const uint8_t * el;
    ListVal         lv;
    uint32_t         h;
    sz = this->get_size( hdr, 0, start, end );
    if ( this->count > sz )
      return -30;
    len = sz_min( this->count, sz );
    for ( size_t i = 1; i < len; i++ ) {
      SetStatus sstat = (SetStatus) this->lindex( hdr, i, lv );
      if ( sstat == SET_NOT_FOUND )
        return -31;
      if ( lv.sz + lv.sz2 >= hdr.data_size() )
        return -32;
      h = hash32( lv.data, lv.sz, 0 );
      if ( lv.sz2 > 0 )
        h = hash32( lv.data2, lv.sz2, h );
      el = (const uint8_t *) hdr.blob( hdr.data_offset( start, i ) );
      if ( el[ 0 ] != (uint8_t) h )
        return -33;
    }
    return 0;
  }
  /* me={a,b,c,d} | x={a,c,e} = me={a,b,c,d,e} */
  template<class T, class U>
  SetStatus sunion( const ListHeader &myhdr,  const ListHeader &xhdr,
                    SetStorage<T, U> &x,  MergeCtx &ctx ) {
    HashPos   pos;
    ListVal   lv;
    SetStatus sstat;

    if ( x.count <= 1 )
      return SET_OK;
    if ( ! ctx.is_started ) {
      size_t start, end, sz;
      this->get_hash_bits( myhdr, ctx.bits );
      sz = x.get_size( xhdr, 0, start, end );
      ctx.start( start, sz, x.count, xhdr.data_size(), xhdr.blob( start ),
                 xhdr.blob( 0 ), false );
    }
    while ( ! ctx.is_complete() ) {
      /* get element */
      sstat = (SetStatus) x.lindex( xhdr, ctx.j, lv );
      if ( sstat != SET_OK )
        return sstat;
      if ( ! ctx.get_pos( pos ) )
        sstat = this->sappend( myhdr, lv, pos );
      else /* search for it */
        sstat = this->sadd( myhdr, lv, pos );
      if ( sstat == SET_FULL || sstat == SET_BAD )
        return sstat;
      ctx.incr();
    }
    return SET_OK;
  }
  /* intersect:  me={a,b,c,d} & x={a,c,e} = me={a,c} */
  /* difference: me={a,b,c,d} - x={a,c,e} = me={b,d} */
  template<class T, class U>
  SetStatus sinter( const ListHeader &myhdr,  const ListHeader &xhdr,
                    SetStorage<T, U> &x,  MergeCtx &ctx,
                    bool is_intersect /* or diff */) {
    HashPos   pos;
    ListVal   lv;
    SetStatus sstat;
    if ( x.count <= 1 || this->count <= 1 ) {
      if ( is_intersect )
        this->count = 0;
      return SET_OK;
    }
    if ( ! ctx.is_started ) {
      size_t start, end, sz;
      x.get_hash_bits( xhdr, ctx.bits );
      sz = this->get_size( myhdr, 0, start, end );
      ctx.start( start, sz, this->count, myhdr.data_size(), myhdr.blob( start ),
                 myhdr.blob( 0 ), true );
    }
    while ( ! ctx.is_complete() ) {
      bool diff = ! ctx.get_pos( pos );
      if ( ! diff ) {
        sstat = (SetStatus) this->lindex( myhdr, ctx.j, lv );
        if ( sstat != SET_OK )
          return sstat;
        if ( x.sismember( xhdr, lv, pos ) == SET_NOT_FOUND )
          diff = true;
      }
      if ( is_intersect ) {
        if ( diff )
          this->spopn( myhdr, ctx.j );
      }
      else {
        if ( ! diff )
          this->spopn( myhdr, ctx.j );
      }
      ctx.incr();
    }
    return SET_OK;
  }
};

typedef SetStorage<uint16_t, uint8_t>  SetStorage8;
typedef SetStorage<uint32_t, uint16_t> SetStorage16;
typedef SetStorage<uint64_t, uint32_t> SetStorage32;

struct SetData : public HashData {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  SetData() : HashData() {}
  SetData( void *l,  size_t sz ) : HashData( l, sz ) {}

  static const uint16_t set8_sig  = 0xf7e6U;
  static const uint32_t set16_sig = 0xddbe7ae6UL;
  static const uint64_t set32_sig = 0xa5f5ff85c9f6c3e6ULL;
  void init( size_t count,  size_t data_len ) {
    this->init_sig( count, data_len, set8_sig, set16_sig, set32_sig );
  }

#define SET_CALL( GOTO ) \
  ( is_uint8( this->size ) ? ((SetStorage8 *) this->listp)->GOTO : \
    is_uint16( this->size ) ? ((SetStorage16 *) this->listp)->GOTO : \
                              ((SetStorage32 *) this->listp)->GOTO )
  int sverify( void ) const {
    int x = this->lverify();
    if ( x == 0 )
      x = SET_CALL( sverify( *this ) );
    return x;
  }
  SetStatus sismember( const void *key,  size_t keylen,  HashPos &pos ) {
    return SET_CALL( sismember( *this, key, keylen, pos ) );
  }
  SetStatus sadd( const void *key,  size_t keylen ) {
    HashPos pos;
    pos.init( key, keylen );
    return this->sadd( key, keylen, pos );
  }
  SetStatus sadd( const void *key,  size_t keylen,  HashPos &pos ) {
    return SET_CALL( sadd( *this, key, keylen, pos ) );
  }
  SetStatus srem( const void *key,  size_t keylen ) {
    HashPos pos;
    pos.init( key, keylen );
    return this->srem( key, keylen, pos );
  }
  SetStatus srem( const void *key,  size_t keylen,  HashPos &pos ) {
    return SET_CALL( srem( *this, key, keylen, pos ) );
  }
  SetStatus spopn( size_t n ) {
    return SET_CALL( spopn( *this, n ) );
  }
  SetStatus spopall( void ) {
    return SET_CALL( spopall( *this ) );
  }
  template<class T, class U>
  SetStatus sunion( SetData &set,  MergeCtx &ctx ) {
    if ( is_uint8( this->size ) )
      return ((SetStorage8 *) this->listp)->sunion<T, U>( *this, set,
                                        *(SetStorage<T, U> *) set.listp, ctx );
    else if ( is_uint16( this->size ) )
      return ((SetStorage16 *) this->listp)->sunion<T, U>( *this, set,
                                        *(SetStorage<T, U> *) set.listp, ctx );
    return ((SetStorage32 *) this->listp)->sunion<T, U>( *this, set,
                                        *(SetStorage<T, U> *) set.listp, ctx );
  }
  template<class T, class U>
  SetStatus sinter( SetData &set,  MergeCtx &ctx,  bool is_intersect ) {
    if ( is_uint8( this->size ) )
      return ((SetStorage8 *) this->listp)->sinter<T, U>( *this, set,
                          *(SetStorage<T, U> *) set.listp, ctx, is_intersect );
    else if ( is_uint16( this->size ) )
      return ((SetStorage16 *) this->listp)->sinter<T, U>( *this, set,
                          *(SetStorage<T, U> *) set.listp, ctx, is_intersect );
    return ((SetStorage32 *) this->listp)->sinter<T, U>( *this, set,
                          *(SetStorage<T, U> *) set.listp, ctx, is_intersect );
  }
  SetStatus sunion( SetData &set,  MergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->sunion<uint16_t, uint8_t>( set, ctx );
    else if ( is_uint16( set.size ) )
      return this->sunion<uint32_t, uint16_t>( set, ctx );
    return this->sunion<uint64_t, uint32_t>( set, ctx );
  }
  SetStatus sinter( SetData &set,  MergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->sinter<uint16_t, uint8_t>( set, ctx, true );
    else if ( is_uint16( set.size ) )
      return this->sinter<uint32_t, uint16_t>( set, ctx, true );
    return this->sinter<uint64_t, uint32_t>( set, ctx, true );
  }
  SetStatus sdiff( SetData &set,  MergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->sinter<uint16_t, uint8_t>( set, ctx, false );
    else if ( is_uint16( set.size ) )
      return this->sinter<uint32_t, uint16_t>( set, ctx, false );
    return this->sinter<uint64_t, uint32_t>( set, ctx, false );
  }
};

struct SetMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }

  SetMsg( void *bb,  size_t off,  size_t len,  MDDict *d,  MDMsgMem &m )
    : MDMsg( bb, off, len, d, m ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;

  static bool is_setmsg( void *bb,  size_t off,  size_t len,
                         uint32_t h ) noexcept;
  static MDMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                        MDDict *d,  MDMsgMem &m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

}
}

#endif
#endif
