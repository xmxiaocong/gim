#!/bin/bash

cd ../efnfw;
sh build.sh
cd -

cd ../common;
sh build.sh
cd -

cd ../logic_server;
sh build.sh
cd -


mkdir -p bin		

rm -fr CMakeCache.txt
rm -fr CMakeFiles


cmake .
make clean;make

rm -fr CMakeCache.txt
rm -fr CMakeFiles
