#! /usr/bin/env python2.7
import argparse
import struct
from sc_serverproxy import SimpleConcoordServer
# network api type
# 0 socket
# 1 connect
# 2 send
# 3 recv
# 4 close

# now we just hardcore the server address as 127.0.0.1:14001

replica_group = "127.0.0.1:14000"



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-f","--file",dest="temp_file_name",type=str,required=True,help="the temporary file name used to transfer data between python proxy and c program")
    parser.add_argument("-t","--type",dest="api_type",type=int,required=True,help="define the type of network socket api",choice=[0,1,2,3,4])
    options = parser.parse_args()
    concoord_object = SimpleConcoordServer("127.0.0.1:14000")
    if options.api_type == 0:
        ret 



if __name__=="__main__":
    main()

