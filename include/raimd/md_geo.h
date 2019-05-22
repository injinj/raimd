#ifndef __rai_raimd__md_geo_h__
#define __rai_raimd__md_geo_h__

#include <raimd/md_zset.h>

namespace rai {
namespace md {

enum GeoStatus {
  GEO_OK        = ZSET_OK,
  GEO_NOT_FOUND = ZSET_NOT_FOUND,
  GEO_FULL      = ZSET_FULL,
  GEO_UPDATED   = ZSET_UPDATED,
  GEO_EXISTS    = ZSET_EXISTS,
  GEO_BAD       = ZSET_BAD
};

/* this would be H3Index with Uber H3 */
typedef uint64_t GeoIndx;
typedef ZSetStorage<uint16_t, uint8_t, GeoIndx>  GeoStorage8;
typedef ZSetStorage<uint32_t, uint16_t, GeoIndx> GeoStorage16;
typedef ZSetStorage<uint64_t, uint32_t, GeoIndx> GeoStorage32;
typedef ZSetValT<GeoIndx>   GeoVal;
typedef ZMergeCtxT<GeoIndx> GeoMergeCtx;

struct GeoData : public HashData {
  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }

  GeoData() : HashData() {}
  GeoData( void *l,  size_t sz ) : HashData( l, sz ) {}

  static const uint16_t geo8_sig  = 0xf7e8U;
  static const uint32_t geo16_sig = 0xddbe7ae8UL;
  static const uint64_t geo32_sig = 0xa5f5ff85c9f6c3e8ULL;
  void init( size_t count,  size_t data_len ) {
    this->init_sig( count, data_len, geo8_sig, geo16_sig, geo32_sig );
  }

#define GEO_CALL( GOTO ) \
  ( is_uint8( this->size ) ? ((GeoStorage8 *) this->listp)->GOTO : \
    is_uint16( this->size ) ? ((GeoStorage16 *) this->listp)->GOTO : \
                              ((GeoStorage32 *) this->listp)->GOTO )
  int gverify( void ) const {
    int x = this->lverify();
    if ( x == 0 )
      x = GEO_CALL( zverify( *this ) );
    return x;
  }

  GeoStatus geoexists( const char *key,  size_t keylen,  HashPos &pos,
                       GeoIndx &h3i ) {
    return (GeoStatus) GEO_CALL( zexists( *this, key, keylen, pos, h3i ) );
  }
  GeoStatus geoget( const char *key,  size_t keylen,  GeoVal &gv,
                    HashPos &pos ) {
    return (GeoStatus) GEO_CALL( zget( *this, key, keylen, gv, pos ) );
  }
  GeoStatus geoadd( const char *key,  size_t keylen,  GeoIndx h3i ) {
    HashPos pos;
    pos.init( key, keylen );
    return this->geoadd( key, keylen, h3i, pos );
  }
  GeoStatus geoadd( const char *key,  size_t keylen,  GeoIndx h3i,
                    HashPos &pos ) {
    return (GeoStatus)
      GEO_CALL( zadd( *this, key, keylen, h3i, pos, ZAGGREGATE_NONE, 0, NULL ));
  }
  GeoStatus georem( const char *key,  size_t keylen ) {
    HashPos pos;
    pos.init( key, keylen );
    return this->georem( key, keylen, pos );
  }
  GeoStatus georem( const char *key,  size_t keylen,  HashPos &pos ) {
    return (GeoStatus)
      GEO_CALL( zrem( *this, key, keylen, pos ) );
  }
  GeoStatus geoappend( const char *key,  size_t keylen,  GeoIndx h3i,
                       HashPos &pos ) {
    return (GeoStatus)
      GEO_CALL( zadd( *this, key, keylen, h3i, pos, ZAGGREGATE_NONE,
                      ZADD_DUP_KEY_OK, NULL ) );
  }
  GeoStatus geoindex( size_t n,  GeoVal &gv ) {
    return (GeoStatus) GEO_CALL( zindex( *this, n, gv ) );
  }
  GeoStatus geobsearch( GeoIndx h3i,  size_t &pos,  bool gt,  GeoIndx &h3j ) {
    return (GeoStatus) GEO_CALL( zbsearch( *this, h3i, pos, gt, h3j ) );
  }
  GeoStatus georem_index( size_t pos ) {
    return (GeoStatus) GEO_CALL( zrem_index( *this, pos ) );
  }
  GeoStatus georemall( void ) {
    return (GeoStatus) GEO_CALL( zremall( *this ) );
  }
  template<class T, class U, class Z>
  GeoStatus geounion( GeoData &set,  GeoMergeCtx &ctx ) {
    if ( is_uint8( this->size ) )
      return (GeoStatus) ((GeoStorage8 *) this->listp)->zunion<T, U, Z>(
        *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx );
    else if ( is_uint16( this->size ) )
      return (GeoStatus) ((GeoStorage16 *) this->listp)->zunion<T, U, Z>(
        *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx );
    return (GeoStatus) ((GeoStorage32 *) this->listp)->zunion<T, U, Z>(
      *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx );
  }
  template<class T, class U, class Z>
  GeoStatus geointer( GeoData &set,  GeoMergeCtx &ctx,  bool is_intersect ) {
    if ( is_uint8( this->size ) )
      return (GeoStatus) ((GeoStorage8 *) this->listp)->zinter<T, U, Z>(
        *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx, is_intersect );
    else if ( is_uint16( this->size ) )
      return (GeoStatus) ((GeoStorage16 *) this->listp)->zinter<T, U, Z>(
        *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx, is_intersect );
    return (GeoStatus) ((GeoStorage32 *) this->listp)->zinter<T, U, Z>(
      *this, set, *(ZSetStorage<T, U, Z> *) set.listp, ctx, is_intersect );
  }
  GeoStatus geounion( GeoData &set,  GeoMergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->geounion<uint16_t, uint8_t, GeoIndx>( set, ctx );
    else if ( is_uint16( set.size ) )
      return this->geounion<uint32_t, uint16_t, GeoIndx>( set, ctx );
    return this->geounion<uint64_t, uint32_t, GeoIndx>( set, ctx );
  }
  GeoStatus geointer( GeoData &set,  GeoMergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->geointer<uint16_t, uint8_t, GeoIndx>( set, ctx, true );
    else if ( is_uint16( set.size ) )
      return this->geointer<uint32_t, uint16_t, GeoIndx>( set, ctx, true );
    return this->geointer<uint64_t, uint32_t, GeoIndx>( set, ctx, true );
  }
  GeoStatus geodiff( GeoData &set,  GeoMergeCtx &ctx ) {
    if ( is_uint8( set.size ) )
      return this->geointer<uint16_t, uint8_t, GeoIndx>( set, ctx, false );
    else if ( is_uint16( set.size ) )
      return this->geointer<uint32_t, uint16_t, GeoIndx>( set, ctx, false );
    return this->geointer<uint64_t, uint32_t, GeoIndx>( set, ctx, false );
  }
};

struct GeoMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }

  GeoMsg( void *bb,  size_t off,  size_t len,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, len, d, m ) {}

  virtual const char *get_proto_string( void ) final;
  virtual int get_field_iter( MDFieldIter *&iter ) final;

  static bool is_geomsg( void *bb,  size_t off,  size_t len,  uint32_t h );
  static MDMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                        MDDict *d,  MDMsgMem *m );
  static void init_auto_unpack( void );
};

struct GeoFieldIter : public MDFieldIter {
  GeoData geo;
  char    key[ 48 ];
  size_t  keylen;
  GeoVal  val;
  void * operator new( size_t, void *ptr ) { return ptr; }
  GeoFieldIter( MDMsg &m ) : MDFieldIter( m ),
    geo( &((uint8_t *) m.msg_buf)[ m.msg_off ], m.msg_end - m.msg_off ) {
    this->geo.open();
    this->keylen = 0;
    this->val.zero();
  }
  virtual int get_name( MDName &name ) final;
  virtual int first( void ) final;
  virtual int next( void ) final;
  virtual int get_reference( MDReference &mref ) final;
};

}
}

#endif
