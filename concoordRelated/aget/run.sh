#! /bin/bash
rm ./out/*
make clean
make
LD_PRELOAD=./rand-intercept.so ./aget -f -n2 -p 8080 http://localhost/main.c
rm main.c
