#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <raimd/md_hll.h>

namespace rai {
namespace md {

/*
template<> double HyperLogLogT<14>::lz_sum[] = {};
template<> double HyperLogLogT<14>::ht_lin[] = {};
template<> double HyperLogLogT<14>::ht_beta[] = {};
*/

double HyperLogLog::lz_sum[] = {};
double HyperLogLog::ht_lin[] = {};
double HyperLogLog::ht_beta[] = {};

}
}

using namespace rai;
using namespace md;

const char *
HLLMsg::get_proto_string( void ) noexcept
{
  return "MD_HYPERLOGLOG";
}

uint32_t
HLLMsg::get_type_id( void ) noexcept
{
  return MD_HYPERLOGLOG;
}

static MDMatch hllmsg_match = {
  .off         = 0,
  .len         = 4, /* cnt of buf[] */
  .hint_size   = 0, /* cnt of hint[] */
  .ftype       = MD_HYPERLOGLOG,
  .buf         = { 0x02, 0x06, 0x0e, 0x06 },
  .hint        = { 0 },
  .is_msg_type = HLLMsg::is_hllmsg,
  .unpack      = HLLMsg::unpack
};

bool
HLLMsg::is_hllmsg( void *bb,  size_t off,  size_t end,  uint32_t ) noexcept
{
  uint8_t * buf = &((uint8_t *) bb)[ off ];
  size_t    len = end - off;
  if ( len == 0x602 * 8 && get_u32<MD_LITTLE>( buf ) == 0x60e0602U )
    return true;
  return false;
}

MDMsg *
HLLMsg::unpack( void *bb,  size_t off,  size_t end,  uint32_t h,  MDDict *d,
                MDMsgMem *m ) noexcept
{
  if ( ! is_hllmsg( bb, off, end, h ) )
    return NULL;
#ifdef MD_REF_COUNT
  if ( m->ref_cnt != MDMsgMem::NO_REF_COUNT )
    m->ref_cnt++;
#endif
  /* check if another message is the first opaque field of the HLLMsg */
  void * ptr;
  m->alloc( sizeof( HLLMsg ), &ptr );
  return new ( ptr ) HLLMsg( bb, off, end, d, m );
}

void
HLLMsg::init_auto_unpack( void ) noexcept
{
  MDMsg::add_match( hllmsg_match );
}

int
HLLMsg::get_reference( MDReference &mref ) noexcept
{
  mref.zero();
  mref.ftype = MD_HYPERLOGLOG;
  mref.fptr  = (uint8_t *) this->msg_buf;
  mref.fptr  = &mref.fptr[ this->msg_off ];
  mref.fsize = this->msg_end - this->msg_off;
  return 0;
}

/* beta from https://arxiv.org/pdf/1612.02284.pdf */
static double
beta( double ez )
{
  double zl = log( ez + 1 );
  return -0.370393911 * ez +
          0.070471823 * zl +
          0.17393686  * pow( zl, 2 ) +
          0.16339839  * pow( zl, 3 ) +
         -0.09237745  * pow( zl, 4 ) +
          0.03738027  * pow( zl, 5 ) +
         -0.005384159 * pow( zl, 6 ) +
          0.00042419  * pow( zl, 7 );
}
void
rai::md::hll_ginit( uint32_t htsz,  uint32_t lzsz,  double *lz_sum,
                    double *ht_beta,  double *ht_lin ) noexcept
{
  uint32_t i;
  /* linear counting is an estimate of card based on how full ht[] is */
  for ( i = 1; i < htsz; i++ ) {
    double n = (double) ( htsz - i ) / (double) htsz;
    ht_lin[ i ] = -(double) htsz * log( n );
  }
  /* hyperloglog counting is based on the sum of 1/lz,
   * where lz is pow2 counting of leading zeros in the hash */
  for ( i = 1; i < lzsz; i++ )
    lz_sum[ i ] = 1.0 / (double) ( (uint64_t) 1 << i );
  for ( i = 1; i < htsz; i++ )
    ht_beta[ i ] = beta( (double) i );
}

