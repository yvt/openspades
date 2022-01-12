#!/bin/bash
rm -r openspades.mk
mkdir openspades.mk
cd openspades.mk
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 8
mv Resources bin
cd bin
mv openspades openspadesplus

