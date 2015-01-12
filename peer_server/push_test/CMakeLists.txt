cmake_minimum_required(VERSION 2.6)

set(CMAKE_VERBOSE_MAKEFILE ON)
project(PushTest)

add_definitions(-g)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# add dependency

include_directories(/usr/local/include ../../efnfw)
link_directories(/usr/local/lib ../../efnfw/lib)

link_libraries(libefnfw.a protobuf pthread) 


file(GLOB_RECURSE SRC_LISTS src/*.cpp)
add_executable(push_server ${SRC_LISTS})

