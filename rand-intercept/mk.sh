#! /bin/sh
CUR_DIR=$(pwd)
LD_LIB="${CUR_DIR}/ldi.so"

LD_PRELOAD=${LD_LIB} ./test
