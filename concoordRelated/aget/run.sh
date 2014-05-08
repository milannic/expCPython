#! /bin/bash
LIB_NAME="./libpctern.so"
rm -rf ./out/*
rm -rf main.c
rm -rf test.py
make clean
make
make link
#LD_PRELOAD=${LIB_NAME} ./aget -f -n2 -p 8080 http://localhost/hahaha.c
#LD_PRELOAD=${LIB_NAME} ./aget -f -n2 -p 8080 http://localhost/aget_cp
#LD_PRELOAD=${LIB_NAME} ./aget -f -n1 -p 8080 http://localhost/test.py
LD_PRELOAD=${LIB_NAME} ./aget_cp -f -n2 -p 8080 http://localhost/test.py
