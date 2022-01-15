#!/bin/bash
cd /home/edrian/Documents/GitHub/openspadesplus/
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 8
mv Resources bin
cd bin
mv openspades openspadesplus
./openspadesplus
