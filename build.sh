#!/bin/bash
rm -r openspades.mk
mkdir openspades.mk
cd openspades.mk
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 4
echo Remember to move Resources into bin if you are not running install.sh
