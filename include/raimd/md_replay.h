#ifndef __rai_raimd__md_replay_h__
#define __rai_raimd__md_replay_h__

#include <raimd/md_msg.h>

#ifdef __cplusplus
extern "C" {
#endif
  
typedef struct MDReplay_s {
  MDMsgMem_t mem;
  char     * buf,
           * subj,
           * msgbuf;
  size_t     bufoff,
             buflen,
             bufsz,
             subjlen,
             msglen;
  void     * input;
} MDReplay_t;

bool md_replay_create( MDReplay_t **r,  void *inp );
void md_replay_destroy( MDReplay_t *r );
void md_replay_init( MDReplay_t *r,  void *inp );
bool md_replay_open( MDReplay_t *r,  const char *fname );
void md_replay_close( MDReplay_t *r );
void md_replay_resize( MDReplay_t *r,  size_t sz );
bool md_replay_fillbuf( MDReplay_t *r,  size_t need_bytes );
bool md_replay_first( MDReplay_t *r );
bool md_replay_next( MDReplay_t *r );
bool md_replay_parse( MDReplay_t *r );
#ifdef __cplusplus
}   
namespace rai {
namespace md {

struct MDReplay : public MDReplay_s {

  void * operator new( size_t, void *ptr ) { return ptr; }
  void operator delete( void *ptr ) { ::free( ptr ); }
  MDReplay( void *inp = NULL ) {
    this->init( inp );
  }
  ~MDReplay() noexcept;
  void init( void *inp ) {
    md_msg_mem_init( &this->mem );
    this->buf     = NULL;
    this->subj    = NULL;
    this->msgbuf  = NULL;
    this->buflen  = 0;
    this->bufsz   = 0;
    this->subjlen = 0;
    this->msglen  = 0;
    this->input   = inp;
  }
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
#endif
