#pragma once

#include <algorithm>  // for transform
#include <cstring>    // for strerror
#include <filesystem>
#include <string>  // for strerror

#include "../main/peglib.h"

using namespace peg;
using namespace peg::udl;
using namespace std;

/// Returns the contents of a file (binary) as a string
// Thanks to http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html for comparing
// the speed of various methods
std::string getFileContents( const char *filename );

/// Prints a trace of every parsing step
void trace_parser( parser parser );

/// returns a string of spaces for indentation
string indent( size_t level, size_t tab_size = 2 );