project(DPTEST)
set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -pg -g")
include_directories(${PROJECT_SOURCE_DIR}/../../common/include
${PROJECT_SOURCE_DIR}/../../efnfw ${PROJECT_SOURCE_DIR}/src)
link_directories(/usr/local/lib ../../common/lib/ ../../efnfw/lib)
link_libraries(libcommon.a libefnfw.a hiredis zookeeper_mt protobuf jsoncpp pthread)
file(GLOB_RECURSE SRC_LIST src/*.cpp)
add_executable(bin/dptest ${SRC_LIST})
