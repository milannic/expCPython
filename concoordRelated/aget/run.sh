#! /bin/bash
LIB_NAME="./libpctern.so"
rm ./out/*
rm main.c
make clean
make
make link
LD_PRELOAD=${LIB_NAME} ./aget -f -n2 -p 8080 http://localhost/main.c
