#ifndef __rai_raimd__md_replay_h__
#define __rai_raimd__md_replay_h__

#include <raimd/md_msg.h>

namespace rai {
namespace md {

struct MDReplay {
  MDMsgMem mem;
  char   * buf,
         * subj,
         * msgbuf;
  size_t   bufoff,
           buflen,
           bufsz,
           subjlen,
           msglen;
  void   * input;

  MDReplay( void *inp = NULL )
    : buf( NULL ), subj( NULL ), msgbuf( NULL ),
      buflen( 0 ), bufsz( 0 ), subjlen( 0 ), msglen( 0 ), input( inp ) {}
  bool open( const char *fname ) noexcept;
  void close( void ) noexcept;
  void resize( size_t sz ) noexcept;
  bool fillbuf( size_t need_bytes ) noexcept;
  bool first( void ) noexcept;
  bool next( void ) noexcept;
  bool parse( void ) noexcept;
};

}
}

#endif
