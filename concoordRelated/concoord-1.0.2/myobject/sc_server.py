"""
@author: Milannic Cheng Liu
@note: demo Simple Concoord Server Object that can support socket,connect,rece  
@copyright: See LICENSE
"""
import subprocess
import os
import sysv_ipc
import signal
#import urllib2
import requests
import struct
import socket

class SimpleConcoordServer():
    def __init__(self):
        self.parameter = "-t 2"
        self.bin_path = "/home/milannic/expCPython/concoordRelated/mongoose/mongoose"
        self.running = 0
        self.socket_base = 10000
        self.socket_count = 0
        self.socket_dict = {}
        args = (self.bin_path)
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        self.cpid=popen.pid 
        self.popen = popen
        self.running = 1

    def test(self):
        return 5543

    def scSocket(self,domain,type,protocol):
        try:
            new_socket = socket(domain,type,protocol);
            self.socket_dict[self.socket_count] = new_socket
            self.socket_count = self.socket_count + 1
            return self.socket_base+self.socket_count-1
        except Exception,e:
            return -1

    def scConnect(self,socket_num,address,port):
        try:
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.connect((address,port))
            return 0
        except Exception,e:
            return -1;

    def scSend(self,socket_num,data,flags):
        try:
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.send(data,flags)
            return len(data)
        except Exception,e:
            return -1;

    def scRecv(self,socket_num,buff_size,flags):
        try:
            my_socket = self.socket_dict[socket_num-self.socket_base]
            data = my_socket.recv(buff_size,flags)
            return data
        except Exception,e:
            return -1;
    def scClose(self,socket_num):
        try:
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.close()
            self.socket_dict.pop(socket_num-self.socket_base,None)
            return 0
        except Exception,e:
            return -1;
