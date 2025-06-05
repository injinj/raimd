#ifndef __rai_raimd__cfile_h__
#define __rai_raimd__cfile_h__

#include <raimd/md_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}

namespace rai {
namespace md {

/*
FIELD_NAME
{
        CLASS_ID        12345; # FID, range 1 -> 16383
        IS_PRIMITIVE    true;  # true if field, false if form
        IS_FIXED        true;  # fixed size always
        DATA_SIZE       6;     # size of field data
        DATA_TYPE       2;     # TSS_type
}
*/
enum TSS_type {
  TSS_NODATA     =  0,
  TSS_INTEGER    =  1,
  TSS_STRING     =  2,
  TSS_BOOLEAN    =  3,
  TSS_DATE       =  4,
  TSS_TIME       =  5,
  TSS_PRICE      =  6,
  TSS_BYTE       =  7,
  TSS_FLOAT      =  8,
  TSS_SHORT_INT  =  9,
  TSS_DOUBLE     = 10,
  TSS_OPAQUE     = 11,
  TSS_NULL       = 12,
  TSS_RESERVED   = 13,
  TSS_DOUBLE_INT = 14,
  TSS_GROCERY    = 15,
  TSS_SDATE      = 16,
  TSS_STIME      = 17,
  TSS_LONG       = 18,
  TSS_U_SHORT    = 19,
  TSS_U_INT      = 20,
  TSS_U_LONG     = 21
};
const char * tss_type_str( TSS_type tp ) noexcept;

enum CFileTok {
  CFT_ERROR            = -2,
  CFT_EOF              = -1,
  CFT_IDENT            = 0,
  CFT_INT              = 1,
  CFT_TRUE             = 2,
  CFT_FALSE            = 3,
  CFT_FIELDS           = 4,
  CFT_FIELD_CLASS_NAME = 5,
  CFT_CF_INCLUDES      = 6,
  CFT_OPEN_BR          = 7,
  CFT_CLOSE_BR         = 8,
  CFT_SEMI             = 9,
  CFT_CLASS_ID         = 10,
  CFT_IS_PRIMITIVE     = 11,
  CFT_IS_FIXED         = 12,
  CFT_IS_PARTIAL       = 13,
  CFT_DATA_SIZE        = 14,
  CFT_DATA_TYPE        = 15
};

struct CFileKeyword {
  const char * str;
  size_t       len;
  CFileTok     tok;
};

struct CFRecField {
  CFRecField * next;
  char fname[ 256 ],
       classname[ 256 ];
  
  void * operator new( size_t, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  CFRecField( const char *fn, size_t sz ) {
    this->next = NULL;
    this->fname[ 0 ] = this->classname[ 0 ] = '\0';
    this->set( this->fname, fn, sz );
  }
  void set_class( const char *cn,  size_t sz ) {
    this->set( this->classname, cn, sz );
  }
  void set( char *nm,  const char *fn,  size_t sz ) {
    if ( sz > 255 )
      sz = 255;
    ::memcpy( nm, fn, sz );
    nm[ sz ] = '\0';
  }
};

struct MDDictBuild;
struct MDMsg;
struct CFile : public DictParser {
  CFileTok stmt;
  bool     cf_includes;
  uint8_t  is_primitive,
           is_fixed,
           is_partial;
  int      data_size,
           data_type,
           class_id,
           ident_lineno;
  char     ident[ 256 ];
  MDQueue< CFRecField > fld;

  void * operator new( size_t, void *ptr ) { return ptr; } 
  void operator delete( void *ptr ) { ::free( ptr ); } 

  CFile( CFile *n,  const char *p,  int debug_flags )
    : DictParser( p, CFT_INT, CFT_IDENT, CFT_ERROR, debug_flags, "TIB Cfile" ),
      stmt( CFT_ERROR ), cf_includes( false ),
      is_primitive( 0 ), is_fixed( 0 ), is_partial( 0 ),
      data_size( 0 ), data_type( 0 ), class_id( 0 ),
      ident_lineno( 1 ) {
    this->next = n;
    this->ident[ 0 ] = '\0';
  }
  ~CFile() {
    this->clear_ident();
    this->close();
  }
  void clear_ident( void ) noexcept;
  void set_ident( void ) noexcept;
  void add_field( void ) noexcept;
  void set_field_class_name( void ) noexcept;
  CFileTok consume( CFileTok k,  size_t sz ) {
    return (CFileTok) this->DictParser::consume_tok( k, sz );
  }
  CFileTok consume( CFileKeyword &kw ) {
    return this->consume( kw.tok, kw.len );
  }
  CFileTok consume_int( void ) {
    return (CFileTok) this->DictParser::consume_int_tok();
  }
  CFileTok consume_ident( void ) {
    return (CFileTok) this->DictParser::consume_ident_tok();
  }
  CFileTok consume_string( void ) {
    return (CFileTok) this->DictParser::consume_string_tok();
  }
  CFileTok get_token( void ) noexcept;
  bool match( CFileKeyword &kw ) {
    return this->DictParser::match( kw.str, kw.len );
  }
  static CFile * push_path( CFile *tos,  const char *path,
                            const char *filename,  size_t file_sz,
                            int debug_flags ) noexcept;
  static int parse_path( MDDictBuild &dict_build,  const char *path,
                         const char *fn ) noexcept;
  static int parse_string( MDDictBuild &dict_build,  const char *str_input,
                           size_t str_size ) noexcept;
  static int parse_loop( MDDictBuild &dict_build,  CFile *p,
                         const char *path ) noexcept;
  static int unpack_sass( MDDictBuild &dict_build,  MDMsg *m ) noexcept;
};

}
}
#endif
#endif

