#! /usr/bin/env python2.7
import argparse
import struct
from sc_serverproxy import SimpleConcoordServer
import posix_ipc
import sys
import mmap
import os
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
    parser.add_argument("-k","--key",dest="key",type=str,required=True,help="the temporary file name used to transfer data between python proxy and c program")
    parser.add_argument("-t","--type",dest="api_type",type=int,required=True,help="define the type of network socket api",choices=[0,1,2,3,4])
    options = parser.parse_args()
    concoord_object = SimpleConcoordServer("127.0.0.1:14000")
    try:
        shared_mem = posix_ipc.SharedMemory(options.key)
        map_file = mmap.mmap(shared_mem.fd,shared_mem.size)
        os.close(shared_mem.fd)
    except Exception,e:
        print e
        print "cannot open the shared memory"
        sys.exit(1)

    if options.api_type == 0:
        try:
            domain = struct.unpack("<i",map_file.read(4))[0]
            print domain
            my_type= struct.unpack("<i",map_file.read(4))[0]
            print my_type
            protocol= struct.unpack("<i",map_file.read(4))[0]
            print protocol
            ret = concoord_object.sc_socket(domain,my_type,protocol)
        except Exception,e:
            print "error occured"
            ret = -1
            print e
        print ret
        map_file.seek(0)
        map_file.write(struct.pack("<i",ret))
        map_file.close()
        




if __name__=="__main__":
    main()

