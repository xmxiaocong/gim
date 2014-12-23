#!/bin/bash
mkdir -p bin

cd ../common;
sh build.sh
cd -

rm -fr CMakeCache.txt
rm -fr CMakeFiles


cmake .
make clean;make

rm -fr CMakeCache.txt
rm -fr CMakeFiles
