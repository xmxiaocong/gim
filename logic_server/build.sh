#!/bin/bash


cp -fr ../proto/* .

protoc  pair.proto --cpp_out=. -I=.
mv -f pair.pb.cc pair.pb.cpp

protoc  connect_server.proto --cpp_out=. -I=.
mv -f connect_server.pb.cc connect_server.pb.cpp

rm -fr CMakeCache.txt
rm -fr CMakeFiles
rm -fr cmake_install.cmake

mkdir -p lib

cmake .
make clean;make
rm -fr CMakeCache.txt
rm -fr CMakeFiles
rm -fr cmake_install.cmake
