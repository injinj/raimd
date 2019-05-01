#ifndef __rai_raimd__md_field_iter_h__
#define __rai_raimd__md_field_iter_h__

#include <raimd/md_types.h>

namespace rai {
namespace md {

struct MDFieldIter { /* generic field iterator */
  MDMsg & iter_msg;
  size_t  field_start,   /* marks the area of a field, if msg is byte blob */
          field_end;

  MDFieldIter( MDMsg &m )
    : iter_msg( m ), field_start( 0 ), field_end( 0 ) {}

  /* these fuctions implemented in subclass */
  virtual int get_name( MDName &name );
  virtual int get_reference( MDReference &mref );
  virtual int get_hint_reference( MDReference &mref );
  virtual int get_enum( MDReference &mref, MDEnum &enu );
  virtual int find( const char *name );
  virtual int first( void );
  virtual int next( void );

  /* escaped strings is used for json escaping */
  int print( MDOutput *out, int indent_newline,
                     const char *fname_fmt,  const char *type_fmt );
  int print( MDOutput *out ) {
    return this->print( out, 1, "%-18s : " /* fname fmt */,
                                "%-10s %3d : " /* type fmt */);
  }
};

}
} // namespace rai

#endif
