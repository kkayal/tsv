#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "util.h"

using namespace peg;
using namespace peg::udl;
using namespace std;

const char *tsv_version = "0.2.0";
const char *tsv_help    = "Usage: tsv [--version] [-h] INPUT_FILE [--ast] [--trace]";

enum alignmet { no_preference, left, center, right };

/// prints a single table cell to standard output and takes care of
/// padding for the alignment based on column size
void print_cell( string_view token, const alignmet alignment, const size_t &size ) {
  switch ( alignment ) {
    case alignmet::center: {
      auto spaces_left  = ( size - token.size() ) / 2;
      auto spaces_right = size + 1 - token.size() - spaces_left;
      for ( size_t j = 0; j < spaces_left; j++ ) cout << ' ';
      cout << token;
      for ( size_t j = 0; j < spaces_right; j++ ) cout << ' ';
    } break;
    case alignmet::right: {
      for ( size_t j = 0; j < size - token.size(); j++ ) cout << ' ';
      cout << token << ' ';
    } break;
    case alignmet::left: [[fallthrough]];
    case alignmet::no_preference: {
      cout << token;
      for ( size_t j = 0; j < size + 1 - token.size(); j++ ) cout << ' ';
    } break;
    default: break;
  }
}

/// Find the max size of each column
void get_max_col_sizes( const std::shared_ptr<Ast> &ast, vector<size_t> &column_sizes ) {
  size_t i = 0;
  for ( auto cell : ast->nodes ) {
    auto size = cell->token.size();
    if ( size > column_sizes[i] ) {
      column_sizes[i] = size;
    }
    i++;
  }
}

alignmet get_alignment_from_colons( string_view token ) {
  auto first_char = *token.begin();
  auto last_char  = *( token.end() - 1 );
  if ( first_char == ':' && last_char == ':' ) {
    return alignmet::center;
  } else if ( first_char == ':' ) {
    return alignmet::left;
  } else if ( last_char == ':' ) {
    return alignmet::right;
  } else {
    return alignmet::no_preference;
  }
}

//
// Main
//

int main( int argc, const char **argv ) {
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
    auto path        = ( source_from_pipe ) ? "Inline" : argv[1];
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

    // Read the PEG Grammer into the string grammar
#ifdef NDEBUG  // build type = release
#include "tsv.peg.h"
#else  // build type = debug
    const string grammar = getFileContents( "src/main/tsv.peg" );
#endif

    // Read a source file into memory
    if ( !source_from_pipe ) {
      source = getFileContents( path );
    };

    // Is the input empty?
    if ( source.size() == 0 ) return 0;

    // Setup a PEG parser
    parser parser( grammar );
    parser.enable_ast<Ast>();
    parser.enable_packrat_parsing();
    parser.log = [&]( size_t ln, size_t col, const string &msg ) {
      cerr << path << ":" << ln << ":" << col << ": " << msg << endl;
    };

    // Enable tracing during parsing
    if ( print_trace ) {
      cout << "============= Parser trace =============" << endl;
      trace_parser( parser );
    }

    // Parse the source and make an AST
    shared_ptr<Ast> ast;
    if ( parser.parse_n( source.data(), source.size(), ast, path ) ) {
      if ( print_ast ) {
        cout << "============= Regular AST =============" << endl;
        cout << ast_to_s<Ast>( ast );
      }

      // If there is only a header line and no body, do not optimize the AST
      bool have_body = ( ast->nodes.size() == 2 ) ? true : false;

      ast = parser.optimize_ast(
          ast );  // Note that in the PEG we disable optimizing 'head' and 'body'

      if ( print_ast ) {
        cout << "============= Optimized AST =============" << endl;
        cout << peg::ast_to_s( ast );
        cout << "============= End of AST =============" << endl;
      }

      auto head     = ast->nodes[0];
      auto head_row = head->nodes[0];

      auto body = have_body ? ast->nodes[1] : nullptr;

      // For each row, the number of columns **MUST** be the same
      // Get the number of columns in the headrow
      size_t n_columns = head_row->nodes.size();

      // Here we check that all rows of the body have the same number of columns as the head row

      if ( have_body ) {
        size_t row_nr = 1;
        for ( auto row : body->nodes ) {
          auto n = row->nodes.size();
          if ( n != n_columns ) {
            // TODO: When I move this to a library, find an alternative to throwing exceptions
            // The code, which uses this may not understand c++ exceptions
            stringstream ss;
            ss << "All columns must have the same number of columns. The header has " << n_columns
               << " columns, but row " << row_nr << " has " << n << endl;
            throw runtime_error( ss.str() );
          }
          row_nr++;
        }
      }

      // Let's look at the column alignment.

      // Initialise an array of alignments
      vector<alignmet> column_alignments;
      column_alignments.reserve( n_columns );  // We know the number of columns

      // Do we have colons in the header?
      for ( auto cell : head_row->nodes ) {
        auto token = cell->token;
        column_alignments.push_back( get_alignment_from_colons( token ) );
      }

      // Concerning the alignment, what happens if there are no colons?
      // If all cells of a column are empty -> default alignment = left
      // If all cells are numbers and perhaps some empty -> right
      if ( have_body ) {
        for ( size_t i = 0; i < n_columns; i++ ) {
          if ( column_alignments[i] == alignmet::no_preference ) {
            bool all_numbers = true;
            bool all_empty   = true;
            for ( auto row : body->nodes ) {
              if ( row->nodes[i]->name != "empty" ) {
                all_empty = false;
              }
              if ( row->nodes[i]->name != "number" && row->nodes[i]->name != "empty" ) {
                all_numbers = false;
                break;
              }
            }
            if ( all_numbers && !all_empty ) {
              column_alignments[i] = alignmet::right;
            }
          }
        }
      }

      // Get the size of each column as vector<size_t>
      vector<size_t> column_sizes;
      column_sizes.reserve( n_columns );  // We know the number of columns
      for ( size_t i = 0; i < n_columns; i++ ) {
        column_sizes.push_back( 0 );
      }

      // First, for the header row
      get_max_col_sizes( head_row, column_sizes );

      // Adjust the header cell size by ignoring any colons
      size_t i = 0;
      for ( auto cell : head_row->nodes ) {
        auto token      = cell->token;
        auto first_char = *token.begin();
        auto last_char  = *( token.end() - 1 );
        if ( first_char == ':' && last_char == ':' ) {
          column_sizes[i] -= 2;
        } else if ( first_char == ':' ) {
          column_sizes[i] -= 1;
        } else if ( last_char == ':' ) {
          column_sizes[i] -= 1;
        }
        i++;
      }

      // Now, for the body
      if ( have_body ) {
        for ( auto row : body->nodes ) {
          get_max_col_sizes( row, column_sizes );
        }
      }

      // We are finished with weighing and measuring!
      // Now it is time to produce the output

      //
      // 1 - The header
      //

      // Remove any alignment related colons in the header, if any
      cout << "| ";  // Start the line
      bool first = true;
      i          = 0;
      for ( auto cell : head_row->nodes ) {
        if ( first ) {
          first = false;
        } else {
          cout << "| ";
        }
        auto token      = cell->token;
        auto first_char = *token.begin();
        auto last_char  = *( token.end() - 1 );
        string_view token_;
        if ( first_char == ':' && last_char == ':' ) {
          token_ = token.substr( 1, token.size() - 2 );
        } else if ( first_char == ':' ) {
          token_ = token.substr( 1, token.size() - 1 );
        } else if ( last_char == ':' ) {
          token_ = token.substr( 0, token.size() - 1 );
        } else {
          token_ = token;
        }

        // Calculate the spaces on the left and right side and print the token
        print_cell( token_, column_alignments[i], column_sizes[i] );
        i++;
      }
      cout << "|" << endl;  // Finish the line

      //
      // 2 - The separation line
      //
      cout << "|";  // Start the line
      first = true;
      for ( size_t i = 0; i < n_columns; i++ ) {
        if ( first ) {
          first = false;
        } else {
          cout << "|";
        }

        auto n_dashes = column_sizes[i] + 2;  // +2 for the spaces around headers
        switch ( column_alignments[i] ) {
          case alignmet::center: n_dashes -= 2; break;
          case alignmet::left: n_dashes -= 1; break;
          case alignmet::right: n_dashes -= 1; break;
          default: break;
        }

        switch ( column_alignments[i] ) {
          case alignmet::center: cout << ':'; break;
          case alignmet::left: cout << ':'; break;
          default: break;
        }

        for ( size_t j = 0; j < n_dashes; j++ ) cout << '-';

        switch ( column_alignments[i] ) {
          case alignmet::center: cout << ':'; break;
          case alignmet::right: cout << ':'; break;
          default: break;
        }
      }
      cout << "|" << endl;  // Finish the line

      //
      // 3 - The table body
      //

      if ( have_body ) {
        for ( auto row : body->nodes ) {
          cout << "| ";  // Start the line
          first = true;
          i     = 0;
          for ( auto cell : row->nodes ) {
            if ( first ) {
              first = false;
            } else {
              cout << "| ";
            }

            auto token = cell->token;

            // Calculate the spaces on the left and right side and print the token
            print_cell( token, column_alignments[i], column_sizes[i] );
            i++;
          }
          cout << "|" << endl;  // Finish the line
        }
      }

      return 0;
    }
  } catch ( const runtime_error &e ) {
    cerr << e.what() << endl;
  } catch ( const exception &e ) {
    cerr << e.what() << endl;
  }

  return -1;
}
