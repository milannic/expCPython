"""
@author: Milannic Cheng Liu
@note: demo Simple Concoord Server Object that can support socket,connect,rece  At this time we do not involve things about parrot, such as the logical clock or the concoord level function hack. Just To show that our primitive design can work.
@copyright: See LICENSE
"""
# and those admin function can only be processed by the administer, at the later time we will add the private key to it


import subprocess
import socket
import os
import signal
from timeout import timeout
#import urllib2
#import sysv_ipc
#import requests
#import struct

class SimpleConcoordServer():
    #Now we just hard core
    def __init__(self):
        self.running = 0
        #default parameter
        self.bin_args = ["/home/milannic/expCPython/concoordRelated/mongoose/mongoose","-t 2"]
        #default path
        self.socket_base = 10000
        self.socket_count = 0
        self.socket_dict = {}
        self.running = 0
        self.debug=1

    #test function, just to make sure this concoord object works
    def test(self):
        return 5543


    #start_server 
    def start_server(self,bin_args=None):
        #if starting server in the initialization failed, we should manually start it with this function 
        if bin_args is not None:
            self.bin_args = bin_args
        if not self.running:
            try:
                args = self.bin_args
                #popen = subprocess.Popen(args, stdout=subprocess.PIPE)
                self.cpid=os.fork()
                if len(self.bin_args) > 1:
                    args=self.bin_args[1:]
                else:
                    args = [""]
                if not self.cpid:
                    os.execv(self.bin_args[0],args)
                print self.cpid
                try:
                    os.kill(int(self.cpid), 0)
                    self.running = 1
                except Exception,e:
                    return -1
            except Exception,e:
                print self.cpid
                print e
                self.kill_server()
                return -1
            return 0
        else:
            return 0


    def kill_server(self):
        if self.running:
            try:
                os.kill(int(self.cpid), signal.SIGTERM)
                os.wait()[0]
            except Exception,e:
                print e
            # Check if the process that we killed is alive.
            # If the process has been killed, then it should raise a exception
            try: 
               if self.debug:
                   print "we test whether it is still here"
               os.kill(int(self.cpid), 0)
               return -1
            except Exception,e:
                print e
                self.running = 0
                return 0
        else:
            return 0
        
    def restart_server(self,bin_args=None):
        #if starting server in the initialization failed, we should manually start it with this function 
        if not self.kill_server():
            if not self.start_server(bin_args):
                return 0
        return -1

    def sc_socket(self,domain,type,protocol):
        try:
            new_socket = socket.socket(domain,type,protocol);
            self.socket_dict[self.socket_count] = new_socket
            self.socket_count = self.socket_count + 1
            if self.debug:
                print self.socket_dict
            user_account=self.socket_base+self.socket_count-1
            return user_account 
        except Exception,e:
            print e
            return -1

#    def scConnect(self,socket_num,address,port):
#        try:
#            my_socket = self.socket_dict[socket_num-self.socket_base]
#            my_socket.connect((address,port))
#            return 0
#        except Exception,e:
#            return -1;

    def sc_connect(self,socket_num):
        try:
            if self.debug:
                print "*"*10+"now we are connect"+"*"*10
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.connect(("127.0.0.1",8080))
            if self.debug:
                print socket_num
                print "connect is called successfully"
                print "*"*20
            return 0
        except Exception,e:
            print e
            return -1;

    def sc_send(self,socket_num,data,flags):
        try:
            if self.debug:
                print "*"*10+"now we are send"+"*"*10
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.send(data,flags)
            if self.debug:
                print socket_num
                print data
                print "send is called successfully"
                print "*"*20
            return len(data)
        except Exception,e:
            print e
            return -1;

    #@timeout(60)
    def sc_recv(self,socket_num,buff_size,flags):
        try:
            if self.debug:
                print "*"*10+"now we are recv"+"*"*10
            my_socket = self.socket_dict[socket_num-self.socket_base]
            data = my_socket.recv(buff_size,flags)
            if self.debug:
                print socket_num
                print data
                print "recv is called successfully"
                print "*"*20
            return (len(data),data)
        except Exception,e:
            print e
            return (-1,None);

    def sc_close(self,socket_num):
        try:
            my_socket = self.socket_dict[socket_num-self.socket_base]
            my_socket.close()
            self.socket_dict.pop(socket_num-self.socket_base,None)
            return 0
        except Exception,e:
            print e
            return -1;
