#pragma once

// #include <algorithm>  // for transform
// #include <cstring>    // for strerror
// #include <filesystem>
#include <string>  // for strerror

#include "util.h"

extern const char *tsv_version;
extern const char *tsv_help;

using namespace std;

enum alignmet { no_preference, left, center, right };

/// prints a single table cell to standard output and takes care of
/// padding for the alignment based on column size
string print_cell( string_view token, const alignmet alignment, const size_t &size );

alignmet get_alignment_from_colons( string_view token );

Result tsv_to_md( string source, const char *path, stringstream &out, stringstream &err,
                     bool print_ast = false, bool print_trace = false );