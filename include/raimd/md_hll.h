#ifndef __rai_raimd__md_hll_h__
#define __rai_raimd__md_hll_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

/* precompute log tables, they are used for the estimations */
void hll_ginit( uint32_t htsz, uint32_t lzsz, double *lz_sum,
                double *ht_beta, double *ht_lib );

template <const uint32_t HT_BITS> /* 14b = 12k ht sz using 6 bit lz per entry */
struct HyperLogLogT {
  static const uint32_t LZ_BITS    = 6, /* bitsize for leading zeros register */
                        HASH_WIDTH = 64 - HT_BITS; /* word size of LZ calc */
  static const uint16_t LZSZ       = 1 << LZ_BITS,
                        LZ_MASK    = LZSZ - 1;
  static const uint64_t HASH_MASK  = ( (uint64_t) 1 << HASH_WIDTH ) - 1;
  static const uint32_t HTSZ       = 1 << HT_BITS, /* num registers */
                        HT_MASK    = HTSZ - 1;
  /* precalculated tables for estimates */
  static double lz_sum[ LZSZ ];   /* table for hyperloglog: 1/1<<LZ */
  static double ht_beta[ HTSZ ];  /* table for hyperloglog: log(ez) */
  static double ht_lin[ HTSZ ];   /* table for linear counting -m*log(n) */

  /* hdr for 14b / 6b = 0x60e0602 = 0x602(1538) + 14(htbits) + 6(lzbits)  */
  uint16_t size8;     /* sizeof this / 8 */
  uint8_t  ht_bits,   /* HT_BITS */
           lz_bits;   /* LZ_BITS */
  uint32_t popcnt; /* how many ht[] entries are used */
  double   sum;    /* sum of lz_sum[ n ] or 1 / ( 1 << ht[ n ] ) */
  uint8_t  ht[ ( HTSZ * LZ_BITS + 7 ) / 8 ]; /* ht % HTSZ = LZ (registers) */

  size_t size( void ) const {
    return this->size8 * 8;
  }
  /* zero stuff, init tables if necessary */
  void init( void ) {
    ginit();
    this->size8   = sizeof( *this ) / 8; /* when <= 1<<18 */
    this->ht_bits = HT_BITS;
    this->lz_bits = LZ_BITS;
    this->popcnt  = 0;
    this->sum     = 0;
    ::memset( this->ht, 0, sizeof( this->ht ) );
  }
  /* init log tables if not already done */
  static void ginit( void ) {
    if ( lz_sum[ 1 ] == 0.0 )
      hll_ginit( HTSZ, LZSZ, lz_sum, ht_beta, ht_lin );
  }
  static inline uint16_t get_u16( const void *p ) {
    uint16_t i; ::memcpy( &i, p, sizeof( i ) ); return i;
  }
  static inline void put_u16( uint16_t i,  void *p ) {
    ::memcpy( p, &i, sizeof( i ) );
  }
  /* return 1 if added, 0 if not added */
  int add( uint64_t h ) {
    uint64_t lzwd = ( h & HASH_MASK ) >> HT_BITS; /* the upper bits of hash */
    uint32_t reg  = ( h & HT_MASK ) * LZ_BITS, /* register bit position */
             off  = reg / 8,                   /* 8 bit address and shift */
             shft = reg % 8;
    uint16_t hv = get_u16( &this->ht[ off ] );
    uint16_t v, w, m, lz;

    v  = hv;                      /* the 6 bit reg val plus surrounding bits */
    m  = ~( LZ_MASK << shft ) & v; /* m = surrounding bits, reg masked to 0 */
    v  = ( v >> shft ) & LZ_MASK;  /* extract the value from surrounding */
    lz = __builtin_clzl( lzwd ) + 1; /* the new value */
    lz -= 64 + HT_BITS - HASH_WIDTH; /* minus the hash table bits */
    w  = ( v > lz ? v : lz );      /* maximum of old and new */
    if ( v == w )                  /* no change */
      return 0;
    hv = m | ( w << shft );        /* put the value back into bits blob */
    put_u16( hv, &this->ht[ off ] );
    if ( v == 0 )                  /* new entry */
      this->popcnt++;
    this->sum += lz_sum[ w ] - lz_sum[ v ]; /* remove old and add new */
    return 1;
  }
  /* return true if hash exists in hll */
  bool test( uint64_t h ) const {
    uint64_t lzwd = ( h & HASH_MASK ) >> HT_BITS; /* the upper bits of hash */
    uint32_t reg  = ( h & HT_MASK ) * LZ_BITS, /* register bit position */
             off  = reg / 8,                   /* 8 bit address and shift */
             shft = reg % 8;
    uint16_t hv   = get_u16( &this->ht[ off ] );
    uint16_t v, lz;

    v  = ( hv >> shft ) & LZ_MASK; /* extract the value from surrounding */
    lz = __builtin_clzl( lzwd ) + 1; /* the new value */
    lz -= 64 + HT_BITS - HASH_WIDTH; /* minus the hash table bits */
    return v >= lz;
  }
  /* alpha from http://algo.inria.fr/flajolet/Publications/FlFuGaMe07.pdf */
  static inline double alpha( void ) {
    return 0.7213 / ( 1.0 + 1.079 / (double) HTSZ );
  }
  bool is_linear( void ) const {
    /* linear counting threshold 95% full -- not studied deeply, 
     * I chose it because this is where linear hash tables are degrading fast */
    return ( this->popcnt < HTSZ * 950 / 1000 );
  }
  /* return the current estimate, islin = is linear counting */
  double estimate( void ) const {
    if ( this->is_linear() )
      return ht_lin[ this->popcnt ];

    uint32_t empty = HTSZ - this->popcnt;
    double   sum2  = this->sum + (double) empty; /* 1/1<<0 == 1 */
    return alpha() * (double) HTSZ * (double) this->popcnt /
           ( ht_beta[ empty ] + sum2 );
  }
  static inline uint64_t get_u64( const void *p ) {
    uint64_t i; ::memcpy( &i, p, sizeof( i ) ); return i;
  }
  /* merge hll into this */
  void merge( const HyperLogLogT &hll ) {
    uint32_t bitsleft = 0;
    uint64_t hbits = 0, mbits = 0;
    uint16_t x, y;
    /* reg is the bit offset of each register */
    for ( uint32_t reg = 0; reg < HTSZ * LZ_BITS; reg += LZ_BITS ) {
      const uint32_t off  = reg / 8,
                     shft = reg % 8;
      if ( bitsleft < LZ_BITS ) {  /* 64 bit shift words, little endian style */
        hbits = get_u64( &hll.ht[ off ] );
        mbits = get_u64( &this->ht[ off ] );
        hbits >>= shft;
        mbits >>= shft;
        bitsleft = 64 - shft;
      }
      x = hbits & LZ_MASK; /* 6 bit register at reg offset */
      y = mbits & LZ_MASK;
      if ( y < x ) {       /* if merge is larger, take that value */
        if ( y == 0 )      /* new entry */
          this->popcnt++;
        this->sum += lz_sum[ x ] - lz_sum[ y ]; /* update sum */
        /* splice in the new bits */
        uint16_t hv = get_u16( &this->ht[ off ] );
        hv = ( ~( LZ_MASK << shft ) & hv ) | (uint16_t) ( x << shft );
        put_u16( hv, &this->ht[ off ] );
      }
      hbits >>= LZ_BITS; /* next register */
      mbits >>= LZ_BITS;
      bitsleft -= LZ_BITS;
    }
  }
};

/* HTSZ (P) of 14 is the most commonly used and studied */
typedef HyperLogLogT<14> HyperLogLog;

struct HLLMsg : public MDMsg {
  void * operator new( size_t, void *ptr ) { return ptr; }

  HLLMsg( void *bb,  size_t off,  size_t len,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, len, d, m ) {}

  virtual const char *get_proto_string( void ) final;
  virtual uint32_t get_type_id( void ) final;
  virtual int get_reference( MDReference &mref ) final;

  static bool is_hllmsg( void *bb,  size_t off,  size_t len,  uint32_t h );
  static MDMsg *unpack( void *bb,  size_t off,  size_t len,  uint32_t h,
                        MDDict *d,  MDMsgMem *m );
  static void init_auto_unpack( void );
};

}
}

#endif
