cmake_minimum_required(VERSION 2.6)

set(CMAKE_VERBOSE_MAKEFILE ON)
project(redisclitest)

add_definitions("-Wall -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# add dependency
include_directories(/usr/local/include ../../../efnfw ../../include)
link_directories(/usr/local/lib ../../../efnfw/lib ../../lib)
link_libraries(libcommon.a libefnfw.a hiredis)
#file(GLOB_RECURSE SRC_LISTS ./*.cpp)
set(SRC_LISTS redis_cli_test.cpp)
add_executable(redisclitest ${SRC_LISTS})
