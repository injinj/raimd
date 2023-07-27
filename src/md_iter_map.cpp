#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <raimd/md_field_iter.h>
#include <raimd/md_msg.h>

using namespace rai;
using namespace md;

bool
MDIterMap::index_array( size_t i,  void *&ptr,  size_t &sz ) noexcept
{
  sz  = this->elem_fsize;
  ptr = this->fptr;

  if ( sz == 0 )
    sz = this->fsize;
  else {
    if ( i * sz >= this->fsize )
      return false;
    ptr = &((uint8_t *) ptr)[ i * sz ];
  }
  return true;
}

bool
MDIterMap::copy_string( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  if ( mref.ftype == MD_STRING || mref.ftype == MD_OPAQUE ) {
    size_t len = sz;
    if ( len > mref.fsize )
      len = mref.fsize;
    ::memcpy( ptr, mref.fptr, len );
    /* null term strings */
    if ( this->ftype == MD_STRING || this->elem_ftype == MD_STRING ) {
      size_t end = sz - 1;
      if ( len < end )
        end = len;
      ((char *) ptr)[ end ] = '\0';
    }
    else if ( len < sz ) /* zero out opaque bufs */
      ::memset( &((char *) ptr)[ len ], 0, sz - len );
  }
  else {
    to_string( mref, (char *) ptr, sz );
  }
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_uint( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  switch ( sz ) {
    case 1: ((uint8_t *)  ptr)[ 0 ] = get_int<uint8_t>( mref );  break;
    case 2: ((uint16_t *) ptr)[ 0 ] = get_int<uint16_t>( mref ); break;
    case 4: ((uint32_t *) ptr)[ 0 ] = get_int<uint32_t>( mref ); break;
    case 8: ((uint64_t *) ptr)[ 0 ] = get_int<uint64_t>( mref ); break;
    default: return false;
  }
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_sint( size_t i,  MDReference &mref ) noexcept
{
  void * ptr;
  size_t sz;
  if ( ! this->index_array( i, ptr, sz ) )
    return false;
  switch ( sz ) {
    case 1: ((int8_t *)  ptr)[ 0 ] = get_int<int8_t>( mref );  break;
    case 2: ((int16_t *) ptr)[ 0 ] = get_int<int16_t>( mref ); break;
    case 4: ((int32_t *) ptr)[ 0 ] = get_int<int32_t>( mref ); break;
    case 8: ((int64_t *) ptr)[ 0 ] = get_int<int64_t>( mref ); break;
    default: return false;
  }
  if ( this->elem_count != NULL )
    this->elem_count[ 0 ]++;
  return true;
}

bool
MDIterMap::copy_array( MDMsg &msg,  MDReference &mref ) noexcept
{
  MDReference aref;
  size_t i, num_entries = mref.fsize;
  bool b = false;
  if ( mref.fentrysz > 0 )
    num_entries /= mref.fentrysz;

  if ( mref.fentrysz != 0 ) {
    aref.zero();
    aref.ftype   = mref.fentrytp;
    aref.fsize   = mref.fentrysz;
    aref.fendian = mref.fendian;
    for ( i = 0; i < num_entries; i++ ) {
      aref.fptr = &mref.fptr[ i * (size_t) mref.fentrysz ];
      if ( this->elem_ftype == MD_STRING || this->elem_ftype == MD_OPAQUE )
        b |= this->copy_string( i, aref );
      else if ( this->elem_ftype == MD_UINT )
        b |= this->copy_uint( i, aref );
      else if ( this->elem_ftype == MD_INT )
        b |= this->copy_sint( i, aref );
    }
    return b;
  }
  for ( i = 0; i < num_entries; i++ ) {
    if ( msg.get_array_ref( mref, i, aref ) == 0 ) {
      if ( this->elem_ftype == MD_STRING || this->elem_ftype == MD_OPAQUE )
        b |= this->copy_string( i, aref );
      else if ( this->elem_ftype == MD_UINT )
        b |= this->copy_uint( i, aref );
      else if ( this->elem_ftype == MD_INT )
        b |= this->copy_sint( i, aref );
    }
  }
  return b;
}

size_t
MDIterMap::get_map( MDMsg &msg,  MDIterMap *map,  size_t n,
                    MDFieldIter *iter ) noexcept
{
  MDReference mref;
  size_t count = 0;
  int    status;
  if ( iter == NULL ) {
    if ( msg.get_field_iter( iter ) != 0 )
      return 0;
  }
  for ( size_t i = 0; i < n; i++ ) {
    if ( (status = iter->find( map[ i ].fname, map[ i ].fname_len,
                               mref )) == 0 ) {
      bool b = false;
      if ( map[ i ].ftype == MD_STRING || map[ i ].ftype == MD_OPAQUE )
        b = map[ i ].copy_string( 0, mref );
      else if ( map[ i ].ftype == MD_UINT )
        b = map[ i ].copy_uint( 0, mref );
      else if ( map[ i ].ftype == MD_INT )
        b = map[ i ].copy_sint( 0, mref );
      else if ( map[ i ].ftype == MD_ARRAY ) {
        if ( mref.ftype == MD_ARRAY )
          b = map[ i ].copy_array( msg, mref );
      }
      if ( b )
        count++;
    }
  }
  return count;
}
