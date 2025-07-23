#ifndef __rai_raimd__md_hash_tab_h__
#define __rai_raimd__md_hash_tab_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MDSubjectKey_s {
  const char * subj;
  size_t       len;
} MDSubjectKey_t;

void md_subject_key_init( MDSubjectKey_t *k, const char *s, size_t l );
bool md_subject_key_equals( const MDSubjectKey_t *k,  const MDSubjectKey_t *x );
size_t md_subject_key_hash( const MDSubjectKey_t *k );

typedef void (*MDHashTab_key_f)( void *value,  MDSubjectKey_t *k );

typedef struct MDHashTab_s {
  void         ** tab;
  size_t          tab_mask,
                  elem_count;
  MDHashTab_key_f get_key;
} MDHashTab_t;

bool md_hash_tab_create( MDHashTab_t **tab,  size_t ini_sz,  MDHashTab_key_f key );
void md_hash_tab_destroy( MDHashTab_t *tab );
void md_hash_tab_init( MDHashTab_t *tab,  size_t ini_sz,  MDHashTab_key_f key );
void md_hash_tab_release( MDHashTab_t *tab );
bool md_hash_tab_locate( MDHashTab_t *tab,  const MDSubjectKey_t *k,  size_t *pos );
void * md_hash_tab_find( MDHashTab_t *tab,  const MDSubjectKey_t *k,  size_t *pos );
void * md_hash_tab_insert( MDHashTab_t *tab,  size_t pos,  void *value );
void * md_hash_tab_upsert( MDHashTab_t *tab,  const MDSubjectKey_t *k,  void *value );
void * md_hash_tab_remove( MDHashTab_t *tab,  const MDSubjectKey_t *k );
size_t md_hash_tab_min_size( MDHashTab_t *tab );
size_t md_hash_tab_max_size( MDHashTab_t *tab );
void * md_hash_tab_swap( MDHashTab_t *tab,  size_t pos,  void *val );
void md_hash_tab_fill_empty_hole( MDHashTab_t *tab,  size_t pos );
void md_hash_tab_grow( MDHashTab_t *tab );
void md_hash_tab_shrink( MDHashTab_t *tab );
void md_hash_tab_resize( MDHashTab_t *tab,  size_t new_sz );

#ifdef __cplusplus
}

namespace rai {
namespace md {

struct MDSubjectKey : public MDSubjectKey_s {
  static inline size_t hash_subject( const char *subj,  size_t len ) {
    size_t key = 5381;
    for ( size_t i = 0; i < len; i++ ) {
      size_t c = (size_t) (uint8_t) subj[ i ];
      key = c ^ ( ( key << 5 ) + key );
    }
    return key;
  }
  size_t hash( void ) const {
    return hash_subject( this->subj, this->len );
  }
  bool equals( const MDSubjectKey_t &k ) const {
    return k.len == this->len &&
           ::memcmp( k.subj, this->subj, this->len ) == 0;
  }
  void init( const char *s,  size_t l ) {
    this->subj = s;
    this->len  = l;
  }
  MDSubjectKey( const char *s,  size_t l ) {
    this->init( s, l );
  }
};

template <class Key, class Value>
struct MDHashTabT {
  Value ** tab;
  size_t   tab_mask,
           elem_count;

  MDHashTabT( size_t ini_sz = 4 )
      : tab( 0 ), tab_mask( 0 ), elem_count( 0 ) {
    if ( ini_sz > 0 )
      this->init( ini_sz );
  }
  void init( size_t ini_sz ) {
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
#endif
