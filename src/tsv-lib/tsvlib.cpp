#include "tsvlib.h"

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "peglib.h"

using namespace peg;
using namespace peg::udl;
using namespace std;

const char *tsv_version = "0.4.0";
const char *tsv_help    = "Usage: tsv [--version] [-h] INPUT_FILE [--ast] [--trace]";

// Copied and modified from cpp_peg linter at https://github.com/yhirose/cpp-peglib
void trace_parser( parser parser, stringstream &out ) {
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
        out << "E " << pos << backtrack << "\t" << indent << "┌" << name << " #"
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
        out << "L " << pos << "\t" << indent << ret << name << " #" << c.trace_ids.back()
            << choice.str() << token << matched << std::endl;
      } );
}

/// prints a single table cell to standard output and takes care of
/// padding for the alignment based on column size
string print_cell( string_view token, const alignmet alignment, const size_t &size ) {
  stringstream ss;
  // Get the length of the token as number of code points
  size_t len = count_ut8_codepoints(token);
  switch ( alignment ) {
    case alignmet::center: {
      auto spaces_left  = ( size - len ) / 2;
      auto spaces_right = size + 1 - len - spaces_left;
      for ( size_t j = 0; j < spaces_left; j++ ) ss << ' ';
      ss << token;
      for ( size_t j = 0; j < spaces_right; j++ ) ss << ' ';
    } break;
    case alignmet::right: {
      for ( size_t j = 0; j < size - len; j++ ) ss << ' ';
      ss << token << ' ';
    } break;
    case alignmet::left: [[fallthrough]];
    case alignmet::no_preference: {
      ss << token;
      for ( size_t j = 0; j < size + 1 - len; j++ ) ss << ' ';
    } break;
    default: break;
  }
  return ss.str();
}

/// Find the max size of each column
void get_max_col_sizes( const std::shared_ptr<Ast> &ast, vector<size_t> &column_sizes ) {
  size_t i = 0;
  for ( auto cell : ast->nodes ) {
    auto size = count_ut8_codepoints( cell->token );
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

Result tsv_to_md( string source, const char *path, stringstream &out, stringstream &err,
                     bool print_ast, bool print_trace ) {
  try {
    // Read the PEG Grammer into the string grammar
#ifdef NDEBUG  // build type = release
#include "tsv.peg.h"
#else  // build type = debug
    const string grammar = getFileContents( "src/tsv-lib/tsv.peg" );
#endif

    // Is the input empty?
    if ( source.size() == 0 ) return Result{ .code = 0, .msg = nullptr };

    // Setup a PEG parser
    parser parser( grammar );
    parser.enable_ast<Ast>();
    parser.enable_packrat_parsing();
    parser.log = [&]( size_t ln, size_t col, const string &msg ) {
      err << path << ":" << ln << ":" << col << ": " << msg << endl;
    };

    // Enable tracing during parsing
    if ( print_trace ) {
      out << "============= Parser trace =============\n";
      trace_parser( parser, out );
    }

    // Parse the source and make an AST
    shared_ptr<Ast> ast;
    if ( parser.parse_n( source.data(), source.size(), ast, path ) ) {
      if ( print_ast ) {
        out << "============= Regular AST =============\n";
        out << ast_to_s<Ast>( ast );
      }

      // If there is only a header line and no body, do not optimize the AST
      bool have_body = ( ast->nodes.size() == 2 ) ? true : false;

      // Note that in the PEG we disable optimizing 'head' and 'body'
      ast = parser.optimize_ast( ast );

      if ( print_ast ) {
        out << "============= Optimized AST =============\n";
        out << peg::ast_to_s( ast );
        out << "============= End of AST =============\n";
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
      out << "| ";  // Start the line
      bool first = true;
      i          = 0;
      for ( auto cell : head_row->nodes ) {
        if ( first ) {
          first = false;
        } else {
          out << "| ";
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
        out << print_cell( token_, column_alignments[i], column_sizes[i] );
        i++;
      }
      out << "|\n";  // Finish the line

      //
      // 2 - The separation line
      //
      out << "|";  // Start the line
      first = true;
      for ( size_t i = 0; i < n_columns; i++ ) {
        if ( first ) {
          first = false;
        } else {
          out << "|";
        }

        auto n_dashes = column_sizes[i] + 2;  // +2 for the spaces around headers
        switch ( column_alignments[i] ) {
          case alignmet::center: n_dashes -= 2; break;
          case alignmet::left: n_dashes -= 1; break;
          case alignmet::right: n_dashes -= 1; break;
          default: break;
        }

        switch ( column_alignments[i] ) {
          case alignmet::center: out << ':'; break;
          case alignmet::left: out << ':'; break;
          default: break;
        }

        for ( size_t j = 0; j < n_dashes; j++ ) out << '-';

        switch ( column_alignments[i] ) {
          case alignmet::center: out << ':'; break;
          case alignmet::right: out << ':'; break;
          default: break;
        }
      }
      out << "|\n";  // Finish the line

      //
      // 3 - The table body
      //

      if ( have_body ) {
        for ( auto row : body->nodes ) {
          out << "| ";  // Start the line
          first = true;
          i     = 0;
          for ( auto cell : row->nodes ) {
            if ( first ) {
              first = false;
            } else {
              out << "| ";
            }

            auto token = cell->token;

            // Calculate the spaces on the left and right side and print the token
            out << print_cell( token, column_alignments[i], column_sizes[i] );
            i++;
          }
          out << "|\n";  // Finish the line
        }
      }
    }
  } catch ( const runtime_error &e ) {
    return Result{ .code = -1, .msg = e.what() };
  } catch ( const exception &e ) {
    return Result{ .code = -1, .msg = e.what() };
  }

  return Result{ .code = 0, .msg = nullptr };
}
