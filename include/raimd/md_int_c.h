#ifndef __rai_raimd__md_int_c_h__
#define __rai_raimd__md_int_c_h__

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t
get_u16_md_big( const void* data )
{
  const uint8_t * u = (const uint8_t *) data;
  return ( (uint16_t) u[ 0 ] << 8 ) | (uint16_t) u[ 1 ];
}

static inline uint16_t
get_u16_md_little( const void* data )
{
  const uint8_t * u = (const uint8_t *) data;
  return ( (uint16_t) u[ 1 ] << 8 ) | (uint16_t) u[ 0 ];
}

static inline void
set_u16_md_big( void* data, uint16_t value )
{
  uint8_t * u = (uint8_t*) data;
  u[ 0 ] = (uint8_t) ( value >> 8 );
  u[ 1 ] = (uint8_t) value;
}

static inline void
set_u16_md_little( void* data, uint16_t value )
{
  uint8_t* u = (uint8_t*) data;
  u[ 0 ] = (uint8_t) value;
  u[ 1 ] = (uint8_t) ( value >> 8 );
}

static inline uint32_t
get_u32_md_big( const void* data )
{
  const uint8_t * u = (const uint8_t *) data;
  return ( (uint32_t) u[ 0 ] << 24 ) | ( (uint32_t) u[ 1 ] << 16 ) |
         ( (uint32_t) u[ 2 ] << 8 ) | (uint32_t) u[ 3 ];
}

static inline uint32_t
get_u32_md_little( const void* data )
{
  const uint8_t * u = (const uint8_t *) data;
  return ( (uint32_t) u[ 3 ] << 24 ) | ( (uint32_t) u[ 2 ] << 16 ) |
         ( (uint32_t) u[ 1 ] << 8 ) | (uint32_t) u[ 0 ];
}

static inline void
set_u32_md_big( void* data, uint32_t value )
{
  uint8_t * u = (uint8_t *) data;
  u[ 0 ] = (uint8_t) ( value >> 24 );
  u[ 1 ] = (uint8_t) ( value >> 16 );
  u[ 2 ] = (uint8_t) ( value >> 8 );
  u[ 3 ] = (uint8_t) value;
}

static inline void
set_u32_md_little( void* data, uint32_t value )
{
  uint8_t * u = (uint8_t *) data;
  u[ 0 ] = (uint8_t) value;
  u[ 1 ] = (uint8_t) ( value >> 8 );
  u[ 2 ] = (uint8_t) ( value >> 16 );
  u[ 3 ] = (uint8_t) ( value >> 24 );
}

static inline uint64_t
get_u64_md_big( const void* data )
{
  return (uint64_t) get_u32_md_big( &((const uint8_t *) data)[ 4 ] ) |
       ( (uint64_t) get_u32_md_big( data ) ) << 32;
}

static inline uint64_t
get_u64_md_little( const void* data )
{
  return (uint64_t) get_u32_md_little( data ) |
       ( (uint64_t) get_u32_md_little( &((const uint8_t *) data)[ 4 ] ) ) << 32;
}

static inline void
set_u64_md_big( void* data, uint64_t value )
{
  set_u32_md_big( data, (uint32_t) ( value >> 32 ) );
  set_u32_md_big( &((uint8_t *) data)[ 4 ], (uint32_t) value );
}

static inline void
set_u64_md_little( void* data, uint64_t value )
{
  set_u32_md_little( data, (uint32_t) value );
  set_u32_md_little( &((uint8_t *) data)[ 4 ], (uint32_t) ( value >> 32 ) );
}

/* Helper function to determine system endianness */
static inline MDEndian
md_get_endian( void )
{
  uint16_t test = 1;
  /* If the lowest byte is 1, this system is little endian */
  return *(uint8_t*) &test == 1 ? MD_LITTLE : MD_BIG;
}

#ifdef __cplusplus
}
#endif
#endif
