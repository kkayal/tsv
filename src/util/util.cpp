#include "util.h"

std::string getFileContents( const char *filename ) {
  std::ifstream in( filename, std::ios::in | std::ios::binary );
  if ( in ) {
    std::string contents;
    in.seekg( 0, std::ios::end );
    contents.resize( in.tellg() );
    in.seekg( 0, std::ios::beg );
    in.read( &contents[0], contents.size() );
    in.close();
    return ( contents );
  }
  std::stringstream ss;
  ss << "Unable to read '" << filename << "' : Errno: " << errno;
  throw std::runtime_error( ss.str() );
}

std::string indent( size_t level, size_t tab_size ) {
  std::stringstream ss;
  for ( int i = 0; i < level; i++ ) {
    for ( int i = 0; i < tab_size; i++ ) {
      ss << " ";
    }
  }
  return ss.str();
}

size_t count_ut8_codepoints(const char *s) {
  size_t len = 0;
  while ( *s ) len += ( *s++ & 0xc0 ) != 0x80;
  return len;
}

size_t count_ut8_codepoints( const std::string_view str ) {
  size_t len = 0;
  const char *s = str.data();
  for (size_t i = 0; i < str.length(); i++ ) len += ( *s++ & 0xc0 ) != 0x80;
  return len;
}
