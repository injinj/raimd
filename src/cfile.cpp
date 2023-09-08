#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/cfile.h>
#include <raimd/md_msg.h>
#include <raimd/tib_sass_msg.h>

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

