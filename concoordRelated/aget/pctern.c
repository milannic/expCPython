/* Copyright (c) 2014,  Regents of the Columbia University 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
 * materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Use LD_PRELOAD to intercept some important libc function calls for diagnosing x86 programs.
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#ifndef _LARGEFILE_SOURCE     /* ----- #if 0 : If0Label_2 ----- */
#define _LARGEFILE_SOURCE
#endif     /* ----- #if 0 : If0Label_2 ----- */

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/socket.h>
#include <execinfo.h>
#include "Python.h"
#include "enter_sys.h"

#define PROJECT_TAG "PCTERN"
#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }
#define MAXSIZE 1024

// each thread has its only copy, so we don't need to worry about the synchronous issues
static __thread int entered = 0; 

// alpha 0.1 version we implement socket -> connect -> send -> recv -> close 5 functions.


// initialize the python runtime for each thread, as for the finalize, 
// it should be done in the exit signal handler of the program.
static int init_python(void);
int init_python(void){
    if(!Py_IsInitialized()){
        Py_Initialize();       
    }
    if(Py_IsInitialized()){
        return 1;
    }else{
        return 0;
    }
}


// for this function actually we 
static int (*fp_socket)(int domain, int type, int protocol);
int socket(int domain, int type, int protocol){
    if(check_sys()){
        fprintf(stderr,"now we have entered the sys\n");
    }else{
        fprintf(stderr,"now we have not entered the sys\n");
    }
    int ret =0;
    return -1;
}


//connect
static int (*fp_connect)(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len){
  struct sockaddr_in* in_address=(struct sockaddr_in*)address;
  fprintf(stderr,"socket : %d , port : %u , addr : %u , socklen : %d \n",socket,in_address->sin_port,in_address->sin_addr.s_addr,(int)address_len);
  fprintf(stderr,"the address length is %lu\n",sizeof(in_address->sin_addr.s_addr));
  int ret = fp_connect(socket,address,address_len);
  return ret;
}



static ssize_t (*fp_send)(int socket, const void *buffer, size_t length, int flags);

ssize_t send(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret=0;
  fprintf(stderr,"socket : %d , content : %s , length : %d \n",socket,(char*)buffer,(int)length);
  //ssize_t ret = fp_send(socket, buffer, length, flags);
  return ret;
};


static int (*fp_recv)(int socket, void *buffer, size_t length, int flags);
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
  int ret = fp_recv(socket, buffer, length, flags);
  return ret;
}

//close
static int (*fp_close)(int fd);
int close(int fd) {
  int ret=0;
  return ret;
}
