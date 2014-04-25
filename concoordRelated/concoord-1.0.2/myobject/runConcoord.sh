#! /bin/bash
ps -A |grep "concoord" | awk '{print $1}' | xargs kill
ps -A |grep "mongoose" | awk '{print $1}' | xargs kill
concoord object -o sc_server.SimpleConcoordServer
concoord replica -o sc_server.SimpleConcoordServer -a 127.0.0.1 -p 14001 &
concoord acceptor -b 127.0.0.1:14001 -w &


#! /bin/sh

