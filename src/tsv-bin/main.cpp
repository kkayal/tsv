#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "tsvlib.h"
#include "util.h"

using namespace std;

//
// Main
//

int main( int argc, const char** argv ) {
  try {
    cout << boolalpha;  // I want to see 'true' and 'false' instead of '1' and '0'

    // Check if there is input from a pipe.
    bool source_from_pipe = false;
    string source;
    if ( !isatty( STDIN_FILENO ) ) {
      // STDIN_FILENO is **not** a tty. That means not a terminal and
      // that means it could be piped by some other program to this one
      source_from_pipe = true;
      string lineInput;
      stringstream ss;
      while ( getline( cin, lineInput ) ) {
        ss << lineInput << endl;
      }
      source = ss.str();
    }

    // If the source code is available from a pipe, we don't need a source file and
    // have to consider one less argc
    size_t n = ( source_from_pipe ) ? 1 : 0;

    // Parser commandline parameters
    const char* path = ( source_from_pipe ) ? "Inline" : argv[1];
    bool print_trace = false;
    bool print_ast   = false;

    if ( argc == 1 - n ) {
      cout << endl;
      cout << tsv_help << endl;
      return 0;
    } else if ( argc == 2 - n && string( "-h" ) == argv[1 - n] ) {
      cout << tsv_help << endl;
      return 0;
    } else if ( argc == 2 - n && string( "--version" ) == argv[1 - n] ) {
      cout << tsv_version << endl;
      return 0;
    }

    auto arg = 2 - n;
    while ( arg < argc ) {
      if ( string( "--ast" ) == argv[arg] ) {
        print_ast = true;
      } else if ( string( "--trace" ) == argv[arg] ) {
        print_trace = true;
      }
      arg++;
    }

    // Read a source file into memory
    if ( !source_from_pipe ) {
      source = getFileContents( path );
    };

    stringstream out;
    stringstream err;

    auto result = tsv_to_md( source, path, out, err, print_ast, print_trace );

    if ( result.code == 0 ) {
      cout << out.str() << flush;
    } else {
      cout << result.msg << endl;
    }

    return result.code;
  } catch ( const runtime_error& e ) {
    cerr << e.what();
  }
  return -1;
}