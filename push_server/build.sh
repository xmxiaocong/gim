#!/bin/bash


cp -fr ../proto src/

protoc  src/proto/connect_server.proto --cpp_out=src/proto -I=src/proto

mv src/proto/connect_server.pb.cc src/proto/connect_server.pb.cpp

rm -fr CMakeCache.txt
rm -fr CMakeFiles
rm -fr cmake_install.cmake

mkdir -p lib

cmake .
make clean;make
rm -fr CMakeCache.txt
rm -fr CMakeFiles
rm -fr cmake_install.cmake
