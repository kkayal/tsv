cmake_minimum_required(VERSION 3.13.4)

project(tsv-bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_C_FLAGS -Wfatal-errors)
	set(CMAKE_CXX_FLAGS -Wfatal-errors)
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB SOURCES "*.c" "*.cc" "*.cpp")

include_directories(../util ../tsv-lib)

add_executable(tsv-bin ${SOURCES})

# Compile and link with -pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} tsv-lib util Threads::Threads)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# The executable name shall be just tsv and not tsv-bin
set_target_properties(tsv-bin PROPERTIES OUTPUT_NAME tsv)