#!/bin/bash
mkdir -p  ${2}/include
mkdir -p  ${2}lib
mkdir -p  ${2}/bin
cp -r ${1}/external/src/tbb_2019/include/tbb  ${2}/include/ 
cp $(find ${1}/external/src/tbb_2019/ -name *.a) ${2}/lib/ 

