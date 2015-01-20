#!/bin/sh

cd `dirname $0`

cp -fr ../../proto src/

protoc src/proto/pair.proto -I=src/proto/ --cpp_out=src/proto
mv src/proto/pair.pb.cc src/proto/pair.pb.cpp

protoc src/proto/message.proto -I=src/proto/ --cpp_out=src/proto
mv src/proto/message.pb.cc src/proto/message.pb.cpp

protoc src/proto/peer_server.proto -I=src/proto/ --cpp_out=src/proto
mv src/proto/peer_server.pb.cc src/proto/peer_server.pb.cpp

mkdir -p bin

echo "clean build files"
rm -fr CMakeCache.txt 
rm -fr CMakeFiles

cmake .
make clean;make

