cmake_minimum_required(VERSION 3.13.4)

set (tsv_VERSION_MAJOR 0)
set (tsv_VERSION_MINOR 1)
set (tsv_VERSION_PATCH 0)

project(unit_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_C_FLAGS -Wfatal-errors)
	set(CMAKE_CXX_FLAGS -Wfatal-errors)
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB SOURCES "*.c" "*.cpp")

# Compile and link with -pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(../main ../util)

add_executable(unit_tests ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE util Threads::Threads)
