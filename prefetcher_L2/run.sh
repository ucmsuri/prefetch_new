#! /bin/bash

cp $1.h prefetcher.h
cp $1.C prefetcher.C
make clean
make
#cp cacheSim $1_$2_$3_$4
cp cacheSim $1
#./cacheSim trace/dieharder.trace 

