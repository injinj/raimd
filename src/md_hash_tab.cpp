#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <raimd/md_hash_tab.h>


extern "C" {

void
md_subject_key_init( MDSubjectKey_t *k, const char *s, size_t l )
{
  k->subj = s;
  k->len  = l;
}

bool
md_subject_key_equals( const MDSubjectKey_t *k,  const MDSubjectKey_t *x )
{
  return ((const rai::md::MDSubjectKey *) k)->equals( *x );
}

size_t
md_subject_key_hash( const MDSubjectKey_t *k )
{
  return ((const rai::md::MDSubjectKey *) k)->hash();
}

void
md_hash_tab_init( MDHashTab_t *tab,  size_t ini_sz,  MDHashTab_key_f key )
{
  tab->tab        = NULL;
  tab->tab_mask   = 0;
  tab->elem_count = 0;
  tab->get_key    = key;
  if ( ini_sz != 0 ) {
    tab->tab = (void **) ::malloc( sizeof( void * ) * ini_sz );
    ::memset( tab->tab, 0, sizeof( void * ) * ini_sz );
    tab->tab_mask = ini_sz - 1;
  }
}

void
md_hash_tab_release( MDHashTab_t *tab )
{
  if ( tab->tab != NULL )
    free( tab->tab );
  tab->tab        = NULL;
  tab->tab_mask   = 0;
  tab->elem_count = 0;
}

bool
md_hash_tab_locate( MDHashTab_t *tab,  const MDSubjectKey_t *k,  size_t *pos )
{
  size_t h = ((rai::md::MDSubjectKey *) k)->hash();
  rai::md::MDSubjectKey k2( 0, 0 );
  size_t i;
  for ( i = h & tab->tab_mask; ; i = ( i + 1 ) & tab->tab_mask ) {
    if ( tab->tab[ i ] == NULL ) {
      if ( pos != NULL ) *pos = i;
      return false;
    }
    if ( pos != NULL ) *pos = i;
    tab->get_key( tab->tab[ i ], &k2 );
    if ( k2.equals( *k ) )
      return true;
  }
}

void *
md_hash_tab_find( MDHashTab_t *tab,  const MDSubjectKey_t *k,  size_t *pos )
{
  size_t i;
  if ( pos == NULL )
    pos = &i;
  if ( md_hash_tab_locate( tab, k, pos ) )
    return tab->tab[ *pos ];
  return NULL;
}

void *
md_hash_tab_insert( MDHashTab_t *tab,  size_t pos,  void *value )
{
  void *old = md_hash_tab_swap( tab, pos, value );
  if ( old == NULL ) {
    if ( ++tab->elem_count > md_hash_tab_max_size( tab ) )
      md_hash_tab_grow( tab );
  }
  return old;
}

void *
md_hash_tab_upsert( MDHashTab_t *tab,  const MDSubjectKey_t *k,  void *value )
{
  size_t pos;
  md_hash_tab_locate( tab, k, &pos );
  return md_hash_tab_insert( tab, pos, value );
}

void *
md_hash_tab_remove( MDHashTab_t *tab,  const MDSubjectKey_t *k )
{
  size_t pos;
  if ( md_hash_tab_find( tab, k, &pos ) ) {
    void *old = md_hash_tab_swap( tab, pos, NULL );
    md_hash_tab_fill_empty_hole( tab, pos );
    if ( --tab->elem_count < md_hash_tab_min_size( tab ) )
      md_hash_tab_shrink( tab );
    return old;
  }
  return NULL;
}

size_t
md_hash_tab_min_size( MDHashTab_t *tab )
{
  size_t sz = tab->tab_mask + 1;
  return sz / 2 - sz / 4;
}

size_t
md_hash_tab_max_size( MDHashTab_t *tab )
{
  size_t sz = tab->tab_mask + 1;
  return sz / 2 + sz / 4;
}

void *
md_hash_tab_swap( MDHashTab_t *tab,  size_t pos,  void *val )
{
  void *old = tab->tab[ pos ];
  tab->tab[ pos ] = val;
  return old;
}

void
md_hash_tab_fill_empty_hole( MDHashTab_t *tab,  size_t pos )
{
  rai::md::MDSubjectKey k2( 0, 0 );
  for (;;) {
    pos = ( pos + 1 ) & tab->tab_mask;
    if ( tab->tab[ pos ] == NULL )
      return;
    tab->get_key( tab->tab[ pos ], &k2 );
    size_t i = k2.hash() & tab->tab_mask;
    if ( pos != i ) {
      void *sav = md_hash_tab_swap( tab, pos, NULL );
      while ( tab->tab[ i ] != NULL )
        i = ( i + 1 ) & tab->tab_mask;
      tab->tab[ i ] = sav;
    }
  }
}

void
md_hash_tab_grow( MDHashTab_t *tab )
{
  md_hash_tab_resize( tab, ( tab->tab_mask + 1 ) * 2 );
}

void
md_hash_tab_shrink( MDHashTab_t *tab )
{
  if ( tab->tab_mask > 3 )
    md_hash_tab_resize( tab, ( tab->tab_mask + 1 ) / 2 );
}

void
md_hash_tab_resize( MDHashTab_t *tab,  size_t new_sz )
{
  MDHashTab_t tmp;
  rai::md::MDSubjectKey k2( 0, 0 );
  md_hash_tab_init( &tmp, new_sz, tab->get_key );
  size_t count = tab->elem_count;
  for ( size_t pos = 0; count > 0; pos++ ) {
    if ( tab->tab[ pos ] == NULL )
      continue;
    tab->get_key( tab->tab[ pos ], &k2 );
    size_t new_pos = k2.hash() & tmp.tab_mask;
    for ( ; tmp.tab[ new_pos ] != NULL; new_pos = ( new_pos + 1 ) & tmp.tab_mask )
      ;
    tmp.tab[ new_pos ] = tab->tab[ pos ];
    count--;
  }
  ::free( tab->tab );
  tab->tab      = tmp.tab;
  tmp.tab       = NULL;
  tab->tab_mask = tmp.tab_mask;
}

}

