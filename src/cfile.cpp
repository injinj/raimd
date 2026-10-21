#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/md_msg.h>
#include <raimd/tib_sass_msg.h>
#include <raimd/tib_msg.h>
#include <raimd/app_a.h>

using namespace rai;
using namespace md;

#define S( s ) s, sizeof( s ) - 1
static CFileKeyword
  true_tok             = { S( "true"             ), CFT_TRUE },
  false_tok            = { S( "false"            ), CFT_FALSE },
  fields_tok           = { S( "fields"           ), CFT_FIELDS },
  field_class_name_tok = { S( "field_class_name" ), CFT_FIELD_CLASS_NAME },
  class_id_tok         = { S( "class_id"         ), CFT_CLASS_ID },
  cfile_includes_tok   = { S( "cfile_includes"   ), CFT_CF_INCLUDES },
  data_size_tok        = { S( "data_size"        ), CFT_DATA_SIZE },
  data_type_tok        = { S( "data_type"        ), CFT_DATA_TYPE },
  is_fixed_tok         = { S( "is_fixed"         ), CFT_IS_FIXED },
  is_partial_tok       = { S( "is_partial"       ), CFT_IS_PARTIAL },
  is_primitive_tok     = { S( "is_primitive"     ), CFT_IS_PRIMITIVE };
#undef S

const char *
rai::md::tss_type_str( TSS_type tp ) noexcept
{
  switch ( tp ) {
    case TSS_INTEGER:    return "INTEGER";
    case TSS_STRING:     return "STRING";
    case TSS_BOOLEAN:    return "BOOLEAN";
    case TSS_DATE:       return "DATE";
    case TSS_TIME:       return "TIME";
    case TSS_PRICE:      return "PRICE";
    case TSS_BYTE:       return "BYTE";
    case TSS_FLOAT:      return "FLOAT";
    case TSS_SHORT_INT:  return "SHORT_INT";
    case TSS_DOUBLE:     return "DOUBLE";
    case TSS_OPAQUE:     return "OPAQUE";
    case TSS_NULL:       return "NULL";
    case TSS_RESERVED:   return "RESERVED";
    case TSS_DOUBLE_INT: return "DOUBLE_INT";
    case TSS_GROCERY:    return "GROCERY";
    case TSS_SDATE:      return "SDATE";
    case TSS_STIME:      return "STIME";
    case TSS_LONG:       return "LONG";
    case TSS_U_SHORT:    return "U_SHORT";
    case TSS_U_INT:      return "U_INT";
    case TSS_U_LONG:     return "U_LONG";
    default:             return "NODATA";
  }
}

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

rai::md::TSS_type
CFile::md_to_tss_type( MDLookup &mentry,  uint32_t &size ) noexcept
{
  TSS_type tp;
  switch ( mentry.ftype ) {
    case MD_REAL:
      size = mentry.fsize;
      switch ( size ) {
        case 4: tp = TSS_PRICE; break;
        case 8: tp = TSS_DOUBLE_INT; break;
        default: tp = TSS_FLOAT; break;
      }
      break;
    case MD_PARTIAL:
      tp   = TSS_STRING;
      size = mentry.fsize;
      break;
    case MD_STRING:
      tp   = TSS_STRING;
      size = mentry.fsize;
      break;
    case MD_TIME:
      tp   = TSS_STIME;
      size = mentry.fsize;
      break;
    case MD_DATE:
      tp   = TSS_SDATE;
      size = mentry.fsize;
      break;
    case MD_INT:
      size = mentry.fsize;
      switch ( size ) {
        case 2: tp = TSS_SHORT_INT; break;
        case 8: tp = TSS_LONG; break;
        default: tp = TSS_INTEGER; break;
      }
      break;
    case MD_UINT:
      size = mentry.fsize;
      switch ( size ) {
        case 1: tp = TSS_BYTE; break;
        case 2: tp = TSS_U_SHORT; break;
        case 4: tp = TSS_U_INT; break;
        case 8: tp = TSS_U_LONG; break;
        default: tp = TSS_INTEGER; break;
      }
      break;
    case MD_DECIMAL:
      tp   = TSS_GROCERY;
      size = mentry.fsize;
      break;
    case MD_OPAQUE:
      tp   = TSS_OPAQUE;
      size = mentry.fsize;
      break;
    case MD_BOOLEAN:
      tp   = TSS_BOOLEAN;
      size = mentry.fsize;
      break;
    default:
      tp   = TSS_NODATA;
      size = 0;
      break;
  }
  return tp;
}

static MDType
tss_to_md_type( CFile *p ) noexcept
{
  if ( ! p->fld.is_empty() )
    return MD_MESSAGE;
  if ( p->is_partial == 1 &&
       ( p->data_type == TSS_OPAQUE ||
         p->data_type == TSS_STRING ) )
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
        else if ( p->cf_includes )
          p->cf_includes = false;
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
          MDDictAdd a;
          a.fid      = p->class_id;
          a.fsize    = p->data_size;
          a.ftype    = type;
          a.tss_type = p->data_type;
          a.flags    = flags;
          a.fname    = p->ident;
          a.filename = p->fname;
          a.lineno   = p->ident_lineno;
          dict_build.add_entry( a );
                              /*p->class_id, p->data_size, type,
                                flags, p->ident, NULL, NULL, p->fname,
                                p->ident_lineno ); */
          /*printf( "(%s:%u) %s { class-id %u, size %u, type %u }\n",
                  p->fname, p->ident_lineno,
                  p->ident, p->class_id, p->data_size, p->data_type );*/
          if ( p->fld.hd != NULL ) {
            MDFormBuild form_build;
            for ( CFRecField *f = p->fld.hd; f != NULL; f = f->next ) {
              /*printf( "%s/%s ", f->fname, f->classname );*/
              size_t len = ::strlen( f->classname ) + 1;
              uint32_t h = MDDict::dict_hash( f->classname, len );
              MDDictEntry *entry =
                dict_build.idx->get_fname_entry( f->classname, len, h );
              if ( entry == NULL ) {
                fprintf( stderr, "form %s, field %s undefined at \"%s\" line %u\n",
                         p->ident, f->classname, p->fname, p->lineno );
              }
              else {
                if ( ! form_build.add( entry->fid ) ) {
                  fprintf( stderr, "form %s, field %s too many fids \"%s\" line %u\n",
                           p->ident, f->classname, p->fname, p->lineno );
                  form_build.nfids = 0;
                  break;
                }
              }
            }
            if ( form_build.nfids > 0 ) {
              if ( ! form_build.compress() ) {
                fprintf( stderr, "form %s, too many fids \"%s\" line %u\n",
                         p->ident, p->fname, p->lineno );
              }
              else {
                dict_build.add_form_build( form_build );
                dict_build.update_entry_form( p->class_id, form_build.map_num );
              }
            }
          }
          p->clear_ident();
        }
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
          p2->stmt = p->stmt;
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
        if ( p->stmt == CFT_IDENT && ! p->fld.is_empty() ) {
          p2->fld.push_tl( p->fld.hd );
          p2->fld.tl = p->fld.tl;
          p->fld.init();
        }
        delete p;
        p = p2;
        break;
      }
    }
  }
  return ret;
}

bool
MDFormBuild::compress( void ) noexcept
{
  size_t i = 0, j, k, n, rng, x, start, end, bit;
  uint16_t bits[ 4 * 1024 ];

  ::memset( bits, 0, sizeof( bits ) );
  n = 0;
  x = 1;
  for (;;) {
    for ( j = i + 1; ; j++ ) {
      if ( j == this->nfids )
        break;
      if ( this->fids[ j ] <= this->fids[ j-1 ] )
        break;
      if ( this->fids[ j ] >= this->fids[ j-1 ] + 48 )
        break;
    }
    if ( x + 2 > sizeof( this->code ) / sizeof( this->code[ 0 ] ) )
      return false;
    this->code[ x++ ] = this->fids[ i ];
    this->code[ x++ ] = this->fids[ j - 1 ];
    if ( i + 2 < j ) {
      start = this->fids[ i ] + 1;
      end   = this->fids[ j - 1 ];
      rng   = end - start;

      for ( k = i + 1; k < j - 1; k++ ) {
        bit = this->fids[ k ] - start;
        bits[ ( n + bit ) / 16 ] |= 1 << ( ( n + bit ) % 16 );
      }
    }
    else {
      rng = 0;
      this->code[ x-1 ] |= 0x8000;
    }
    n += rng;
    i = j;
    if ( i == this->nfids )
      break;
  }
  n += ( 16 - ( n % 16 ) );
  n /= 16;
  if ( x + n > sizeof( this->code ) / sizeof( this->code[ 0 ] ) )
    return false;
  ::memcpy( &this->code[ x ], bits, sizeof( bits[ 0 ] ) * n );
  this->code[ 0 ] = x;
  x += n;
  this->code_size = x;
  return true;
}

uint32_t
MDFormMap::fid_count( void ) noexcept
{
  uint16_t * code = this->code();
  size_t bit = 0, cnt = code[ 0 ], x = 0;
  for ( size_t i = 1; i < cnt; i += 2 ) {
    uint16_t start   = code[ i ],
             no_bits = code[ i+1 ] & 0x8000,
             end     = code[ i+1 ] & 0x7fff;
    x++;
    if ( no_bits ) {
      if ( end != start )
        x++;
    }
    else {
      uint16_t rng = end - (start+1);
      for ( uint16_t i = 0; i < rng; bit++, i++ ) {
        if ( ( code[ cnt + (bit/16) ] & ( 1 << (bit%16) ) ) != 0 )
          x++;
      }
      x++;
    }
  }
  return x;
}

uint32_t
MDFormMap::get_fids( uint16_t *fids ) noexcept
{
  uint16_t * code = this->code();
  size_t bit = 0, cnt = code[ 0 ], x = 0;
  for ( size_t i = 1; i < cnt; i += 2 ) {
    uint16_t start   = code[ i ],
             no_bits = code[ i+1 ] & 0x8000,
             end     = code[ i+1 ] & 0x7fff;
    fids[ x++ ] = start;
    if ( no_bits ) {
      if ( end != start )
        fids[ x++ ] = end;
    }
    else {
      uint16_t rng = end - (start+1);
      for ( uint16_t i = 0; i < rng; bit++, i++ ) {
        if ( ( code[ cnt + (bit/16) ] & ( 1 << (bit%16) ) ) != 0 )
          fids[ x++ ] = start+1+i;
      }
      fids[ x++ ] = end;
    }
  }
  return x;
}

bool
MDFormMap::fid_is_member( uint16_t fid ) noexcept
{
  uint16_t * code = this->code();
  size_t bit = 0, cnt = code[ 0 ];
  for ( size_t i = 1; i < cnt; i += 2 ) {
    uint16_t start   = code[ i ],
             no_bits = code[ i+1 ] & 0x8000,
             end     = code[ i+1 ] & 0x7fff;
    if ( fid <= end && fid >= start ) {
      if ( fid == end || fid == start )
        return true;
      if ( no_bits )
        return false;
      bit += fid - (start+1);
      if ( ( code[ cnt + (bit/16) ] & ( 1 << (bit%16) ) ) != 0 )
        return true;
      return false;
    }
    else if ( ! no_bits ) {
      bit += end - (start+1);
    }
  }
  return false;
}

static inline uint32_t mkhtmask( uint32_t n ) {
  for ( uint32_t x = n - 1; ( ( n + 1 ) & n ) != 0; x >>= 1 )
    n |= x;
  return n;
}

MDFormClass *
MDFormClass::make_form_class( MDDict &d,  MDFid fid,  MDFormMap &map ) noexcept
{
  uint32_t count   = map.fid_count(),
           htmask  = mkhtmask( count + count / 4 );
  size_t       sz  = sizeof( MDFormClass ) + count * sizeof( MDFormEntry ) +
                     (htmask+1) * sizeof( uint16_t );
  void        * m  = ::malloc( sz );
  MDFormEntry * e  = (MDFormEntry *) (void *) &((MDFormClass *) m)[ 1 ];
  uint16_t    * ht = (uint16_t *) (void *) &e[ count ];
  MDFormClass * fc = new ( m ) MDFormClass( fid, d, map, e, ht, count, htmask+1 );

  map.get_fids( ht );
  size_t off = 0;
  uint32_t i;
  for ( i = 0; i < count; i++ ) {
    e[ i ].fid     = ht[ i ];
    e[ i ].foffset = off;

    MDLookup by( e[ i ].fid );
    bool b = ( d.lookup( by ) && by.flags == ( MD_PRIMITIVE | MD_FIXED ) );
    if ( b ) {
      size_t fsz = by.fsize;

      if ( by.ftype != MD_PARTIAL )
        fsz = tib_sass_pack_size( fsz );
      else
        fsz = tib_sass_partial_pack_size( fsz );
      off += fsz;
      if ( off > 0xffff ) {
        fprintf( stderr, "formclass %u too large\n", fid );
        b = false;
      }
    }
    else {
      fprintf( stderr, "formclass %u missing fid %u\n", fid, e[ i ].fid );
    }
    if ( ! b ) {
      delete fc;
      return NULL;
    }
  }
  fc->form_size = off;
  ::memset( ht, 0, (htmask+1) * sizeof( uint16_t ) );
  for ( i = 0; i < count; i++ ) {
    size_t pos = MDFormKey::hash( e[ i ].fid ) & htmask;
    while ( ht[ pos ] != 0 )
      pos = ( pos + 1 ) & htmask;
    ht[ pos ] = i + 1;
  }
  return fc;
}

const MDFormEntry *
MDFormClass::lookup( MDLookup &by ) const
{
  if ( ! this->dict.lookup( by ) )
    return NULL;
  return this->get_entry( by.fid );
}

const MDFormEntry *
MDFormClass::get( MDLookup &by ) const
{
  if ( ! this->dict.get( by ) )
    return NULL;
  return this->get_entry( by.fid );
}

const MDFormEntry *
MDFormClass::get_entry( MDFid fid ) const
{
  size_t htmask = this->htsize - 1,
         pos    = MDFormKey::hash( fid ) & htmask;

  while ( this->ht[ pos ] != 0 ) {
    uint16_t i = this->ht[ pos ] - 1;
    if ( (MDFid) this->entries[ i ].fid == fid )
      return &this->entries[ i ];
    pos = ( pos + 1 ) & htmask;
  }
  return NULL;
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
  status = m->get_sub_msg( mref, fids_msg, iter );
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
      MDDictAdd a;
      a.fid      = class_id;
      a.fsize    = fsize;
      a.ftype    = (MDType) ftype;
      a.tss_type = tss_type;
      a.flags    = flags;
      a.fname    = name.fname;
      a.filename = "msg";
      a.lineno   = ++num;
      status = dict_build.add_entry( a );
      /*status = dict_build.add_entry( class_id, fsize, (MDType) ftype, flags,
                                     name.fname, NULL, NULL, "msg", ++num );*/
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

int
CFile::pack_sass( MDDict *dict,  TibMsgWriter &wr ) noexcept
{
  for ( ; dict != NULL; dict = dict->get_next() )
    if ( dict->dict_type[ 0 ] == 'c' ) /* need cfile type */
      break;
  if ( dict == NULL )
    return Err::NO_DICTIONARY;

  uint32_t count = 0;
  for ( MDFid fid = dict->min_fid; fid <= dict->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( dict->lookup( by ) ) {
      uint32_t u32 = 0;
      TSS_type tss = md_to_tss_type( by, u32 );
      if ( tss != TSS_NODATA )
        count += (uint32_t) by.fname_len;
    }
  }
  static const char max_fid[]    = "MAX_FID";
  static const char name_bytes[] = "NAME_BYTES";
  static const char fids[]       = "FIDS";
  wr.append_uint( max_fid, sizeof( max_fid ), (uint16_t) dict->max_fid );
  wr.append_uint( name_bytes, sizeof( name_bytes ), (uint32_t) count );

  TibMsgWriter sub( wr.mem(), NULL, 0 );
  wr.append_msg( fids, sizeof( fids ), sub );
  for ( MDFid fid = dict->min_fid; fid <= dict->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( dict->lookup( by ) ) {
      uint16_t u16     = (uint16_t) fid;
      uint32_t is_part = ( by.ftype == MD_PARTIAL ),
               u32     = 0;
      TSS_type tss     = md_to_tss_type( by, u32 );
      if ( tss != TSS_NODATA ) {
        u32 |= ( is_part << 24 ) | ( tss << 16 );
        MDReference ref( &u16, sizeof( u16 ), MD_UINT ),
                    href( &u32, sizeof( u32 ), MD_UINT );
        sub.append_ref( by.fname, by.fname_len, ref, href );
      }
    }
  }
  if ( sub.err != 0 )
    return sub.err;
  wr.update_hdr( sub );
  if ( wr.err != 0 )
    return wr.err;
  return 0;
}

/* sass3 dictionary ------------------------------------------------------
 *
 * The format published by the RaiCore cache on _TIC.REPLY.SASS.DATA.DICTIONARY
 * (RaiMsg_config::packDataDictionary( msg, true ) followed by
 * RaiMfeed_dict::packDataDictionary2( msg )) and consumed by raimdapi:
 *
 *   MAX_FID, NAME_BYTES, FIDS { fname : u16 fid, hint u32 bits }  (pack_sass)
 *   FORMS { formname : u16 fid, hint u32 nfields,  then nfields x
 *           fname : u16 fid, hint u32 bits }
 *   MARKETFEED { entry-count .. max-fid counts, then opaque records:
 *     a : i16 fid, i16 ripple, u16 flen, u16 enumlen, u16 fenum, u8 ftype,
 *         u8 dde_off, acro\0, dde_acro\0
 *     e : u16 fenum, u16 value, display\0
 *     l : u16 fidcount, u16 valuecount, u16 min, u16 max, i16 fids[],
 *         u16 values[]
 *     m : i16 n, fname\0            (flist-count flist maps, fid-count fid maps)
 *     rwf-type-num, then entry-count x w : i16 fid, u16 rwflen, u8 rwftype,
 *         u8 rwfbits }
 *
 * Integers inside the opaque records are big endian.  The count fields are
 * the allocation hints the legacy reader needs, computed the same way.
 * fenum is the 1 based index of the l record a fid's enum belongs to. */

static const char forms_f[]                = "FORMS",
                  mfeed_f[]                = "MARKETFEED",
                  entry_count_f[]          = "entry-count",
                  enum_entry_count_f[]     = "enum-entry-count",
                  enum_list_count_f[]      = "enum-list-count",
                  enum_list_size_f[]       = "enum-list-size",
                  flist_count_f[]          = "flist-count",
                  fid_count_f[]            = "fid-count",
                  map_fname_bytes_f[]      = "map-fname-bytes",
                  dict_size_f[]            = "dict-size",
                  enum_acro_elem_count_f[] = "enum-acro-elem-count",
                  max_fid_f[]              = "max-fid",
                  rwf_type_count_f[]       = "rwf-type-num",
                  acro_f[]                 = "a",
                  enum_f[]                 = "e",
                  enumlist_f[]             = "l",
                  map_f[]                  = "m",
                  rwf_f[]                  = "w";

/* raimd MF_type -> RaiMfeed_type on the wire:
 * MFT_ALPHANUMERIC 1, TIME 2, DATE 3, ENUMERATED 4, INTEGER 5, PRICE 6,
 * TIME_SECONDS 7, BINARY 8 */
static const uint8_t md_mf_to_wire[] = {
/* MF_TIME_SECONDS      */ 7,
/* MF_INTEGER           */ 5,
/* MF_NUMERIC           */ 6,
/* MF_DATE              */ 3,
/* MF_PRICE             */ 6,
/* MF_ALPHANUMERIC      */ 1,
/* MF_ENUMERATED        */ 4,
/* MF_TIME              */ 2,
/* MF_BINARY            */ 8,
/* MF_LONG_ALPHANUMERIC */ 1,
/* MF_OPAQUE            */ 8
};
static const int8_t wire_mf_to_md[] = {
  MF_NONE, MF_ALPHANUMERIC, MF_TIME, MF_DATE, MF_ENUMERATED, MF_INTEGER,
  MF_PRICE, MF_TIME_SECONDS, MF_BINARY
};

static inline uint8_t *be_put16( uint8_t *p,  uint16_t v ) {
  p[ 0 ] = (uint8_t) ( v >> 8 ); p[ 1 ] = (uint8_t) v; return &p[ 2 ];
}
static inline uint16_t be_get16( const uint8_t *p ) {
  return (uint16_t) ( ( (uint16_t) p[ 0 ] << 8 ) | p[ 1 ] );
}
static inline uint32_t align4( uint32_t x ) { return ( x + 3U ) & ~3U; }

/* the fid + bits hint used for FIDS and FORMS field entries */
static inline void
sass_fid_hint( MDLookup &by,  uint16_t &u16,  uint32_t &u32 ) noexcept
{
  uint32_t is_part = ( by.ftype == MD_PARTIAL );
  u32 = 0;
  TSS_type tss = CFile::md_to_tss_type( by, u32 );
  u16  = (uint16_t) by.fid;
  u32 |= ( is_part << 24 ) | ( (uint32_t) tss << 16 );
}

int
CFile::pack_sass3( MDDict *dict,  TibMsgWriter &wr ) noexcept
{
  MDDict * cfile = NULL,
         * rdm   = NULL,
         * flist = NULL;
  int      status;

  for ( MDDict *d = dict; d != NULL; d = d->get_next() ) {
    if ( d->dict_type[ 0 ] == 'c' )      cfile = d;
    else if ( d->dict_type[ 0 ] == 'a' ) rdm   = d;
    else if ( d->dict_type[ 0 ] == 'f' ) flist = d;
  }
  if ( cfile == NULL )
    return Err::NO_DICTIONARY;
  if ( (status = CFile::pack_sass( cfile, wr )) != 0 )
    return status;

  /* FORMS: every MD_MESSAGE class with a form map */
  {
    TibMsgWriter sub( wr.mem(), NULL, 0 );
    wr.append_msg( forms_f, sizeof( forms_f ), sub );
    for ( MDFid fid = cfile->min_fid; fid <= cfile->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( ! cfile->lookup( by ) || by.ftype != MD_MESSAGE || by.map_num == 0 )
        continue;
      MDFormClass * fc = cfile->get_form_class( fid );
      if ( fc == NULL )
        continue;
      uint16_t u16 = (uint16_t) fid;
      uint32_t u32 = fc->nentries;
      MDReference ref( &u16, sizeof( u16 ), MD_UINT ),
                  href( &u32, sizeof( u32 ), MD_UINT );
      sub.append_ref( by.fname, by.fname_len, ref, href );
      for ( uint32_t i = 0; i < fc->nentries; i++ ) {
        MDLookup fby( fc->entries[ i ].fid );
        if ( ! cfile->lookup( fby ) )
          return Err::NO_FORM;
        sass_fid_hint( fby, u16, u32 );
        MDReference r2( &u16, sizeof( u16 ), MD_UINT ),
                    h2( &u32, sizeof( u32 ), MD_UINT );
        sub.append_ref( fby.fname, fby.fname_len, r2, h2 );
      }
    }
    if ( sub.err != 0 )
      return sub.err;
    wr.update_hdr( sub );
  }
  if ( rdm == NULL )
    return wr.err;

  /* MARKETFEED: app_a entries + enum maps + flist map */
  MDMsgMem   tmp;
  uint32_t   nmaps            = rdm->map_count,
             entry_count      = 0,
             dict_size        = 0,
             rwf_count        = 0,
             enum_list_count  = 0,
             enum_entry_count = 0,
             enum_list_size   = 0,
             enum_acro_count  = 0,
             flist_count      = 0,
             map_fname_bytes  = 0,
             i, j, k;
  int        max_fid          = 0;
  uint32_t * map_fid_cnt = (uint32_t *) tmp.make( nmaps * sizeof( uint32_t ) ),
           * map_fid_off = (uint32_t *) tmp.make( nmaps * sizeof( uint32_t ) ),
           * map_list    = (uint32_t *) tmp.make( nmaps * sizeof( uint32_t ) );
  ::memset( map_fid_cnt, 0, nmaps * sizeof( uint32_t ) );
  ::memset( map_list, 0, nmaps * sizeof( uint32_t ) );

  for ( MDFid fid = rdm->min_fid; fid <= rdm->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( ! rdm->lookup( by ) )
      continue;
    const char * name, * ripple;
    uint8_t      namelen, ripplelen,
                 rwft;
    uint32_t     rwfl;
    by.get_name_ripple( name, namelen, ripple, ripplelen );
    uint32_t x = by.fname_len,
             y = ( namelen != 0 ? namelen : by.fname_len );
    dict_size += align4( 16 + x + y ); /* sizeof( RaiMfeed_entry ) - 4 */
    entry_count++;
    if ( fid > max_fid )
      max_fid = fid;
    by.rwf_type( rwft, rwfl );
    if ( rwft != RWF_NONE )
      rwf_count++;
    if ( by.map_num != 0 && by.map_num < nmaps )
      map_fid_cnt[ by.map_num ]++;
  }
  /* enum lists are the enum maps in map_num order, fenum = list index + 1 */
  for ( i = 1; i < nmaps; i++ ) {
    MDEnumMap * map = rdm->get_enum_map( i );
    if ( map == NULL )
      continue;
    map_list[ i ]     = ++enum_list_count;
    map_fid_off[ i ]  = enum_acro_count;
    enum_acro_count  += map_fid_cnt[ i ];
    enum_entry_count += map->value_cnt;
    enum_list_size   += map->value_cnt * align4( 4 + map->max_len + 1 );
  }
  int16_t * map_fids = (int16_t *)
                       tmp.make( ( enum_acro_count + 1 ) * sizeof( int16_t ) );
  for ( MDFid fid = rdm->min_fid; fid <= rdm->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( rdm->lookup( by ) && by.map_num != 0 && by.map_num < nmaps &&
         map_list[ by.map_num ] != 0 )
      map_fids[ map_fid_off[ by.map_num ]++ ] = (int16_t) fid;
  }
  for ( i = 1; i < nmaps; i++ ) /* back to the start of each list */
    map_fid_off[ i ] -= map_fid_cnt[ i ];
  if ( flist != NULL ) {
    for ( MDFid fid = flist->min_fid; fid <= flist->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( flist->lookup( by ) ) {
        flist_count++;
        map_fname_bytes += align4( by.fname_len );
      }
    }
  }

  TibMsgWriter sub( wr.mem(), NULL, 0 );
  wr.append_msg( mfeed_f, sizeof( mfeed_f ), sub );
  sub.append_uint( entry_count_f, sizeof( entry_count_f ), entry_count );
  sub.append_uint( enum_entry_count_f, sizeof( enum_entry_count_f ),
                   enum_entry_count );
  sub.append_uint( enum_list_count_f, sizeof( enum_list_count_f ),
                   enum_list_count );
  sub.append_uint( enum_list_size_f, sizeof( enum_list_size_f ),
                   enum_list_size );
  sub.append_uint( flist_count_f, sizeof( flist_count_f ), flist_count );
  sub.append_uint( fid_count_f, sizeof( fid_count_f ), (uint32_t) 0 );
  sub.append_uint( map_fname_bytes_f, sizeof( map_fname_bytes_f ),
                   map_fname_bytes );
  sub.append_uint( dict_size_f, sizeof( dict_size_f ), dict_size );
  sub.append_uint( enum_acro_elem_count_f, sizeof( enum_acro_elem_count_f ),
                   enum_acro_count );
  sub.append_int( max_fid_f, sizeof( max_fid_f ), (int32_t) max_fid );

  size_t    pak_sz = 8 * 1024;
  uint8_t * pak    = (uint8_t *) tmp.make( pak_sz );
  /* a : the entries */
  for ( MDFid fid = rdm->min_fid; fid <= rdm->max_fid; fid++ ) {
    MDLookup by( fid );
    if ( ! rdm->lookup( by ) )
      continue;
    const char * name, * ripple;
    uint8_t      namelen, ripplelen, mft;
    uint32_t     mfl, enuml;
    int16_t      ripple_fid = 0;
    by.get_name_ripple( name, namelen, ripple, ripplelen );
    if ( ripplelen != 0 ) {
      MDLookup rby( ripple, ripplelen );
      if ( rdm->get( rby ) )
        ripple_fid = (int16_t) rby.fid;
    }
    by.mf_type( mft, mfl, enuml );
    uint8_t * p = pak;
    p = be_put16( p, (uint16_t) (int16_t) fid );
    p = be_put16( p, (uint16_t) ripple_fid );
    p = be_put16( p, (uint16_t) mfl );
    p = be_put16( p, (uint16_t) enuml );
    p = be_put16( p, (uint16_t) ( by.map_num < nmaps ? map_list[ by.map_num ]
                                                     : 0 ) );
    *p++ = ( mft < sizeof( md_mf_to_wire ) ? md_mf_to_wire[ mft ] : 0 );
    *p++ = by.fname_len;
    ::memcpy( p, by.fname, by.fname_len ); p = &p[ by.fname_len ];
    if ( namelen != 0 ) {
      ::memcpy( p, name, namelen ); p = &p[ namelen ];
    }
    else {
      ::memcpy( p, by.fname, by.fname_len ); p = &p[ by.fname_len ];
    }
    MDReference ref( pak, p - pak, MD_OPAQUE );
    sub.append_ref( acro_f, sizeof( acro_f ), ref );
  }
  /* e : the enum values, l : the enum lists */
  for ( i = 1; i < nmaps; i++ ) {
    MDEnumMap * map = rdm->get_enum_map( i );
    if ( map == NULL )
      continue;
    uint16_t * value = map->value();
    uint8_t  * disp  = map->map();
    for ( j = 0; j < map->value_cnt; j++ ) {
      uint8_t * p = pak;
      p = be_put16( p, (uint16_t) map_list[ i ] );
      p = be_put16( p, ( value != NULL ? value[ j ] : (uint16_t) j ) );
      ::memcpy( p, &disp[ j * map->max_len ], map->max_len );
      p = &p[ map->max_len ];
      *p++ = '\0';
      MDReference ref( pak, p - pak, MD_OPAQUE );
      sub.append_ref( enum_f, sizeof( enum_f ), ref );
    }
  }
  for ( i = 1; i < nmaps; i++ ) {
    MDEnumMap * map = rdm->get_enum_map( i );
    if ( map == NULL )
      continue;
    uint16_t * value = map->value();
    size_t     need  = 8 + 2 * ( map_fid_cnt[ i ] + map->value_cnt );
    if ( need > pak_sz ) {
      pak_sz = need;
      pak    = (uint8_t *) tmp.make( pak_sz );
    }
    uint8_t * p = pak;
    p = be_put16( p, (uint16_t) map_fid_cnt[ i ] );
    p = be_put16( p, (uint16_t) map->value_cnt );
    p = be_put16( p, ( value != NULL ? value[ 0 ] : (uint16_t) 0 ) );
    p = be_put16( p, map->max_value );
    for ( j = 0; j < map_fid_cnt[ i ]; j++ )
      p = be_put16( p, (uint16_t) map_fids[ map_fid_off[ i ] + j ] );
    for ( k = 0; k < map->value_cnt; k++ )
      p = be_put16( p, ( value != NULL ? value[ k ] : (uint16_t) k ) );
    MDReference ref( pak, p - pak, MD_OPAQUE );
    sub.append_ref( enumlist_f, sizeof( enumlist_f ), ref );
  }
  /* m : flist -> form class name (no fid maps) */
  if ( flist != NULL ) {
    for ( MDFid fid = flist->min_fid; fid <= flist->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( ! flist->lookup( by ) )
        continue;
      uint8_t * p = pak;
      p = be_put16( p, (uint16_t) (int16_t) fid );
      ::memcpy( p, by.fname, by.fname_len ); p = &p[ by.fname_len ];
      MDReference ref( pak, p - pak, MD_OPAQUE );
      sub.append_ref( map_f, sizeof( map_f ), ref );
    }
  }
  /* w : rwf types, one per entry */
  if ( rwf_count != 0 ) {
    sub.append_uint( rwf_type_count_f, sizeof( rwf_type_count_f ), rwf_count );
    for ( MDFid fid = rdm->min_fid; fid <= rdm->max_fid; fid++ ) {
      MDLookup by( fid );
      if ( ! rdm->lookup( by ) )
        continue;
      uint8_t  rwft, bits = 0;
      uint32_t rwfl;
      by.rwf_type( rwft, rwfl );
      if ( ( rwft == RWF_INT || rwft == RWF_UINT || rwft == RWF_REAL ) &&
           ( rwfl == 4 || rwfl == 8 ) )
        bits = (uint8_t) ( rwfl * 8 );
      uint8_t * p = pak;
      p = be_put16( p, (uint16_t) (int16_t) fid );
      p = be_put16( p, (uint16_t) rwfl );
      *p++ = rwft;
      *p++ = bits;
      MDReference ref( pak, p - pak, MD_OPAQUE );
      sub.append_ref( rwf_f, sizeof( rwf_f ), ref );
    }
  }
  if ( sub.err != 0 )
    return sub.err;
  wr.update_hdr( sub );
  return wr.err;
}

/* opaque record pointers collected from the MARKETFEED msg */
struct Sass3Rec {
  const uint8_t * p;
  size_t          len;
};

static int
sass3_find_uint( MDFieldIter *it,  const char *f,  size_t flen,
                 uint32_t &val ) noexcept
{
  MDReference mref;
  int status = it->find( f, flen, mref );
  if ( status != 0 )
    return status;
  if ( mref.ftype != MD_UINT && mref.ftype != MD_INT )
    return Err::BAD_FIELD_TYPE;
  val = get_uint<uint32_t>( mref );
  return 0;
}

int
CFile::unpack_sass3( MDDictBuild &dict_build,  MDMsg *m,
                     MDDict *&dict ) noexcept
{
  MDFieldIter * iter = NULL;
  MDReference   mref,
                href;
  MDName        name;
  int           status;
  uint32_t      i, j, num = 0;

  dict = NULL;
  if ( (status = CFile::unpack_sass( dict_build, m )) != 0 )
    return status;
  /* FORMS */
  if ( (status = m->get_field_iter( iter )) != 0 )
    return status;
  if ( iter->find( forms_f, sizeof( forms_f ), mref ) == 0 ) {
    MDMsg       * forms_msg = NULL;
    MDFieldIter * fi        = NULL;
    if ( (status = m->get_sub_msg( mref, forms_msg, iter )) != 0 ||
         (status = forms_msg->get_field_iter( fi )) != 0 )
      return status;
    status = fi->first();
    while ( status == 0 ) {
      if ( (status = fi->get_name( name )) != 0 ||
           (status = fi->get_reference( mref )) != 0 ||
           (status = fi->get_hint_reference( href )) != 0 )
        break;
      if ( name.fnamelen == 0 ||
           ( mref.ftype != MD_UINT && mref.ftype != MD_INT ) ||
           ( href.ftype != MD_UINT && href.ftype != MD_INT ) ) {
        status = Err::BAD_FIELD;
        break;
      }
      uint16_t form_fid = get_uint<uint16_t>( mref );
      uint32_t count    = get_uint<uint32_t>( href );
      MDDictAdd a;
      a.fid      = form_fid;
      a.ftype    = MD_MESSAGE;
      a.flags    = MD_FIXED; /* what the parser gives a fields {} class */
      a.fname    = name.fname;
      a.filename = "msg";
      a.lineno   = ++num;
      if ( (status = dict_build.add_entry( a )) != 0 ) {
        fprintf( stderr, "Bad form entry: %.*s class_id %u\n",
                 (int) name.fnamelen, name.fname, form_fid );
        break;
      }
      MDFormBuild * fb = (MDFormBuild *) ::malloc( sizeof( MDFormBuild ) );
      fb->nfids = fb->code_size = fb->map_num = 0;
      for ( i = 0; i < count; i++ ) {
        if ( (status = fi->next()) != 0 ||
             (status = fi->get_reference( mref )) != 0 )
          break;
        if ( ( mref.ftype != MD_UINT && mref.ftype != MD_INT ) ||
             ! fb->add( get_uint<uint16_t>( mref ) ) ) {
          status = Err::BAD_FIELD;
          break;
        }
      }
      if ( status == 0 ) {
        if ( fb->nfids > 0 ) {
          if ( ! fb->compress() )
            fprintf( stderr, "form %.*s, too many fids\n",
                     (int) name.fnamelen, name.fname );
          else {
            dict_build.add_form_build( *fb );
            dict_build.update_entry_form( form_fid, fb->map_num );
          }
        }
        status = fi->next();
      }
      ::free( fb );
    }
    if ( status != Err::NOT_FOUND ) {
      fprintf( stderr, "Error iterating FORMS msg: %d\n", status );
      return status;
    }
  }
  dict_build.index_dict( "cfile", dict );
  dict_build.clear_build();

  /* MARKETFEED */
  if ( (status = m->get_field_iter( iter )) != 0 )
    return status;
  if ( iter->find( mfeed_f, sizeof( mfeed_f ), mref ) != 0 )
    return 0;
  MDMsg       * mf = NULL;
  MDFieldIter * mi = NULL;
  if ( (status = m->get_sub_msg( mref, mf, iter )) != 0 ||
       (status = mf->get_field_iter( mi )) != 0 )
    return status;
  uint32_t entry_count, enum_entry_count, enum_list_count, flist_count,
           fid_count, rwf_count = 0, ign;
  if ( (status = sass3_find_uint( mi, entry_count_f, sizeof( entry_count_f ),
                                  entry_count )) != 0 ||
       (status = sass3_find_uint( mi, enum_entry_count_f,
                            sizeof( enum_entry_count_f ), enum_entry_count )) != 0 ||
       (status = sass3_find_uint( mi, enum_list_count_f,
                            sizeof( enum_list_count_f ), enum_list_count )) != 0 ||
       (status = sass3_find_uint( mi, flist_count_f, sizeof( flist_count_f ),
                                  flist_count )) != 0 ||
       (status = sass3_find_uint( mi, fid_count_f, sizeof( fid_count_f ),
                                  fid_count )) != 0 ||
       (status = sass3_find_uint( mi, max_fid_f, sizeof( max_fid_f ),
                                  ign )) != 0 ) {
    fprintf( stderr, "Missing MARKETFEED count field: %d\n", status );
    return status;
  }
  sass3_find_uint( mi, rwf_type_count_f, sizeof( rwf_type_count_f ),
                   rwf_count );

  MDMsgMem   tmp;
  Sass3Rec * arec = (Sass3Rec *) tmp.make( ( entry_count + 1 ) * sizeof( Sass3Rec ) ),
           * erec = (Sass3Rec *) tmp.make( ( enum_entry_count + 1 ) * sizeof( Sass3Rec ) ),
           * lrec = (Sass3Rec *) tmp.make( ( enum_list_count + 1 ) * sizeof( Sass3Rec ) ),
           * mrec = (Sass3Rec *) tmp.make( ( flist_count + fid_count + 1 ) * sizeof( Sass3Rec ) ),
           * wrec = (Sass3Rec *) tmp.make( ( entry_count + 1 ) * sizeof( Sass3Rec ) );
  uint32_t   na = 0, ne = 0, nl = 0, nm = 0, nw = 0;

  status = mi->first();
  while ( status == 0 ) {
    if ( (status = mi->get_name( name )) != 0 ||
         (status = mi->get_reference( mref )) != 0 )
      break;
    if ( name.fnamelen == 2 && mref.ftype == MD_OPAQUE ) {
      Sass3Rec r = { mref.fptr, mref.fsize };
      switch ( name.fname[ 0 ] ) {
        case 'a': if ( na < entry_count && r.len >= 14 ) arec[ na++ ] = r; break;
        case 'e': if ( ne < enum_entry_count && r.len >= 5 ) erec[ ne++ ] = r; break;
        case 'l': if ( nl < enum_list_count && r.len >= 8 ) lrec[ nl++ ] = r; break;
        case 'm': if ( nm < flist_count + fid_count && r.len >= 3 ) mrec[ nm++ ] = r; break;
        case 'w': if ( nw < entry_count && r.len >= 6 ) wrec[ nw++ ] = r; break;
        default: break;
      }
    }
    status = mi->next();
  }
  if ( status != Err::NOT_FOUND ) {
    fprintf( stderr, "Error iterating MARKETFEED msg: %d\n", status );
    return status;
  }
  if ( na != entry_count || ne != enum_entry_count || nl != enum_list_count ||
       nm != flist_count + fid_count || ( rwf_count != 0 && nw != entry_count ) ) {
    fprintf( stderr, "MARKETFEED record counts mismatch: a %u/%u e %u/%u "
             "l %u/%u m %u/%u w %u/%u\n", na, entry_count, ne, enum_entry_count,
             nl, enum_list_count, nm, flist_count + fid_count, nw,
             rwf_count != 0 ? entry_count : 0 );
    return Err::BAD_FORMAT;
  }
  /* a : entries, ripple is a fid on the wire, a name in the dict */
  int32_t min_fid = 0x7fff, max_fid = -0x8000;
  for ( i = 0; i < na; i++ ) {
    int16_t fid = (int16_t) be_get16( arec[ i ].p );
    if ( fid < min_fid ) min_fid = fid;
    if ( fid > max_fid ) max_fid = fid;
  }
  const char ** acro_by_fid = NULL;
  if ( na > 0 ) {
    acro_by_fid = (const char **)
      tmp.make( ( max_fid - min_fid + 1 ) * sizeof( char * ) );
    ::memset( acro_by_fid, 0, ( max_fid - min_fid + 1 ) * sizeof( char * ) );
    for ( i = 0; i < na; i++ ) {
      int16_t fid = (int16_t) be_get16( arec[ i ].p );
      acro_by_fid[ fid - min_fid ] = (const char *) &arec[ i ].p[ 12 ];
    }
  }
  for ( i = 0; i < na; i++ ) {
    const uint8_t * p = arec[ i ].p;
    int16_t   fid     = (int16_t) be_get16( p ),
              ripple  = (int16_t) be_get16( &p[ 2 ] );
    uint8_t   ftype   = p[ 10 ],
              dde_off = p[ 11 ];
    MDDictAdd a;
    a.fid      = fid;
    a.fname    = (const char *) &p[ 12 ];
    a.name     = (const char *) &p[ 12 + dde_off ];
    a.mf_len   = be_get16( &p[ 4 ] );
    a.enum_len = be_get16( &p[ 6 ] );
    a.mf_type  = (uint8_t) ( ftype < sizeof( wire_mf_to_md ) ?
                             wire_mf_to_md[ ftype ] : (int8_t) MF_NONE );
    a.rwf_type = RWF_NONE;
    a.filename = "msg";
    a.lineno   = i + 1;
    if ( ripple != 0 && ripple >= min_fid && ripple <= max_fid )
      a.ripple = acro_by_fid[ ripple - min_fid ];
    if ( nw == entry_count ) {
      const uint8_t * w = wrec[ i ].p;
      if ( (int16_t) be_get16( w ) == fid ) {
        a.rwf_len  = be_get16( &w[ 2 ] );
        a.rwf_type = w[ 4 ];
      }
    }
    AppA::rwf_to_md_type_size( a );
    if ( (status = dict_build.add_entry( a )) != 0 )
      fprintf( stderr, "Bad app_a entry: %s fid %d\n", a.fname, fid );
  }
  /* l + e : enum maps, list index + 1 == map_num == fenum */
  struct EnumTmp {
    const uint8_t * fids, * values;
    uint8_t       * map;
    uint16_t      * value;
    uint32_t        cnt;
    uint16_t        fid_count, value_count, max_value, max_len;
    bool            dense;
  };
  EnumTmp * el = (EnumTmp *) tmp.make( ( nl + 1 ) * sizeof( EnumTmp ) );
  ::memset( el, 0, ( nl + 1 ) * sizeof( EnumTmp ) );
  for ( i = 0; i < nl; i++ ) {
    const uint8_t * p = lrec[ i ].p;
    el[ i ].fid_count   = be_get16( p );
    el[ i ].value_count = be_get16( &p[ 2 ] );
    el[ i ].max_value   = be_get16( &p[ 6 ] );
    el[ i ].fids        = &p[ 8 ];
    el[ i ].values      = &p[ 8 + 2 * el[ i ].fid_count ];
    if ( lrec[ i ].len < 8 + 2 * ( (size_t) el[ i ].fid_count +
                                   el[ i ].value_count ) ) {
      fprintf( stderr, "MARKETFEED enum list %u truncated\n", i );
      return Err::BAD_FORMAT;
    }
  }
  for ( i = 0; i < ne; i++ ) {
    uint16_t fenum = be_get16( erec[ i ].p );
    if ( fenum >= 1 && fenum <= nl ) {
      size_t len = ::strnlen( (const char *) &erec[ i ].p[ 4 ],
                              erec[ i ].len - 4 );
      if ( len > el[ fenum - 1 ].max_len )
        el[ fenum - 1 ].max_len = (uint16_t) len;
    }
  }
  for ( i = 0; i < nl; i++ ) {
    EnumTmp & e = el[ i ];
    e.dense = ( (size_t) e.max_value + 1 < (size_t) e.value_count * 2 );
    e.cnt   = ( e.dense ? (uint32_t) e.max_value + 1 : e.value_count );
    e.map   = (uint8_t *) tmp.make( e.cnt * ( e.max_len + 1 ) );
    ::memset( e.map, ' ', e.cnt * ( e.max_len + 1 ) );
    if ( ! e.dense ) {
      e.value = (uint16_t *) tmp.make( e.cnt * sizeof( uint16_t ) );
      for ( j = 0; j < e.cnt; j++ )
        e.value[ j ] = be_get16( &e.values[ j * 2 ] );
    }
  }
  for ( i = 0; i < ne; i++ ) {
    const uint8_t * p = erec[ i ].p;
    uint16_t fenum = be_get16( p ),
             val   = be_get16( &p[ 2 ] );
    if ( fenum < 1 || fenum > nl )
      continue;
    EnumTmp & e   = el[ fenum - 1 ];
    size_t    len = ::strnlen( (const char *) &p[ 4 ], erec[ i ].len - 4 ),
              idx = e.cnt;
    if ( e.dense ) {
      if ( val <= e.max_value )
        idx = val;
    }
    else { /* values are sorted */
      size_t lo = 0, hi = e.cnt;
      while ( lo < hi ) {
        size_t mid = ( lo + hi ) / 2;
        if ( e.value[ mid ] < val )      lo = mid + 1;
        else if ( e.value[ mid ] > val ) hi = mid;
        else { idx = mid; break; }
      }
    }
    if ( idx < e.cnt && len <= e.max_len )
      ::memcpy( &e.map[ idx * e.max_len ], &p[ 4 ], len );
  }
  for ( i = 0; i < nl; i++ ) {
    EnumTmp & e = el[ i ];
    MDEnumAdd ea;
    ea.map_num   = i + 1;
    ea.value_cnt = e.cnt;
    ea.max_value = e.max_value;
    ea.max_len   = e.max_len;
    ea.value     = e.value;
    ea.map       = e.map;
    dict_build.add_enum_map( ea );
    for ( j = 0; j < e.fid_count; j++ )
      dict_build.update_entry_enum( (int16_t) be_get16( &e.fids[ j * 2 ] ),
                                    i + 1, e.max_len );
  }
  if ( na > 0 ) {
    dict_build.index_dict( "app_a", dict );
    dict_build.clear_build();
  }
  /* m : flist maps (fid maps have no raimd equivalent, ignored) */
  if ( flist_count > 0 ) {
    for ( i = 0; i < flist_count; i++ ) {
      const uint8_t * p = mrec[ i ].p;
      MDDictAdd a;
      a.fid      = (int16_t) be_get16( p );
      a.fname    = (const char *) &p[ 2 ];
      a.ftype    = MD_MESSAGE;
      a.filename = "msg";
      a.lineno   = i + 1;
      dict_build.add_entry( a );
    }
    dict_build.index_dict( "flist", dict );
    dict_build.clear_build();
  }
  return 0;
}

