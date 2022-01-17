cd build
rm -rf bin/Resources
rm -rf Resources
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 8
mv Resources bin/Resources
mv bin/openspades bin/openspadesplus
cd bin
./openspadesplus
