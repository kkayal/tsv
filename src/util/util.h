#pragma once

#include <algorithm>  // for transform
#include <cstring>    // for strerror
#include <filesystem>
#include <fstream>
#include <sstream>  // for stringstream
#include <string>

/// Returns the contents of a file (binary) as a string
// Thanks to http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html for comparing
// the speed of various methods
std::string getFileContents( const char* filename );

/// returns a string of spaces for indentation
std::string indent( size_t level, size_t tab_size = 2 );

struct Result {
  int code;
  const char* msg;
};