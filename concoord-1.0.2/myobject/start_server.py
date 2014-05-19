#! /usr/bin/env python2.7
from sc_serverproxy import SimpleConcoordServer

sc = SimpleConcoordServer("128.59.17.171:14000,128.59.17.173:14000")
sc.start_server()
