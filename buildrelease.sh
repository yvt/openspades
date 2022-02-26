#!/bin/bash
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 8
mv Resources bin
cd bin
mv openspades openspadesplus
cd ..
zip -r -9 bin
mv bin.zip OpenSpadesPlusRel00.zip
