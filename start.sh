export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD
ln -s libcrypto.so libcrypto.so.1.0.0
ln -s libboost_system.so libboost_system.so.1.58.0
./server