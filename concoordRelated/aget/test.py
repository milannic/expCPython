#! /usr/bin/env python2.7
from sc_serverproxy import SimpleConcoordServer

sc = SimpleConcoordServer("127.0.0.1:14001")
sc.start_server()
raw_input()
a = sc.sc_socket(2,1,0)
print a
raw_input()
b = sc.sc_connect(a)
print b
raw_input()
c = sc.sc_send(a,"HEAD /main.c HTTP/1.1\r\nHost: 127.0.0.1\r\nUser-Agent: UNIX\r\n\r\n",0)
print c
raw_input()
d = sc.sc_recv(a,200,0)
print d
raw_input()
sc.kill_server()
raw_input()

