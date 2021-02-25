#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "util.h"

using namespace peg;
using namespace peg::udl;
using namespace std;

const char *tsv_version = "0.1.0";
const char *tsv_help    = "Usage: tsv [--version] [-h] INPUT_FILE [--ast] [--trace]";

enum alignmet { no_preference, left, center, right };

/// prints a single table cell to standard output and takes care of
/// padding for the alignment based on column size
void print_cell( string_view token, const vector<alignmet> &col_alignments,
                 const vector<size_t> &col_sizes, size_t i ) {
  switch ( col_alignments[i] ) {
    case alignmet::center: {
      auto spaces_left  = ( col_sizes[i] - token.size() ) / 2;
      auto spaces_right = col_sizes[i] + 1 - token.size() - spaces_left;
      for ( size_t j = 0; j < spaces_left; j++ ) cout << ' ';
      cout << token;
      for ( size_t j = 0; j < spaces_right; j++ ) cout << ' ';
    } break;
    case alignmet::right: {
      for ( size_t j = 0; j < col_sizes[i] - token.size(); j++ ) cout << ' ';
      cout << token << ' ';
    } break;
    case alignmet::left: [[fallthrough]];
    case alignmet::no_preference: {
      cout << token;
      for ( size_t j = 0; j < col_sizes[i] + 1 - token.size(); j++ ) cout << ' ';
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

//
// Main
//

int main( int argc, const char **argv ) {
  try {
    // Parser commandline parameters
    auto path        = argv[1];
    bool print_trace = false;
    bool print_ast   = false;

    if ( argc == 1 ) {
      cout << endl;
      cout << tsv_help << endl;
      return 0;
    } else if ( argc == 2 && string( "-h" ) == argv[1] ) {
      cout << tsv_help << endl;
      return 0;
    } else if ( argc == 2 && string( "--version" ) == argv[1] ) {
      cout << tsv_version << endl;
      return 0;
    }

    auto arg = 2;
    while ( arg < argc ) {
      if ( string( "--ast" ) == argv[arg] ) {
        print_ast = true;
      } else if ( string( "--trace" ) == argv[arg] ) {
        print_trace = true;
      }
      arg++;
    }

    cout << boolalpha;  // I want to see 'true' and 'false' instead of '1' and '0'

    // Read the PEG Grammer into the string grammar
#ifdef NDEBUG  // build type = release
#include "tsv.peg.h"
#else  // build type = debug
    const string grammar = getFileContents( "src/main/tsv.peg" );
#endif

    // Read a source file into memory
    auto source = getFileContents( path );

    // Setup a PEG parser
    parser parser( grammar );
    parser.enable_ast<Ast>();
    parser.enable_packrat_parsing();
    parser.log = [&]( size_t ln, size_t col, const string &msg ) {
      cerr << path << ":" << ln << ":" << col << ": " << msg << endl;
    };

    // Print the tracing during parsing
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

      ast = parser.optimize_ast( ast );

      if ( print_ast ) {
        cout << "============= Optimized AST =============" << endl;
        cout << peg::ast_to_s( ast );
        cout << "============= End of AST =============" << endl;
      }

      // For each row, the number of columns **MUST** be the same

      // Since we use an optimized AST,
      // ast is the top level table, ast->nodes[0] is the header row. The 2nd level are the cells
      // Otherwise, we would have table -> head -> row
      size_t n_columns = ast->nodes[0]->nodes.size();

      // Optimized AST or not, the body is at ast->nodes[1]
      auto table_body_rows = ast->nodes[1]->nodes;

      // Here we check that all rows have the same number of columns
      size_t row_nr = 1;
      for ( auto row : table_body_rows ) {
        auto n = row->nodes.size();
        if ( n != n_columns ) {
          stringstream ss;
          ss << "All columns must have the same number of columns. The header has " << n_columns
             << " columns, but row " << row_nr << " has " << n << endl;
          throw runtime_error( ss.str() );
        }
        row_nr++;
      }

      // Let's look at the column alignment.

      // Initialise an array of alignments
      vector<alignmet> column_alignments;
      column_alignments.reserve( n_columns );  // We know the number of columns
      for ( size_t i = 0; i < n_columns; i++ ) {
        column_alignments.push_back( alignmet::no_preference );
      }

      // Do we have colons in the header?
      for ( size_t i = 0; i < n_columns; i++ ) {
        auto token      = ast->nodes[0]->nodes[i]->token;
        auto first_char = *token.begin();
        auto last_char  = *( token.end() - 1 );
        if ( first_char == ':' && last_char == ':' ) {
          column_alignments[i] = alignmet::center;
        } else if ( first_char == ':' ) {
          column_alignments[i] = alignmet::left;
        } else if ( last_char == ':' ) {
          column_alignments[i] = alignmet::right;
        }
      }

      // Concerning the alignment, if there is no preference for a certain column,
      // let's check if all cells are numers or empty. Then we chose alignmet::right
      // Otherwise, the default alignment is alignmet::left
      for ( size_t i = 0; i < n_columns; i++ ) {
        if ( column_alignments[i] == alignmet::no_preference ) {
          bool all_numbers = true;
          for ( auto row : ast->nodes[1]->nodes ) {
            if ( row->nodes[i]->name != "number" && row->nodes[i]->name != "empty" ) {
              all_numbers = false;
              break;
            }
          }
          if ( all_numbers ) {
            column_alignments[i] = alignmet::right;
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
      get_max_col_sizes( ast->nodes[0], column_sizes );

      // Ignore the colons in the header, if any
      for ( size_t i = 0; i < n_columns; i++ ) {
        auto token      = ast->nodes[0]->nodes[i]->token;
        auto first_char = *token.begin();
        auto last_char  = *( token.end() - 1 );
        if ( first_char == ':' && last_char == ':' ) {
          column_sizes[i] -= 2;
        } else if ( first_char == ':' ) {
          column_sizes[i] -= 1;
        } else if ( last_char == ':' ) {
          column_sizes[i] -= 1;
        }
      }

      // Now, for the body
      for ( auto row : ast->nodes[1]->nodes ) {
        get_max_col_sizes( row, column_sizes );
      }

      // We are finished with weighing and measuring!
      // Now it is time to produce the output

      //
      // 1 - The header
      //

      // Since we use an optimized AST,
      // ast is the top level table, ast->nodes[0] is the header row. The 2nd level are the cells
      // Otherwise, we would have table -> head -> row
      // Remove any alignment related colons in the header, if any
      cout << "| ";  // Start the line
      bool first = true;
      for ( size_t i = 0; i < n_columns; i++ ) {
        if ( first ) {
          first = false;
        } else {
          cout << "| ";
        }
        auto token      = ast->nodes[0]->nodes[i]->token;
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
        print_cell( token_, column_alignments, column_sizes, i );
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

      for ( auto row : table_body_rows ) {
        cout << "| ";  // Start the line
        first = true;
        for ( size_t i = 0; i < n_columns; i++ ) {
          if ( first ) {
            first = false;
          } else {
            cout << "| ";
          }

          auto token = row->nodes[i]->token;

          // Calculate the spaces on the left and right side and print the token
          print_cell( token, column_alignments, column_sizes, i );
        }
        cout << "|" << endl;  // Finish the line
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
