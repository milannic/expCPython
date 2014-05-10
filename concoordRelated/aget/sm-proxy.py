#! /usr/bin/env python2.7
import argparse
import struct
from sc_serverproxy import SimpleConcoordServer
import posix_ipc
import sys
import mmap
import os
import json
# network api type
# 0 socket
# 1 connect
# 2 send
# 3 recv
# 4 close

# now we just hardcore the server address as 127.0.0.1:14001

#replica_group = "127.0.0.1:14000"
replica_group = "128.59.17.171:14001"



def main():
    debug = 0
    parser = argparse.ArgumentParser()
    parser.add_argument("-k","--key",dest="key",type=str,required=True,help="the temporary file name used to transfer data between python proxy and c program")
    #parser.add_argument("-t","--type",dest="api_type",type=int,required=True,help="define the type of network socket api",choices=[0,1,2,3,4])
    options = parser.parse_args()
    if debug:
        print "my pid is %s"%(str(os.getpid()))
    try:
        shared_mem = posix_ipc.SharedMemory(options.key)
        total_size = shared_mem.size
        map_file = mmap.mmap(shared_mem.fd,shared_mem.size)
        os.close(shared_mem.fd)
    except Exception,e:
        print e
        print "cannot open the shared memory"
        sys.exit(1)
    try:
        concoord_object = SimpleConcoordServer(replica_group)
    except Exception,e:
        print e
        map_file.write(struct.pack("<i",-99))
        sys.exit(1)

    while 1: 
        state = struct.unpack("<i",map_file.read(4))[0]
        #print state
        if state != -1:
            map_file.seek(0)
            pass
        else:
            api_type=struct.unpack("<i",map_file.read(4))[0]
            if debug:
                print state
                print api_type
            if api_type == 0:
                try:
                    domain = struct.unpack("<i",map_file.read(4))[0]
                    my_type= struct.unpack("<i",map_file.read(4))[0]
                    protocol= struct.unpack("<i",map_file.read(4))[0]
                    if debug:
                        print domain
                        print my_type
                        print protocol
                    ret = concoord_object.sc_socket(domain,my_type,protocol)
                except Exception,e:
                    print "error occured"
                    ret = -1
                    print e
                map_file.seek(4)
                map_file.write(struct.pack("<i",ret))
                map_file.seek(0)
                map_file.write(struct.pack("<i",0))
                map_file.seek(0)
            elif api_type == 1:
                try:
                    socket_num = struct.unpack("<i",map_file.read(4))[0]
                    if debug:
                        print socket_num
                    ret = concoord_object.sc_connect(socket_num)
                except Exception,e:
                    print "error occured"
                    ret = -1
                    print e
                map_file.seek(4)
                map_file.write(struct.pack("<i",ret))
                map_file.seek(0)
                map_file.write(struct.pack("<i",0))
                map_file.seek(0)
            elif api_type == 2:
                try:
                    socket_num = struct.unpack("<i",map_file.read(4))[0]
                    flags = struct.unpack("<i",map_file.read(4))[0]
                    length = struct.unpack("<Q",map_file.read(8))[0]
                    if debug:
                        print socket_num
                        print flags
                        print length
                    if length+4*3+8 >= total_size:
                        ret = -1
                    else:
                        data = map_file.read(length)
                        #print type(data)
                        #print len(data)
                        #print "*"*20
                        ret = concoord_object.sc_send(socket_num,data,flags)
                except Exception,e:
                    ret = -1
                    print e
                map_file.seek(4)
                map_file.write(struct.pack("<i",ret))
                map_file.seek(0)
                map_file.write(struct.pack("<i",0))
                map_file.seek(0)
            elif api_type == 3:
                try:
                    socket_num = struct.unpack("<i",map_file.read(4))[0]
                    flags = struct.unpack("<i",map_file.read(4))[0]
                    length = struct.unpack("<i",map_file.read(4))[0]
                    if debug:
                        print socket_num
                        print flags
                        print length
                    ret = concoord_object.sc_recv(socket_num,length,flags)
                except Exception,e:
                    print "error occured"
                    ret = -1
                    print e
                #print ret
                map_file.seek(8)
                if ret[1] is not None:
                    map_file.write(ret[1])
                map_file.seek(4)
                map_file.write(struct.pack("<i",ret[0]))
                map_file.seek(0)
                map_file.write(struct.pack("<i",0))
                map_file.seek(0)
            elif api_type == 4:
                try:
                    socket_num = struct.unpack("<i",map_file.read(4))[0]
                    #print socket_num
                    ret = concoord_object.sc_close(socket_num)
                except Exception,e:
                    #print "error occured"
                    ret = -1
                    #print e
                #print ret
                map_file.seek(0)
                map_file.write(struct.pack("<i",ret))
            else:
                map_file.seek(4)
                map_file.write(struct.pack("<i",-1))
                map_file.seek(0)
                map_file.write(struct.pack("<i",0))
                map_file.seek(0)


if __name__=="__main__":
    main()

