#ifndef __rai_raimd__md_hash_tab_h__
#define __rai_raimd__md_hash_tab_h__

namespace rai {
namespace md {

template <class Key, class Value>
struct MDHashTabT {
  Value ** tab;
  size_t   tab_mask,
           elem_count;

  MDHashTabT( size_t ini_sz = 4 )
      : tab( 0 ), tab_mask( 0 ), elem_count( 0 ) {
    this->tab = (Value **) ::malloc( sizeof( Value * ) * ini_sz );
    ::memset( this->tab, 0, sizeof( Value * ) * ini_sz );
    this->tab_mask = ini_sz - 1;
  }
  ~MDHashTabT() {
    if ( this->tab != NULL )
      ::free( this->tab );
  }

  size_t min_size( void ) const {
    size_t sz = this->tab_mask + 1;
    return sz / 2 - sz / 4;
  }
  size_t max_size( void ) const {
    size_t sz = this->tab_mask + 1;
    return sz / 2 + sz / 4;
  }

  Value *swap( size_t pos,  Value *val ) {
    Value *old = this->tab[ pos ];
    this->tab[ pos ] = val;
    return old;
  }

  bool locate( const Key &h,  size_t &pos ) {
    for ( pos = h.hash() & this->tab_mask; ; pos = ( pos + 1 ) & this->tab_mask ) {
      if ( this->tab[ pos ] == NULL )
        return false;
      if ( h.equals( *this->tab[ pos ] ) )
        return true;
    }
  }

  Value *find( const Key &h,  size_t &pos ) {
    if ( this->locate( h, pos ) )
      return this->tab[ pos ];
    return NULL;
  }

  Value *find( const Key &h ) {
    size_t pos;
    if ( this->locate( h, pos ) )
      return this->tab[ pos ];
    return NULL;
  }

  Value *insert( size_t pos,  Value *value ) {
    Value *old = this->swap( pos, value );
    if ( old == NULL ) {
      if ( ++this->elem_count > this->max_size() )
        this->grow();
    }
    return old;
  }

  Value *upsert( Key &h,  Value *value ) {
    size_t pos;
    this->locate( h, pos );
    return this->insert( pos, value );
  }

  Value *remove( Key &h ) {
    size_t pos;
    if ( this->find( h, pos ) ) {
      Value *old = this->swap( pos, NULL );
      this->fill_empty_hole( pos );
      if ( --this->elem_count < this->min_size() )
        this->shrink();
      return old;
    }
    return NULL;
  }

  void fill_empty_hole( size_t pos ) {
    for (;;) {
      pos = ( pos + 1 ) & this->tab_mask;
      if ( this->tab[ pos ] == NULL )
        return;
      size_t i = this->tab[ pos ]->hash() & this->tab_mask;
      if ( pos != i ) {
        Value *sav = this->swap( pos, NULL );
        while ( this->tab[ i ] != NULL )
          i = ( i + 1 ) & this->tab_mask;
        this->tab[ i ] = sav;
      }
    }
  }

  void grow( void ) {
    this->resize( ( this->tab_mask + 1 ) * 2 );
  }

  void shrink( void ) {
    if ( this->tab_mask > 3 )
      this->resize( ( this->tab_mask + 1 ) / 2 );
  }

  void resize( size_t new_sz ) {
    MDHashTabT tmp( new_sz );
    size_t count = this->elem_count;
    for ( size_t pos = 0; count > 0; pos++ ) {
      if ( this->tab[ pos ] == NULL )
        continue;
      size_t new_pos = this->tab[ pos ]->hash() & tmp.tab_mask;
      for ( ; tmp.tab[ new_pos ] != NULL; new_pos = ( new_pos + 1 ) & tmp.tab_mask )
        ;
      tmp.tab[ new_pos ] = this->tab[ pos ];
      count--;
    }
    ::free( this->tab );
    this->tab      = tmp.tab;
    tmp.tab        = NULL;
    this->tab_mask = tmp.tab_mask;
  }
};

}
}
#endif
