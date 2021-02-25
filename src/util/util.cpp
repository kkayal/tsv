#include "util.h"

#include <fstream>
#include <sstream>

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

string indent( size_t level, size_t tab_size ) {
  stringstream ss;
  for ( int i = 0; i < level; i++ ) {
    for ( int i = 0; i < tab_size; i++ ) {
      ss << " ";
    }
  }
  return ss.str();
}

// Copied from cpp_peg at https://github.com/yhirose/cpp-peglib
void trace_parser( parser parser ) {
  size_t prev_pos = 0;
  parser.enable_trace(
      [&]( const peg::Ope &ope, const char *s, size_t /*n*/, const peg::SemanticValues & /*sv*/,
           const peg::Context &c, const std::any & /*dt*/ ) {
        auto pos       = static_cast<size_t>( s - c.s );
        auto backtrack = ( pos < prev_pos ? "*" : "" );
        string indent;
        auto level = c.trace_ids.size() - 1;
        while ( level-- ) {
          indent += "│";
        }
        std::string name;
        {
          name = peg::TraceOpeName::get( const_cast<peg::Ope &>( ope ) );

          auto lit = dynamic_cast<const peg::LiteralString *>( &ope );
          if ( lit ) {
            name += " '" + peg::escape_characters( lit->lit_ ) + "'";
          }
        }
        std::cout << "E " << pos << backtrack << "\t" << indent << "┌" << name << " #"
                  << c.trace_ids.back() << std::endl;
        prev_pos = static_cast<size_t>( pos );
      },
      [&]( const peg::Ope &ope, const char *s, size_t /*n*/, const peg::SemanticValues &sv,
           const peg::Context &c, const std::any & /*dt*/, size_t len ) {
        auto pos = static_cast<size_t>( s - c.s );
        if ( len != static_cast<size_t>( -1 ) ) {
          pos += len;
        }
        string indent;
        auto level = c.trace_ids.size() - 1;
        while ( level-- ) {
          indent += "│";
        }
        auto ret  = len != static_cast<size_t>( -1 ) ? "└o " : "└x ";
        auto name = peg::TraceOpeName::get( const_cast<peg::Ope &>( ope ) );
        std::stringstream choice;
        if ( sv.choice_count() > 0 ) {
          choice << " " << sv.choice() << "/" << sv.choice_count();
        }
        std::string token;
        if ( !sv.tokens.empty() ) {
          token += ", token '";
          token += sv.tokens[0];
          token += "'";
        }
        std::string matched;
        if ( peg::success( len ) && peg::TokenChecker::is_token( const_cast<peg::Ope &>( ope ) ) ) {
          matched = ", match '" + peg::escape_characters( s, len ) + "'";
        }
        std::cout << "L " << pos << "\t" << indent << ret << name << " #" << c.trace_ids.back()
                  << choice.str() << token << matched << std::endl;
      } );
}
