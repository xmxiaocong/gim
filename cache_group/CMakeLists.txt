cmake_minimum_required(VERSION 2.6)
project(cachegroup)
set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -g")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/../common
${PROJECT_SOURCE_DIR}/include)
AUX_SOURCE_DIRECTORY(src SRC_LIST)
add_library(cachegroup STATIC ${SRC_LIST})
#file(GLOB_RECURSE EF_LIST *.cpp)
#add_library(cachegroup STATIC ${EF_LIST})
