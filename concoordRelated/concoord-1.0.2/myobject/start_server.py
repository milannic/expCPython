#! /usr/bin/env python2.7
from sc_serverproxy import SimpleConcoordServer

sc = SimpleConcoordServer("127.0.0.1:14001")
sc.start_server()
