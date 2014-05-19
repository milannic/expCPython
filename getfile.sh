#! /bin/bash
ps -A | grep "python2.7" | awk '{print $1}' | xargs kill
LIB_NAME="./libpctern.so"
LD_PRELOAD=${LIB_NAME} ./aget_cp -f -n2 -p 8080 http://localhost/input.tar
