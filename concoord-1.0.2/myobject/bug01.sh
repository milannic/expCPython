#! /bin/bash
echo $PYTHONPATH
ps -A |grep "concoord" | awk '{print $1}' | xargs kill
ps -A |grep "mongoose" | awk '{print $1}' | xargs kill
concoord object -o sc_server.SimpleConcoordServer
concoord replica -o sc_server.SimpleConcoordServer -b 128.59.17.171:14000 -a 128.59.17.173 -p 14000 &
#concoord acceptor -b 128.59.17.171:14000 -w &
sleep 5
python2.7 start_server.py &



