project(CommonTest)
set(CMAKE_VERBOSE_MAKEFILE ON)
add_definitions("-Wall -pg -g")
include_directories(${PROJECT_SOURCE_DIR}/../common
${PROJECT_SOURCE_DIR}/../elog
${PROJECT_SOURCE_DIR}/../hiredis-master
${PROJECT_SOURCE_DIR}/../data_cache
${PROJECT_SOURCE_DIR}/../zookeeper/include)
link_directories(/usr/local/lib ../common/lib/ ../elog/lib 
../data_cache/lib ../hiredis-master ../zookeeper/lib) 
link_libraries(libefnfw.a pthread)
file(GLOB_RECURSE SRC_LIST src/*.cpp)
add_executable(bin/CommonTest ${SRC_LIST})
