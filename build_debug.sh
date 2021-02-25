#!/usr/bin/env bash

if [ -d "build_debug" ] 
then
    cd build_debug
else
    mkdir build_debug
    cd build_debug
fi

cmake -S .. -DCMAKE_BUILD_TYPE=debug -B .
make -j8
