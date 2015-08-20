cmake_minimum_required(VERSION 2.6)
PROJECT(sess_cache_test)
set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(/usr/local/include ../../include ../../../efnfw)



link_directories(/usr/local/lib)
link_directories(../../../efnfw/lib ../../lib)

link_libraries(libcommon.a libefnfw.a  zookeeper_mt.a jsoncpp  protobuf hiredis pthread)

file(GLOB SRC_LISTS *.cpp)
MESSAGE( ${SRC_LISTS} )

add_executable(sess_cache_test ${SRC_LISTS})