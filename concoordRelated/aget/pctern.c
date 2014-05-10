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

// alpha 0.1 version we implement socket -> connect -> send -> recv -> close 5 functions.
// And we haven't handle the corresponding errno

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
#include <assert.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h> 
#include <execinfo.h> 
//#include "Python.h" 
#include "enter_sys.h"
#include "pythonapi.h"

#ifndef PROJECT_TAG
#define PROJECT_TAG "PCTERN"
#endif

#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }

#ifndef MAXSIZE
#define MAXSIZE 1024
#endif

#ifndef LD_DEBUG
#define LD_DEBUG 0
#endif

// for this function actually we 
static int (*fp_socket)(int domain, int type, int protocol);
int socket(int domain, int type, int protocol){

    int ret =-1;
#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake socket function\n");
#endif
        enter_sys();
        ret = sc_socket(domain,type,protocol);
        leave_sys();
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real socket function\n");
#endif
        RESOLVE(socket);
        ret = fp_socket(domain,type,protocol);
    }
//   errno = 22;
     return ret;
}


//connect
static int (*fp_connect)(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len){
    int ret =-1;
#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_connect(socket,address,address_len);
        leave_sys();
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(connect);
        ret = fp_connect(socket,address,address_len);
    }
//   errno = 22;
     return ret;
}



static ssize_t (*fp_send)(int socket, const void *buffer, size_t length, int flags);
ssize_t send(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret =-1;
#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_send(socket,buffer,length,flags);
        leave_sys();
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(send);
        ret = fp_send(socket,buffer,length,flags);
    }
//   errno = 22;
     return ret;
};



static ssize_t (*fp_recv)(int socket, void *buffer, size_t length, int flags);
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
    ssize_t ret =-1;
#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_recv(socket,buffer,length,flags);
        leave_sys();
    }else{
#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(recv);
        ret = fp_recv(socket,buffer,length,flags);
    }
//   errno = 22;
     return ret;
}


//close
static int (*fp_close)(int fd);
int close(int fd) {
    int ret =-1;

#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif

    if(!check_sys()){

#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif

        enter_sys();
        ret = sc_close(fd);
        leave_sys();
    }else{

#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(close);
        ret = fp_close(fd);
    }
//   errno = 22;
     return ret;
}

static void (*fp_pthread_exit)(void* retval);
void pthread_exit(void* retval) {
    int ret =-1;

#if LD_DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif

    if(!check_sys()){

#if LD_DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif

        enter_sys();
        sc_pthread_exit(retval);
        leave_sys();
    }else{

#if LD_DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(pthread_exit);
        fp_pthread_exit(retval);
    }
}

