#!/usr/bin/env bash

cd src/main
cat peg.incl.header.h tsv.peg peg.incl.footer.h > tsv.peg.h

cd ../..

if [ -d "build_release" ] 
then
    cd build_release
else
    mkdir build_release
    cd build_release
fi

cmake -S .. -DCMAKE_BUILD_TYPE=release -B .
make -j8
