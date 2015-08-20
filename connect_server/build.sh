#!/bin/bash

cd ../efnfw;
sh build.sh
cd -

cd ../common;
sh build.sh
cd -

cp -fr ../proto src/

protoc  src/proto/connect_server.proto --cpp_out=src/proto -I=src/proto 

mv src/proto/connect_server.pb.cc src/proto/connect_server.pb.cpp

mkdir -p bin		

rm -fr CMakeCache.txt
rm -fr CMakeFiles


cmake .
make clean;make

rm -fr CMakeCache.txt
rm -fr CMakeFiles
