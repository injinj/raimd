#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <raimd/json_msg.h>

using namespace rai;
using namespace md;

int
main( int argc, char **argv )
{
  if ( argc > 1 && ::strcmp( argv[ 1 ], "-h" ) == 0 ) {
    fprintf( stderr,
      "Test using MDMsg unpacking and arbitrary json\n" );
    return 1;
  }

  const char *sinput[] = { "{ \"one\" : 1.001, "
                           "  \"two\" : 2.002, "
                           "  \"sub\" : { \"hello\" : \"world this is a test\", "
                                       "  \"is_set\" : true, "
                                       "  \"is_wrong\" : null }, "
                           "  \"test\" : [1,\"two\",3], "
                           "  \"check\" : \"mate\" }",
                           "\"hello world\"",
                           "1.1234"
                         };
  MDOutput mout;
  MDMsgMem mem;
  MDReference mref;

  for ( size_t i = 0; i < sizeof( sinput ) / sizeof( sinput[ 0 ] ); i++ ) {
    JsonMsg * m = JsonMsg::unpack_any( (void *) sinput[ i ], 0,
                                       ::strlen( sinput[ i ] ), 0,
                                       NULL, &mem );
    printf( "sinput[%" PRIu64 "] = \n", i );
    if ( m != NULL ) {
      m->print( &mout );
      MDFieldIter *f;
      if ( m->get_field_iter( f ) == 0 ) {
        if ( f->find( "test", 5, mref ) == 0 ) {
          printf( "found test = \n" );
          f->print( &mout );
        }
      }
    }
    /*printf( "reuse %u\n", mem.mem_off );*/
    mem.reuse();
  }
  return 0;
}

