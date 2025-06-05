#ifndef __rai_raimd__hex_dump_h__
#define __rai_raimd__hex_dump_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
namespace rai {
namespace md {

struct MDHexDump {
  char line[ 80 ];
  uint32_t boff, hex, ascii;
  uint64_t stream_off;

  MDHexDump() : boff( 0 ), stream_off( 0 ) {
    this->flush_line();
  }
  void reset( void ) {
    this->boff = 0;
    this->stream_off = 0;
    this->flush_line();
  }
  void flush_line( void ) {
    this->stream_off += this->boff;
    this->boff  = 0;
    this->hex   = 9;
    this->ascii = 61;
    this->init_line();
  }
  static char hex_char( uint8_t x ) {
    return x < 10 ? ( '0' + x ) : ( 'a' - 10 + x );
  }
  void init_line( void ) {
    uint64_t j, k = this->stream_off;
    ::memset( this->line, ' ', 79 );
    this->line[ 79 ] = '\0';
    this->line[ 5 ] = hex_char( k & 0xf );
    k >>= 4; j = 4;
    while ( k > 0 ) {
      this->line[ j ] = hex_char( k & 0xf );
      if ( j-- == 0 )
        break;
      k >>= 4;
    }
  }
  uint32_t fill_line( const void *ptr,  size_t off,  size_t len ) {
    while ( off < len && this->boff < 16 ) {
      uint8_t b = ((uint8_t *) ptr)[ off++ ];
      this->line[ this->hex ]   = hex_char( b >> 4 );
      this->line[ this->hex+1 ] = hex_char( b & 0xf );
      this->hex += 3;
      if ( b >= ' ' && b <= 127 )
        line[ this->ascii ] = b;
      this->ascii++;
      if ( ( ++this->boff & 0x3 ) == 0 )
        this->hex++;
    }
    return (uint32_t) off;
  }
  static void print_hex( const void *buf,  size_t buflen ) noexcept;
};

}
}

#endif
#endif
