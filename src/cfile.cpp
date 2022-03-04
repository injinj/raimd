#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

static CFileKeyword
  true_tok             = { "true",              4, CFT_TRUE },
  false_tok            = { "false",             5, CFT_FALSE },
  fields_tok           = { "fields",            6, CFT_FIELDS },
  field_class_name_tok = { "field_class_name", 16, CFT_FIELD_CLASS_NAME },
  class_id_tok         = { "class_id",          8, CFT_CLASS_ID },
  cfile_includes_tok   = { "cfile_includes",   14, CFT_CF_INCLUDES },
  data_size_tok        = { "data_size",         9, CFT_DATA_SIZE },
  data_type_tok        = { "data_type",         9, CFT_DATA_TYPE },
  is_fixed_tok         = { "is_fixed",          8, CFT_IS_FIXED },
  is_partial_tok       = { "is_partial",       10, CFT_IS_PARTIAL },
  is_primitive_tok     = { "is_primitive",     12, CFT_IS_PRIMITIVE };

/*static const int EOF_CHAR = 256;*/

void
CFile::clear_ident( void ) noexcept
{
  this->stmt         = CFT_ERROR;
  this->is_primitive = this->is_fixed  = this->is_partial = 0;
  this->data_size    = this->data_type = this->class_id   = 0;
  this->ident[ 0 ]   = '\0';
  while ( ! this->fld.is_empty() ) {
    CFRecField * f = this->fld.pop();
    delete f;
  }
}

void
CFile::set_ident( void ) noexcept
{
  size_t len = ( this->tok_sz > 255 ? 255 : this->tok_sz );
  ::memcpy( this->ident, this->tok_buf, len );
  this->ident[ len ] = '\0';
  this->stmt         = CFT_IDENT;
  this->ident_lineno = this->lineno;
}

void
CFile::add_field( void ) noexcept
{
  this->fld.push_tl( 
    new ( ::malloc( sizeof( CFRecField ) ) )
      CFRecField( this->tok_buf, this->tok_sz ) );
}

void
CFile::set_field_class_name( void ) noexcept
{
  if ( this->fld.tl != NULL )
    this->fld.tl->set_class( this->tok_buf, this->tok_sz );
}

CFileTok
CFile::get_token( void ) noexcept
{
  for (;;) {
    switch ( this->eat_white() ) {
      case '{': return this->consume( CFT_OPEN_BR, 1 );
      case '}': return this->consume( CFT_CLOSE_BR, 1 );
      case '"': return this->consume_string();
      case ';': return this->consume( CFT_SEMI, 1 );
      case '#': this->eat_comment(); break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return this->consume_int();

      case 't': case 'T':
        if ( this->match( true_tok ) )
          return this->consume( true_tok );
        return this->consume_ident();

      case 'f': case 'F':
        if ( this->match( false_tok ) )
          return this->consume( false_tok );
        if ( this->match( fields_tok ) )
          return this->consume( fields_tok );
        if ( this->match( field_class_name_tok ) )
          return this->consume( field_class_name_tok );
        return this->consume_ident();

      case 'c': case 'C':
        if ( this->match( class_id_tok ) )
          return this->consume( class_id_tok );
        if ( this->match( cfile_includes_tok ) )
          return this->consume( cfile_includes_tok );
        return this->consume_ident();

      case 'd': case 'D':
        if ( this->match( data_size_tok ) )
          return this->consume( data_size_tok );
        if ( this->match( data_type_tok ) )
          return this->consume( data_type_tok );
        return this->consume_ident();

      case 'i': case 'I':
        if ( this->match( is_fixed_tok ) )
          return this->consume( is_fixed_tok );
        if ( this->match( is_partial_tok ) )
          return this->consume( is_partial_tok );
        if ( this->match( is_primitive_tok ) )
          return this->consume( is_primitive_tok );
        return this->consume_ident();

      default:
        if ( isalpha( this->buf[ this->off ] ) )
          return this->consume_ident();
        return this->consume( CFT_ERROR, 1 );

      case EOF_CHAR:
        return CFT_EOF;
    }
  }
}

CFile *
CFile::push_path( CFile *tos,  const char *path,  const char *filename,
                  size_t file_sz,  int debug_flags ) noexcept
{
  char path2[ 1024 ];
  if ( DictParser::find_file( path, filename, file_sz, path2 ) ) {
    void * p = ::malloc( sizeof( CFile ) );
    return new ( p ) CFile( tos, path2, debug_flags );
  }
  return NULL;
}

static MDType
tss_data_type_to_md_type( int data_type ) noexcept
{
  switch ( data_type ) {
    default: return MD_NODATA;
    case TSS_INTEGER     /*1*/: return MD_INT;
    case TSS_STRING      /*2*/: return MD_STRING;
    case TSS_BOOLEAN     /*3*/: return MD_BOOLEAN;
    case TSS_DATE        /*4*/: return MD_OPAQUE; /* binary date */
    case TSS_TIME        /*5*/: return MD_OPAQUE; /* binary time (not used) */
    case TSS_PRICE       /*6*/: return MD_REAL;
    case TSS_BYTE        /*7*/: return MD_UINT;
    case TSS_FLOAT       /*8*/: return MD_REAL;
    case TSS_SHORT_INT   /*9*/: return MD_INT;
    case TSS_DOUBLE     /*10*/: return MD_REAL;
    case TSS_OPAQUE     /*11*/: return MD_OPAQUE;
    case TSS_NULL       /*12*/: return MD_OPAQUE;
    case TSS_RESERVED   /*13*/: return MD_OPAQUE;
    case TSS_DOUBLE_INT /*14*/: return MD_REAL;    /* 8 byte float always int */
    case TSS_GROCERY    /*15*/: return MD_DECIMAL; /* float with hint */
    case TSS_SDATE      /*16*/: return MD_DATE;    /* string date */
    case TSS_STIME      /*17*/: return MD_TIME;    /* string time */
    case TSS_LONG       /*18*/: return MD_INT;
    case TSS_U_SHORT    /*19*/: return MD_UINT;
    case TSS_U_INT      /*20*/: return MD_UINT;
    case TSS_U_LONG     /*21*/: return MD_UINT;
  }
}

static MDType
tss_to_md_type( CFile *p ) noexcept
{
  if ( ! p->fld.is_empty() )
    return MD_MESSAGE;
  if ( p->is_partial == 1 &&
       ( p->data_type == MD_OPAQUE ||
         p->data_type == MD_STRING ) )
    return MD_PARTIAL;
  return tss_data_type_to_md_type( p->data_type );
}

static int
tss_type_default_size( int data_type ) noexcept
{
  switch ( data_type ) {
    default:                    return 0;
    case TSS_INTEGER     /*1*/: return 4;
    case TSS_STRING      /*2*/: return 0;
    case TSS_BOOLEAN     /*3*/: return 1;
    case TSS_DATE        /*4*/: return 6;   /* binary date */
    case TSS_TIME        /*5*/: return 4;   /* binary time (not used) */
    case TSS_PRICE       /*6*/: return 8;
    case TSS_BYTE        /*7*/: return 1;
    case TSS_FLOAT       /*8*/: return 4;
    case TSS_SHORT_INT   /*9*/: return 2;
    case TSS_DOUBLE     /*10*/: return 8;
    case TSS_OPAQUE     /*11*/: return 0;
    case TSS_NULL       /*12*/: return 0;
    case TSS_RESERVED   /*13*/: return 0;
    case TSS_DOUBLE_INT /*14*/: return 8;   /* 8 byte float always int */
    case TSS_GROCERY    /*15*/: return 9;   /* float with hint */
    case TSS_SDATE      /*16*/: return 12;  /* string date */
    case TSS_STIME      /*17*/: return 6;   /* string time */
    case TSS_LONG       /*18*/: return 8;
    case TSS_U_SHORT    /*19*/: return 2;
    case TSS_U_INT      /*20*/: return 4;
    case TSS_U_LONG     /*21*/: return 8;
  }
}

static bool si( int &ival,  int v ) noexcept {
  bool x = ( ival != 0 );
  ival = v;
  return x;
}

static bool eq( const char *str,  const char *tss,  size_t sz ) noexcept {
  return sz == ::strlen( tss ) && ::strncasecmp( str, tss, sz ) == 0;
}

static bool st( int &ival,  const char *tss_type,  size_t tok_sz ) noexcept {
  switch ( tss_type[ 0 ] ) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return si( ival, atoi( tss_type ) );
    case 'i': case 'I':
      if ( eq( tss_type, "integer", tok_sz ) ) return si( ival, TSS_INTEGER );
      break;
    case 's': case 'S':
      if ( eq( tss_type, "string", tok_sz ) ) return si( ival, TSS_STRING );
      if ( eq( tss_type, "short_int", tok_sz ) ) return si( ival, TSS_SHORT_INT );
      if ( eq( tss_type, "sdate", tok_sz ) ) return si( ival, TSS_SDATE );
      if ( eq( tss_type, "stime", tok_sz ) ) return si( ival, TSS_STIME );
      break;
    case 'b': case 'B':
      if ( eq( tss_type, "boolean", tok_sz ) ) return si( ival, TSS_BOOLEAN );
      if ( eq( tss_type, "byte", tok_sz ) ) return si( ival, TSS_BYTE );
      break;
    case 'd': case 'D':
      if ( eq( tss_type, "date", tok_sz ) ) return si( ival, TSS_DATE );
      if ( eq( tss_type, "double", tok_sz ) ) return si( ival, TSS_DOUBLE );
      if ( eq( tss_type, "double_int", tok_sz ) ) return si( ival, TSS_DOUBLE_INT );
      break;
    case 't': case 'T':
      if ( eq( tss_type, "time", tok_sz ) ) return si( ival, TSS_TIME );
      break;
    case 'p': case 'P':
      if ( eq( tss_type, "price", tok_sz ) ) return si( ival, TSS_PRICE );
      break;
    case 'f': case 'F':
      if ( eq( tss_type, "float", tok_sz ) ) return si( ival, TSS_FLOAT );
      break;
    case 'o': case 'O':
      if ( eq( tss_type, "opaque", tok_sz ) ) return si( ival, TSS_OPAQUE );
      break;
    case 'n': case 'N':
      if ( eq( tss_type, "null", tok_sz ) ) return si( ival, TSS_NULL );
      break;
    case 'r': case 'R':
      if ( eq( tss_type, "real", tok_sz ) ) return si( ival, TSS_DOUBLE );
      if ( eq( tss_type, "reserved", tok_sz ) ) return si( ival, TSS_RESERVED );
      break;
    case 'g': case 'G':
      if ( eq( tss_type, "grocery", tok_sz ) ) return si( ival, TSS_GROCERY );
      break;
    case 'l': case 'L':
      if ( eq( tss_type, "long", tok_sz ) ) return si( ival, TSS_LONG );
      break;
    case 'u': case 'U':
      if ( eq( tss_type, "u_int", tok_sz ) ) return si( ival, TSS_U_INT );
      if ( eq( tss_type, "u_short", tok_sz ) ) return si( ival, TSS_U_SHORT );
      if ( eq( tss_type, "u_long", tok_sz ) ) return si( ival, TSS_U_LONG );
      break;
  }
  return si( ival, 0 );
}

static bool sb( uint8_t &bval,  bool v ) noexcept {
  bool x = ( bval != 0 );
  bval = ( v ? 1 : 2 );
  return x;
}

int
CFile::parse_path( MDDictBuild &dict_build,  const char *path,
                   const char *fn ) noexcept
{
  CFile *p = CFile::push_path( NULL, path, fn, ::strlen( fn ),
                               dict_build.debug_flags );
  if ( p == NULL ) {
    fprintf( stderr, "\"%s\": file not found\n", fn );
    return Err::FILE_NOT_FOUND;
  }
  return CFile::parse_loop( dict_build, p, path );
}

int
CFile::parse_string( MDDictBuild &dict_build,  const char *str_input,
                     size_t str_size ) noexcept
{
  void  * m = ::malloc( sizeof( CFile ) );
  CFile * p = new ( m ) CFile( NULL, NULL, 0 );
  p->str_input = str_input;
  p->str_size  = str_size;
  return CFile::parse_loop( dict_build, p, NULL );
}

int
CFile::parse_loop( MDDictBuild &dict_build,  CFile *p,
                   const char *path ) noexcept
{
  CFile * p2;
  int     ret = 0;

  while ( p != NULL ) {
    CFileTok tok = p->get_token();
    switch ( tok ) {
      case CFT_SEMI:
        if ( p->stmt != CFT_ERROR ) {
          if ( p->stmt != CFT_FIELD_CLASS_NAME && p->stmt != CFT_FIELDS )
            p->stmt = CFT_IDENT;
        }
        break;

      case CFT_CLOSE_BR:
        if ( p->stmt == CFT_FIELD_CLASS_NAME )
          p->stmt = CFT_FIELDS;
        else if ( p->stmt == CFT_FIELDS )
          p->stmt = CFT_IDENT;
        else if ( p->stmt != CFT_ERROR ) {
          uint8_t flags = ( ( p->is_fixed < 2 ? MD_FIXED : 0 ) |
                            ( p->is_primitive < 2 ? MD_PRIMITIVE : 0 ) );
          MDType  type  = tss_to_md_type( p );
          if ( flags != ( MD_FIXED | MD_PRIMITIVE ) ) {
            if ( type != MD_STRING && type != MD_OPAQUE && type != MD_MESSAGE ){
              const char * s = ( flags == MD_FIXED ? "IS_PRIMITIVE=false" :
                                 flags == MD_PRIMITIVE ? "IS_FIXED=false" :
                                 "IS_PRIMITIVE,IS_FIXED=false" );
              fprintf( stderr, "ignoring %s at \"%s\" line %u: \"%.*s\"\n", s,
                       p->fname, p->lineno, (int) p->tok_sz, p->tok_buf );
              flags = MD_FIXED | MD_PRIMITIVE;
            }
          }
          if ( p->data_size == 0 ) {
            if ( type != MD_STRING && type != MD_OPAQUE && type != MD_MESSAGE ){
              p->data_size = tss_type_default_size( p->data_type );
              if ( p->data_size == 0 )
                fprintf( stderr, "no data size for type %d at \"%s\" line %u\n",
                         p->data_type, p->fname, p->lineno );
            }
          }
          dict_build.add_entry( p->class_id, p->data_size, type,
                                flags, p->ident, NULL, NULL, p->fname,
                                p->ident_lineno );
          /*printf( "(%s:%u) %s { class-id %u, size %u, type %u }\n",
                  p->fname, p->ident_lineno,
                  p->ident, p->class_id, p->data_size, p->data_type );
          if ( p->fld_hd != NULL ) {
            for ( CFRecField *f = p->fld_hd; f != NULL; f = f->next )
              printf( "%s/%s ", f->fname, f->classname );
            printf( "\n" );
          }*/
          p->clear_ident();
        }
        else if ( p->cf_includes )
          p->cf_includes = false;
        p->br_level--;
        break;

      case CFT_OPEN_BR:      p->br_level++; break;
      case CFT_CF_INCLUDES:  p->cf_includes = true; break;
      case CFT_CLASS_ID:     p->stmt = tok; break;
      case CFT_IS_PRIMITIVE: p->stmt = tok; break;
      case CFT_IS_FIXED:     p->stmt = tok; break;
      case CFT_IS_PARTIAL:   p->stmt = tok; break;
      case CFT_DATA_SIZE:    p->stmt = tok; break;
      case CFT_DATA_TYPE:    p->stmt = tok; break;
      case CFT_INT: {
        bool x = false;
        p->tok_buf[ p->tok_sz ] = '\0';
        switch ( p->stmt ) {
          case CFT_DATA_SIZE: x = si( p->data_size, atoi( p->tok_buf ) ); break;
          case CFT_DATA_TYPE: x = st( p->data_type, p->tok_buf, p->tok_sz ); break;
          case CFT_CLASS_ID:  x = si( p->class_id, atoi( p->tok_buf ) ); break;
          default:            goto is_error;
        }
        if ( x ) {
          fprintf( stderr, "dup " );
          goto is_error;
        }
        break;
      }
      case CFT_TRUE:
      case CFT_FALSE: {
        bool b = ( tok == CFT_TRUE ), x = false;
        switch ( p->stmt ) {
          case CFT_IS_PRIMITIVE: x = sb( p->is_primitive, b ); break;
          case CFT_IS_FIXED:     x = sb( p->is_fixed, b ); break;
          case CFT_IS_PARTIAL:   x = sb( p->is_partial, b ); break;
          default:               goto is_error;
        }
        if ( x ) {
          fprintf( stderr, "dup " );
          goto is_error;
        }
        break;
      }

      case CFT_FIELDS:
        if ( p->stmt != CFT_IDENT )
          goto is_error;
        p->stmt = CFT_FIELDS;
        break;

      case CFT_FIELD_CLASS_NAME:
        if ( p->stmt != CFT_FIELDS )
          goto is_error;
        p->stmt = CFT_FIELD_CLASS_NAME;
        break;

      case CFT_IDENT:
        if ( p->cf_includes ) {
          p2 = CFile::push_path( p, path, p->tok_buf, p->tok_sz,
                                 p->debug_flags );
          if ( p2 == p ) {
            fprintf( stderr, "\"%.*s\": file not found\n", (int) p->tok_sz,
                     p->tok_buf );
            if ( ret == 0 )
              ret = Err::FILE_NOT_FOUND;
            goto is_error;
          }
          p = p2;
        }
        else if ( p->stmt == CFT_FIELD_CLASS_NAME )
          p->set_field_class_name();
        else if ( p->stmt == CFT_FIELDS )
          p->add_field();
        else if ( p->stmt == CFT_DATA_TYPE ) {
          if ( st( p->data_type, p->tok_buf, p->tok_sz ) ) {
            fprintf( stderr, "dup " );
            goto is_error;
          }
          if ( p->data_type == 0 ) {
            fprintf( stderr, "no data type for %s at \"%s\" line %u\n",
                     p->ident, p->fname, p->ident_lineno );
          }
        }
        else if ( p->ident[ 0 ] == '\0' )
          p->set_ident();
        else {
          fprintf( stderr, "ident \"%s\" at \"%s\" line %u\n",
                   p->ident, p->fname, p->ident_lineno );
          if ( ret == 0 )
            ret = Err::DICT_PARSE_ERROR;
          goto is_error;
        }
        break;

      case CFT_ERROR:
      is_error:
        fprintf( stderr, "error at \"%s\" line %u: \"%.*s\"\n",
                 p->fname, p->lineno, (int) p->tok_sz, p->tok_buf );
        if ( ret == 0 )
          ret = Err::DICT_PARSE_ERROR;
        /* FALLTHRU */
      case CFT_EOF: {
        if ( ret == 0 && p->br_level != 0 ) {
          fprintf( stderr, "mismatched brackets: '}' in file \"%s\"\n",
                   p->fname );
        }
        p2 = (CFile *) p->next;
        delete p;
        p = p2;
        break;
      }
    }
  }
  return ret;
}

int
CFile::unpack_sass( MDDictBuild &dict_build,  MDMsg *m ) noexcept
{
  MDMsg       * fids_msg  = NULL;
  MDFieldIter * iter      = NULL,
              * fids_iter = NULL;
  MDName        name;
  MDReference   mref,
                href;
  uint32_t      bits,
                num = 0;
  uint16_t      class_id,
                fsize;
  uint8_t       ftype,
                flags = ( MD_FIXED | MD_PRIMITIVE ),
                tss_type;
  int           status;

  status = m->get_field_iter( iter );
  if ( status != 0 ) {
    fprintf( stderr, "Unable to get dict field iter: %d\n", status );
    return status;
  }
  status = iter->find( "FIDS", 5, mref );
  if ( status != 0 ) {
    fprintf( stderr, "Unable to find FIDS in dictionary: %d\n", status );
    return status;
  }
  status = m->get_sub_msg( mref, fids_msg );
  if ( status != 0 ) {
    fprintf( stderr, "FIDS field is not a message: %d\n", status );
    return status;
  }
  status = fids_msg->get_field_iter( fids_iter );
  if ( status != 0 ) {
    fprintf( stderr, "Unable to get fids field iter: %d\n", status );
    return status;
  }
  status = fids_iter->first();
  if ( status != 0 ) {
    fprintf( stderr, "Empty dict FIDS message: %d\n", status );
    return status;
  }
  do {
    if ( (status = fids_iter->get_name( name )) != 0 ||
         (status = fids_iter->get_reference( mref )) != 0 ||
         (status = fids_iter->get_hint_reference( href )) != 0 )
      break;
    if ( name.fnamelen > 0 &&
         ( mref.ftype == MD_UINT || mref.ftype == MD_INT ) &&
         ( href.ftype == MD_UINT || href.ftype == MD_INT ) ) {
      class_id = get_uint<uint16_t>( mref );
      bits     = get_uint<uint32_t>( href );
      tss_type = (uint8_t) ( bits >> 16 );
      fsize    = (uint16_t) bits;
      if ( ( ( bits >> 24 ) & 1 ) != 0 )
        ftype = MD_PARTIAL;
      else
        ftype = tss_data_type_to_md_type( tss_type );
      if ( fsize == 0 )
        fsize = tss_type_default_size( tss_type );
      status = dict_build.add_entry( class_id, fsize, (MDType) ftype, flags,
                                     name.fname, NULL, NULL, "msg", ++num );
      if ( status != 0 ) {
        fprintf( stderr, "Bad dict entry: %.*s class_id %d fsize %u ftype %u\n",
                 (int) name.fnamelen, name.fname, class_id, fsize, ftype );
      }
    }
    else {
      fprintf( stderr, "Bad dict entry: %.*s mref.ftype %d href.ftype %d\n",
               (int) name.fnamelen, name.fname, mref.ftype, href.ftype );
    }
  } while ( (status = fids_iter->next()) == 0 );
  if ( status != Err::NOT_FOUND ) {
    fprintf( stderr, "Error iterating dict msg: %d\n", status );
    return status;
  }
  return 0;
}

