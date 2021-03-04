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

TEST_CASE( MyFixture, COUNT_CODE_POINTS ) {
  SECTION( "JUST 3 ASCII CHARS" ) {
    const char *s = "123";
    auto len      = count_ut8_codepoints( s );
    CHECK_EQUAL( len, 3 );
  }

  SECTION( "SINGE CODE POINT" ) {
    // Trade mark --> U+2122 ™ TRADE MARK SIGN UTF-8 (hex)	0xE2 0x84 0xA2 (e284a2)
    const char *s = "™";
    CHECK_EQUAL( count_ut8_codepoints( s ), 1 );
  }

  SECTION( "MIXED" ) {
    // Trade mark --> U+2122 ™ TRADE MARK SIGN UTF-8 (hex)	0xE2 0x84 0xA2 (e284a2)
    const char *s = "a™b";
    CHECK_EQUAL( count_ut8_codepoints( s ), 3 );
  }

  SECTION( "STRING VIEW" ) {
    // Trade mark --> U+2122 ™ TRADE MARK SIGN UTF-8 (hex)	0xE2 0x84 0xA2 (e284a2)
    std::string str = "a™bcdefghijklmn";
    auto s          = std::string_view( str.data(), 5 );
    CHECK_EQUAL( count_ut8_codepoints( s ), 3 );
  }
}

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

TEST_CASE( MyFixture, UNICODE ) {
  SECTION( "TM Symbol" ) {
    stringstream out;
    stringstream err;
    const char *in = "Col1\tCol2\n123\t5Chars\n123\t5Char™";  // Contains a TM Symbol
    const char *path = "Inline";
    auto result = tsv_to_md( in, path, out, err, false, false );
    auto expected_out = R"(| Col1 | Col2   |
|-----:|--------|
|  123 | 5Chars |
|  123 | 5Char™ |
)";
    CHECK_EQUAL( out.str(), expected_out );
  }
}

TEST_CASE( MyFixture, ExpectedErrors ) {
  // Test an expected error
}

