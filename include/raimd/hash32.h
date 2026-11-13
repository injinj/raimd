#ifndef __rai_raimd__hash32_h__
#define __rai_raimd__hash32_h__

/* CRC32C (Castagnoli) hardware paths. x86 SSE4.2 _mm_crc32_u* and ARM's
 * ACLE __crc32c* intrinsics implement the same polynomial, so hashes stay
 * bit-for-bit identical across architectures (important: shared memory /
 * wire structures may be produced on one arch and read on another). */
#if defined( __x86_64__ ) || defined( __i386__ )
#  include <immintrin.h>
#  define MD_HASH32_CRC8(h,v)  _mm_crc32_u8( (h), (v) )
#  define MD_HASH32_CRC16(h,v) _mm_crc32_u16( (h), (v) )
#  define MD_HASH32_CRC32(h,v) _mm_crc32_u32( (h), (v) )
#  define MD_HASH32_CRC64(h,v) ( (uint32_t) _mm_crc32_u64( (h), (v) ) )
#elif defined( __aarch64__ ) || defined( __arm__ )
#  include <arm_acle.h>
#  define MD_HASH32_CRC8(h,v)  __crc32cb( (h), (v) )
#  define MD_HASH32_CRC16(h,v) __crc32ch( (h), (v) )
#  define MD_HASH32_CRC32(h,v) __crc32cw( (h), (v) )
#  define MD_HASH32_CRC64(h,v) __crc32cd( (h), (v) )
#else
#  error "md_hash32: no CRC32C intrinsic path for this architecture (add one in include/raimd/hash32.h)"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t
md_hash32( const void *p,  size_t sz,  uint32_t seed )
{
#define u16 ((uint16_t *) (void *) u8)
#define u32 ((uint32_t *) (void *) u8)
#define u64 ((uint64_t *) (void *) u8)
  const uint8_t * u8  = (uint8_t *) p;
  const uint8_t * end = &u8[ sz ];
  uint32_t        h   = seed;

  size_t adj = 8 - (size_t) ( (intptr_t) u8 & 7 );
  for ( adj = ( adj > sz ? sz : adj ) & 7; adj > 0; adj-- )
    h = MD_HASH32_CRC8( h, *u8++ );

  for (;;) {
    switch ( end - u8 ) {
      default: h  = MD_HASH32_CRC64( h, u64[ 0 ] ); u8 = &u8[ 8 ];
               break;
      case 7: return MD_HASH32_CRC8(
                       MD_HASH32_CRC16(
                       MD_HASH32_CRC32( h, u32[ 0 ] ), u16[ 2 ] ), u8[ 6 ] );
      case 6: return MD_HASH32_CRC16(
                       MD_HASH32_CRC32( h, u32[ 0 ] ), u16[ 2 ] );
      case 5: return MD_HASH32_CRC8( MD_HASH32_CRC32( h, u32[ 0 ] ), u8[ 4 ] );
      case 4: return MD_HASH32_CRC32( h, u32[ 0 ] );
      case 3: return MD_HASH32_CRC8( MD_HASH32_CRC16( h, u16[ 0 ] ), u8[ 2 ] );
      case 2: return MD_HASH32_CRC16( h, u16[ 0 ] );
      case 1: return MD_HASH32_CRC8( h, u8[ 0 ] );
      case 0: return h;
    }
  }
#undef u16
#undef u32
#undef u64
}

static inline uint32_t
md_hash_u32( uint32_t i )
{
  return MD_HASH32_CRC32( 0, i );
}

#ifdef __cplusplus
}   
namespace rai {
namespace md {

/* XXX replace with something else on non-x86 */
static inline uint32_t
hash32( const void *p,  size_t sz,  uint32_t seed )
{
  return md_hash32( p, sz, seed );
}

static inline uint32_t
hash_u32( uint32_t i )
{
  return md_hash_u32( i );
}

}
}
#endif
#endif
