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
