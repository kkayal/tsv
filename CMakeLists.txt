cmake_minimum_required(VERSION 3.13.4)

project(all)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/util)
add_subdirectory(src/tsv-lib)
add_subdirectory(src/tsv-bin)
add_subdirectory(src/unit_tests)

install(TARGETS tsv-bin DESTINATION bin)
install(TARGETS tsv-lib DESTINATION lib)