#!/bin/sh

cd `dirname $0`

cp -fr ../src/proto src/

protoc src/proto/pair.proto -I=src/proto/ --cpp_out=src/proto
mv src/proto/pair.pb.cc src/proto/pair.pb.cpp

protoc src/proto/connect_server.proto -I=src/proto/ --cpp_out=src/proto
mv src/proto/connect_server.pb.cc src/proto/connect_server.pb.cpp

mkdir -p bin

echo "clean build files"
rm -fr CMakeCache.txt 
rm -fr CMakeFiles

cmake .
make clean;make

