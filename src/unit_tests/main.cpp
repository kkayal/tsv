// USING TEST FRAMEWORK https://github.com/drleq/CppUnitTestFramework
#define GENERATE_UNIT_TEST_MAIN

#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "CppUnitTestFramework.hpp"
#include "tsvlib.h"
#include "util.h"

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
const string grammar = getFileContents( "src/tsv-lib/tsv.peg" );
#endif

struct MyFixture {
  string source = R"(
Col1	Column2
abc	1
5	77
)";
};

TEST_CASE( MyFixture, IncompleteInput ) {
  const char *path = "Inline";

  SECTION( "NO INPUT - NO OUTPUT" ) {
    stringstream out;
    stringstream err;
    auto result = tsv_to_md( "", path, out, err, true, false );
    CHECK_EQUAL( out.str(), "" );
  }

  SECTION( "ONLY ONE WORD" ) {
    stringstream out;
    stringstream err;
    auto result = tsv_to_md( "Col1", path, out, err, false, false );
    CHECK_EQUAL( out.str(), "| Col1 |\n|------|\n" );
  }

  SECTION( "ONLY TWO COLUMN HEADERS" ) {
    stringstream out;
    stringstream err;
    auto result = tsv_to_md( "Col1\tCol2", path, out, err, false, false );
    CHECK_EQUAL( out.str(), "| Col1 | Col2 |\n|------|------|\n" );
  }
}

TEST_CASE( MyFixture, Tokens ) {
  stringstream out;
  stringstream err;
  const char *path = "Inline";

  SECTION( "AST - SIMPLE TABLE" ) {
    auto result = tsv_to_md( source, path, out, err, true, false );
    CHECK_EQUAL( out.str(), R"(============= Regular AST =============
+ table
  + head
    + row
      + cell/2
        - phrase (Col1)
      + cell/2
        - phrase (Column2)
  + body
    + row
      + cell/2
        - phrase (abc)
      + cell/1
        - number (1)
    + row
      + cell/1
        - number (5)
      + cell/1
        - number (77)
============= Optimized AST =============
+ table
  + head
    + row
      - cell/2[phrase] (Col1)
      - cell/2[phrase] (Column2)
  + body
    + row
      - cell/2[phrase] (abc)
      - cell/1[number] (1)
    + row
      - cell/1[number] (5)
      - cell/1[number] (77)
============= End of AST =============
| Col1 | Column2 |
|------|--------:|
| abc  |       1 |
| 5    |      77 |
)" );
  }
}

TEST_CASE( MyFixture, ExpectedErrors ) {
  // Test an expected error
}
