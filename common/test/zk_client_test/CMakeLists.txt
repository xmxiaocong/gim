cmake_minimum_required(VERSION 2.6)
PROJECT(zk_client_test)
#set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -g")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(/usr/local/include  ../../include ../../../efnfw)



link_directories(/usr/local/lib)
link_directories(../../../efnfw/lib ../../lib)

link_libraries(pthread libcommon.a zookeeper_mt jsoncpp libefnfw.a)

file(GLOB SRC_LISTS test.cpp)
MESSAGE( ${SRC_LISTS} )

add_executable(zk_client_test ${SRC_LISTS})
