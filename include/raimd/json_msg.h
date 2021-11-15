#ifndef __rai_raimd__json_msg_h__
#define __rai_raimd__json_msg_h__

#include <raimd/md_msg.h>
#include <raimd/json.h>

namespace rai {
namespace md {

struct JsonParser;
struct JsonObject;

static const uint32_t JSON_TYPE_ID = 0x4a014cc2;

struct JsonMsg : public MDMsg {
  /* used by unpack() to alloc in MDMsgMem */
  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonValue * js; /* any json atom:  object, string, array, number */

  JsonMsg( void *bb,  size_t off,  size_t end,  MDDict *d,  MDMsgMem *m )
    : MDMsg( bb, off, end, d, m ), js( 0 ) {}

  virtual const char *get_proto_string( void ) noexcept final;
  virtual uint32_t get_type_id( void ) noexcept final;
  virtual int get_sub_msg( MDReference &mref, MDMsg *&msg ) noexcept final;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int get_field_iter( MDFieldIter *&iter ) noexcept final;
  virtual int get_array_ref( MDReference &mref, size_t i,
                             MDReference &aref ) noexcept final;
  static int value_to_ref( MDReference &mref,  JsonValue &x ) noexcept;
  static bool is_jsonmsg( void *bb,  size_t off,  size_t end,
                          uint32_t h ) noexcept;
  /* unpack only objects delimited by '{' '}' */
  static JsonMsg *unpack( void *bb,  size_t off,  size_t end,  uint32_t h,
                          MDDict *d,  MDMsgMem *m ) noexcept;
  /* try to parse any json:  array, string, number */
  static JsonMsg *unpack_any( void *bb,  size_t off,  size_t end,  uint32_t h,
                              MDDict *d,  MDMsgMem *m ) noexcept;
  static void init_auto_unpack( void ) noexcept;
};

struct JsonMsgCtx {
  JsonMsg      * msg;
  JsonParser   * parser;
  JsonBufInput * input;
  MDMsgMem     * mem;
  JsonMsgCtx() : msg( 0 ), parser( 0 ), input( 0 ), mem( 0 ) {}
  ~JsonMsgCtx() noexcept;
  int parse( void *bb,  size_t off,  size_t end,  MDDict *d,
             MDMsgMem *m,  bool is_yaml ) noexcept;
  void release( void ) noexcept;
};

struct JsonFieldIter : public MDFieldIter {
  JsonMsg    & me;
  JsonObject & obj;

  void * operator new( size_t, void *ptr ) { return ptr; }

  JsonFieldIter( JsonMsg &m,  JsonObject &o ) : MDFieldIter( m ),
    me( m ), obj( o ) {}

  virtual int get_name( MDName &name ) noexcept final;
  virtual int copy_name( char *name,  size_t &name_len,  MDFid &fid ) noexcept;
  virtual int get_reference( MDReference &mref ) noexcept final;
  virtual int find( const char *name, size_t name_len,
                    MDReference &mref ) noexcept final;
  virtual int first( void ) noexcept final;
  virtual int next( void ) noexcept final;
};

}
}

#endif
