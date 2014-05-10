#! /bin/bash
ps -A | grep "python2.7" | awk '{print $1}' | xargs kill
LIB_NAME="./libpctern.so"
rm -rf ./out/*
rm -rf main.c
rm -rf test.py
rm -rf llvm.source.tgz
rm -rf input.tar
make clean
make
make link
#LD_PRELOAD=${LIB_NAME} ./aget -f -n2 -p 8080 http://localhost/hahaha.c
#LD_PRELOAD=${LIB_NAME} ./aget -f -n2 -p 8080 http://localhost/aget_cp
#LD_PRELOAD=${LIB_NAME} ./aget -f -n1 -p 8080 http://localhost/test.py
#./aget_cp -f -n2 -p 8080 http://128.59.17.171/llvm.source.tgz
LD_PRELOAD=${LIB_NAME} ./aget_cp -f -n2 -p 8080 http://localhost/input.tar
#LD_PRELOAD=${LIB_NAME} ./aget_cp -f -n2 -p 8080 http://localhost/test.py
