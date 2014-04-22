#! /bin/sh
CUR_DIR=$(pwd)
LD_LIB="${CUR_DIR}/libconcoord.so.1.0"

make;
make link;

LD_PRELOAD=${LD_LIB} ./test.out
