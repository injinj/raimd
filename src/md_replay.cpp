#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <raimd/md_replay.h>

using namespace rai;
using namespace md;

bool
MDReplay::open( const char *fname ) noexcept
{
  this->close();
  this->input = ::fopen( fname, "rb" );
  if ( this->input == NULL ) {
    perror( "fopen" );
    return false;
  }
  return true;
}

void
MDReplay::close( void ) noexcept
{
  if ( this->input != NULL ) {
    ::fclose( (FILE *) this->input );
    this->input = NULL;
  }
}

bool
MDReplay::first( void ) noexcept
{
  this->bufoff  = 0;
  this->buflen  = 0;
  this->subjlen = 0;
  this->msglen  = 0;
  this->msgbuf  = this->buf;
  this->resize( 2 * 1024 );
  return this->next();
}

void
MDReplay::resize( size_t sz ) noexcept
{
  size_t oldsz = this->bufsz;
  if ( oldsz == 0 || oldsz > sz ) {
    this->mem.reuse();
    this->buf = (char *) this->mem.make( sz );
  }
  else {
    this->mem.extend( oldsz, sz, &this->buf );
  }
  this->bufsz  = sz;
  this->subj   = this->buf;
  this->msgbuf = this->buf;
}

bool
MDReplay::next( void ) noexcept
{
  if ( this->input == NULL )
    return false;

  this->bufoff = &this->msgbuf[ this->msglen ] - this->buf;
  return this->parse();
}

bool
MDReplay::fillbuf( size_t need_bytes ) noexcept
{
  size_t left = this->bufsz - this->buflen;
  if ( left < need_bytes ) {
    char * start = &this->buf[ this->bufoff ];
    if ( start > this->buf ) {
      memmove( this->buf, start, this->buflen - this->bufoff );
      this->buflen -= this->bufoff;
      this->bufoff = 0;
      left = this->bufsz - this->buflen;
    }
    if ( left < need_bytes ) {
      size_t newsz = this->bufsz + need_bytes - left;
      this->resize( newsz );
      left = this->bufsz - this->buflen;
    }
  }
  for (;;) {
    FILE  * fp = (FILE *) this->input;
    ssize_t n;
    n = ::fread( &this->buf[ this->buflen ], 1, left, fp );
    if ( n <= 0 ) {
      if ( n < 0 )
        perror( "fread" );
      return false;
    }
    this->buflen += n;
    if ( (size_t) n >= need_bytes )
      break;
    need_bytes -= n;
    left -= n;
  }
  return true;
}

bool
MDReplay::parse( void ) noexcept
{
  size_t sz,
         len[ 2 ];
  char * ar[ 2 ],
       * s   = &this->buf[ this->bufoff ],
       * eol,
       * end = &this->buf[ this->buflen ];
  int    i   = 0;

  sz = end - s;
  for ( ; i < 2; i++ ) {
    while ( (eol = (char *) ::memchr( s, '\n', sz )) == NULL ) {
      if ( ! this->fillbuf( 1 ) )
        return false;
      s   = &this->buf[ this->bufoff ];
      end = &this->buf[ this->buflen ];
      sz  = end - s;
      i   = 0;
    }
    ar[ i ] = s;
    len[ i ] = eol - s;
    if ( len[ i ] > 0 && ar[ i ][ len[ i ] - 1 ] == '\r' )
      len[ i ]--;
    s  = eol + 1;
    sz = end - s;
  }

  long j = strtol( ar[ 1 ], &end, 0 );
  if ( end == ar[ 1 ] || j < 0 || j > 0x7fffffff )
    return false;

  if ( (size_t) j > sz ) {
    size_t need = (size_t) j - sz,
           off[ 2 ];
    off[ 0 ] = ar[ 0 ] - &this->buf[ this->bufoff ];
    off[ 1 ] = s - &this->buf[ this->bufoff ];

    if ( ! this->fillbuf( need ) )
      return false;

    ar[ 0 ] = &this->buf[ this->bufoff + off[ 0 ] ];
    s       = &this->buf[ this->bufoff + off[ 1 ] ];
  }
  this->subj    = ar[ 0 ];
  this->subjlen = len[ 0 ];
  this->msgbuf  = s;
  this->msglen  = j;
  return true;
}

