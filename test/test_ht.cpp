#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <raimd/md_hash_tab.h>

using namespace rai;
using namespace md;

struct TestKey {
  const char * str;
  size_t       len;

  size_t hash( void ) const {
    size_t key = 5381;
    for ( size_t i = 0; i < this->len; i++ ) {
      size_t c = (size_t) (uint8_t) this->str[ i ];
      key = c ^ ( ( key << 5 ) + key );
    }
    return key;
  }
  bool equals( const TestKey &k ) const {
    return k.len == this->len &&
           ::memcmp( k.str, this->str, this->len ) == 0;
  }
  TestKey( const char *s,  size_t l ) : str( s ), len( l ) {}
};

struct TestElem : public TestKey {
  int data;
  void * operator new( size_t, void *ptr ) { return ptr; }
  TestElem( const char *s,  int i ) : TestKey( s, ::strlen( s ) ), data( i ) {}
};

typedef MDHashTabT<TestKey, TestElem> TestHT;

int
main( void )
{
  const char * k[ 20 ] = {
    "one", "two", "three", "four", "five", "six", "seven",
    "eight", "nine", "ten", "eleven", "twelve", "thirteen",
    "fourteen", "fifteen", "sixteen", "seventeen",
    "eighteen", "nineteen", "twenty" };

  TestElem * els[ 20 ];
  TestHT ht;

  for ( int i = 0; i < 20; i++ ) {
    els[ i ] = new ( ::malloc( sizeof( TestElem ) ) ) TestElem( k[ i ], i );
    ht.upsert( *els[ i ], els[ i ] );
  }
  for ( int i = 0; i < 20; i++ ) {
    if ( ! ht.find( *els[ i ] ) )
      printf( "%d not found\n", i );
  }
  for ( int i = 0; i < 20; i += 2 ) {
    if ( ! ht.remove( *els[ i ] ) )
      printf( "%d not removed\n", i );
  }
  for ( int i = 0; i < 20; i++ ) {
    if ( ! ht.find( *els[ i ] ) ) {
      if ( i % 2 == 1 )
        printf( "%d not found 2\n", i );
    }
    else {
      if ( i % 2 == 0 )
        printf( "%d found 2\n", i );
    }
  }

  return 0;
}

