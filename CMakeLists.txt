cmake_minimum_required(VERSION 3.13.4)

set (ALL_VERSION_MAJOR 0)
set (ALL_VERSION_MINOR 1)
set (ALL_VERSION_PATCH 0)

project(all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/util)
add_subdirectory(src/main)
add_subdirectory(src/unit_tests)

install(TARGETS tsv DESTINATION bin)