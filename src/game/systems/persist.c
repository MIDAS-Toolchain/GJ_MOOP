#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persist.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void PersistInit( void ) { /* no-op */ }

int PersistSave( const char* key, const char* data )
{
  EM_ASM({
    try {
      localStorage.setItem( "odd_" + UTF8ToString( $0 ), UTF8ToString( $1 ) );
    } catch( e ) { return; }
  }, key, data );
  return 0;
}

char* PersistLoad( const char* key )
{
  char* result = (char*)EM_ASM_PTR({
    var val = localStorage.getItem( "odd_" + UTF8ToString( $0 ) );
    if ( !val ) return 0;
    var len = lengthBytesUTF8( val ) + 1;
    var buf = _malloc( len );
    stringToUTF8( val, buf, len );
    return buf;
  }, key );
  return result;
}

int PersistDelete( const char* key )
{
  EM_ASM({
    localStorage.removeItem( "odd_" + UTF8ToString( $0 ) );
  }, key );
  return 0;
}

#else /* NATIVE */

#include <sys/stat.h>

#define SAVE_DIR  "saves"
#define MAX_PATH  256

static void build_path( const char* key, char* out, int max )
{
  snprintf( out, max, "%s/%s.txt", SAVE_DIR, key );
}

void PersistInit( void )
{
  struct stat st;
  if ( stat( SAVE_DIR, &st ) == -1 )
    mkdir( SAVE_DIR, 0755 );
}

int PersistSave( const char* key, const char* data )
{
  char path[MAX_PATH];
  build_path( key, path, MAX_PATH );

  FILE* f = fopen( path, "w" );
  if ( !f ) return 1;

  fputs( data, f );
  fclose( f );
  return 0;
}

char* PersistLoad( const char* key )
{
  char path[MAX_PATH];
  build_path( key, path, MAX_PATH );

  FILE* f = fopen( path, "r" );
  if ( !f ) return NULL;

  fseek( f, 0, SEEK_END );
  long len = ftell( f );
  fseek( f, 0, SEEK_SET );

  char* buf = malloc( len + 1 );
  if ( !buf ) { fclose( f ); return NULL; }

  fread( buf, 1, len, f );
  buf[len] = '\0';
  fclose( f );
  return buf;
}

int PersistDelete( const char* key )
{
  char path[MAX_PATH];
  build_path( key, path, MAX_PATH );
  return remove( path ) == 0 ? 0 : 1;
}

#endif
