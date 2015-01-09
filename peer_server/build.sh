#!/bin/bash


cd ../efnfw;
sh build.sh
cd -

cd ../common;
sh build.sh
cd -

cd ../logic_server
sh build.sh
cd -

cp -fr ../proto/*.proto src/
		
protoc  src/peer_server.proto --cpp_out=src -I=src
mv src/peer_server.pb.cc src/peer_server.pb.cpp

rm -fr CMakeCache.txt
rm -fr CMakeFiles

cmake .
make clean;make
rm -fr CMakeCache.txt
rm -fr CMakeFiles
