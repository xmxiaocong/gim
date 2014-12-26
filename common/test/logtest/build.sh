#!/bin/bash
mkdir -p log
rm -fr CMakeCache.txt
cmake .
make clean;make
rm -fr CMakeCache.txt
rm -fr CMakeFiles
rm -fr Makefile
rm -fr cmake_install.cmake 
