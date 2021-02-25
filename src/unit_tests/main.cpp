// USING TEST FRAMEWORK https://github.com/drleq/CppUnitTestFramework
#define GENERATE_UNIT_TEST_MAIN

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "CppUnitTestFramework.hpp"
#include "peglib.h"
#include "util.h"

using namespace peg;
using namespace peg::udl;
using namespace std;

/*
 * Utilities
 */
string format_error_message( const string &path, size_t ln, size_t col, const string &msg ) {
  stringstream ss;
  ss << path << ":" << ln << ":" << col << ": " << msg << endl;
  return ss.str();
}

/*
 * Testing
 */

// Read the PEG Grammer into the string grammar
#ifdef NDEBUG  // build type = release
#include "tsv.peg.h"
#else  // build type = debug
const string grammar = getFileContents( "src/main/tsv.peg" );
#endif

struct MyFixture {
  string source = "var u8 x3";
};

TEST_CASE( MyFixture, Tokens ) {
  // Setup a PEG parser
  parser p( grammar );
  p.enable_ast<Ast>();
  p.log = [&]( size_t ln, size_t col, const string &msg ) {
    cerr << format_error_message( "inline", ln, col, msg ) << endl;
  };

  SECTION( "VAR DECLARATION" ) {
    shared_ptr<Ast> ast;
    if ( p.parse_n( source.data(), source.size(), ast, "inline" ) ) {
      // SymbolTable::build_on_ast(ast);

      string ast_str = ast_to_s<Ast>( ast ).c_str();
      CHECK_EQUAL( ast_str, R"(+ program
  + block
    + var
      + type/0
      - ident (x3)
)" );
    }
  }

  SECTION( "VAR TYPE" ) {
    // Next test within this test case
  }
}

TEST_CASE( MyFixture, ExpectedErrors ) {
  // Test an expected error
}
