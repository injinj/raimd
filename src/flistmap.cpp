#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <raimd/md_dict.h>
#include <raimd/flistmap.h>

using namespace rai;
using namespace md;

FlistMapTok
FlistMap::get_token( void ) noexcept
{
  int c;
  for (;;) {
    while ( (c = this->eat_white()) == '#' )
      this->eat_comment();

    this->col++;
    switch ( c ) {
      case '-':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        return this->consume_int();
      default:
        if ( isalpha( c ) )
          return this->consume_ident();
        break;
      case EOF_CHAR:
        return FLIST_EOF;
    }
    return this->consume( FLIST_ERROR, 1 );
  }
}

void
FlistMap::clear_line( void ) noexcept
{
  this->fid            = 0;
  this->formclass[ 0 ] = '\0';
}

FlistMap *
FlistMap::open_path( const char *path,  const char *filename,
                 int debug_flags ) noexcept
{
  char path2[ 1024 ];
  if ( DictParser::find_file( path, filename, ::strlen( filename ),
                              path2 ) ) {
    void * p = ::malloc( sizeof( FlistMap ) );
    return new ( p ) FlistMap( path2, debug_flags );
  }
  return NULL;
}

int
FlistMap::parse_path( MDDictBuild &dict_build,  const char *path,
                      const char *fn ) noexcept
{
  FlistMap * p = NULL;
  int ret = 0, curlineno = 0;

  p = FlistMap::open_path( path, fn, dict_build.debug_flags );
  if ( p == NULL ) {
    fprintf( stderr, "\"%s\": file not found\n", fn );
    return Err::FILE_NOT_FOUND;
  }
  while ( p != NULL ) {
    FlistMapTok tok = p->get_token();
    if ( p->lineno != curlineno ) {
      if ( curlineno != 0 ) {
        if ( p->fid != 0 && p->formclass[ 0 ] != '\0' ) {
          MDDictAdd a;
          a.fid      = p->fid;
          a.fname    = p->formclass;
          a.ftype    = MD_MESSAGE;
          a.filename = p->fname;
          a.lineno   = curlineno;
          dict_build.add_entry( a );
        }
      }
      curlineno = p->lineno;
      p->clear_line();
    }
    /* fid formclass */
    switch ( tok ) {
      case FLIST_INT:
        if ( p->col == 1 ) {
          p->tok_buf[ p->tok_sz ] = '\0';
          p->fid = atoi( p->tok_buf );
          if ( p->fid != 0 )
            break;
        }
        /* FALLTHRU */
      case FLIST_IDENT:
        if ( p->col == 2 ) {
          size_t len = ( p->tok_sz > 255 ? 255 : p->tok_sz );
          ::memcpy( p->formclass, p->tok_buf, len );
          p->formclass[ len ] = '\0';
          break;
        }
        /* FALLTHRU */

      case FLIST_ERROR:
        fprintf( stderr, "error at \"%s\" line %u: \"%.*s\"\n",
                 p->fname, p->lineno, (int) p->tok_sz, p->tok_buf );
        if ( ret == 0 )
          ret = Err::DICT_PARSE_ERROR;
        /* FALLTHRU */
      case FLIST_EOF:
        delete p;
        p = NULL;
        break;
    }
  }
  return ret;
}

