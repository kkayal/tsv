cmake_minimum_required(VERSION 3.13.4)

project(tsv-lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_C_FLAGS -Wfatal-errors)
	set(CMAKE_CXX_FLAGS -Wfatal-errors)
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB SOURCES "*.c" "*.cc" "*.cpp")

include_directories(../util)

add_library(tsv-lib ${SOURCES})

set_target_properties(tsv-lib PROPERTIES OUTPUT_NAME tsv)