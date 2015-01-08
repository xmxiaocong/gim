#!/bin/bash

cp -fr ../proto . 

protoc  proto/pair.proto --cpp_out=proto -I=proto
mv proto/pair.pb.cc src/pair.pb.cpp

protoc  proto/message.proto --cpp_out=proto -I=proto
mv proto/message.pb.cc src/message.pb.cpp

protoc  proto/session.proto --cpp_out=proto -I=proto
mv proto/session.pb.cc src/session.pb.cpp

cp -f proto/*.pb.* include/

rm -fr proto

mkdir -p lib
rm -fr CMakeCache.txt
rm -fr CMakeFiles
cmake .
make clean;make
rm -fr CMakeCache.txt
rm -fr CMakeFiles
