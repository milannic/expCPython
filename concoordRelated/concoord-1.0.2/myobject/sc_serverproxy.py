from concoord.clientproxy import ClientProxy
'\n@author: Milannic Cheng Liu\n@note: demo Simple Concoord Server Object that can support socket,connect,rece  At this time we do not involve things about parrot, such as the logical clock or the concoord level function hack. Just To show that our primitive design can work.\n@copyright: See LICENSE\n'
import subprocess
import socket
import os
import signal

class SimpleConcoordServer:

    def __init__(self, bootstrap):
        self.proxy = ClientProxy(bootstrap, token='None')

    def __concoordinit__(self):
        return self.proxy.invoke_command('__concoordinit__')

    def test(self):
        return self.proxy.invoke_command('test')

    def start_server(self, bin_args=None):
        return self.proxy.invoke_command('start_server', bin_args)

    def kill_server(self):
        return self.proxy.invoke_command('kill_server')

    def restart_server(self, bin_args=None):
        return self.proxy.invoke_command('restart_server', bin_args)

    def sc_socket(self, domain, type, protocol):
        return self.proxy.invoke_command('sc_socket', domain, type, protocol)

    def sc_connect(self, socket_num):
        return self.proxy.invoke_command('sc_connect', socket_num)

    def sc_send(self, socket_num, data, flags):
        return self.proxy.invoke_command('sc_send', socket_num, data, flags)

    def sc_recv(self, socket_num, buff_size, flags):
        return self.proxy.invoke_command('sc_recv', socket_num, buff_size, flags)

    def sc_close(self, socket_num):
        return self.proxy.invoke_command('sc_close', socket_num)