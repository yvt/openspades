#!/bin/bash
rm -r build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j
mv Resources bin
cd bin
mv openspades openspadesplus
./openspadesplus
