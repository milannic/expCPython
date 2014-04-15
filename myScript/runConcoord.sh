#! /bin/bash

concoord replica -o concoord.object.counter.Counter -a 127.0.0.1 -p 14000 &
concoord acceptor -b 127.0.0.1:14000 -w &


