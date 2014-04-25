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
#define DEBUG 1
#define MAXSOCKET 64



// this should be in a configuration file in the later time
static char* module_name = "sc_serverproxy";
static char* class_name = "SimpleConcoordServer";
static char* replica_group = "127.0.0.1:14001";

// with embedded python, we must carefully check every PyObject's reference to keep python runtime work correctly,then we maintain each of them of a int flag

// and this should be replace by a more elegant way.
static PyObject *pscoket_pool[MAXSOCKET] = {NULL};

static PyObject *pname=NULL,*pmodule=NULL,*pdict=NULL,*pclass=NULL;
static int flag_pname = 0,flag_pmodule=0,flag_pdict=0,flag_pclass=0;

static int flag_python_runtime = 0;
static int flag_concoord_module = 0;
static int ready=0;

// to maintain a python

// each thread has its only copy, so we don't need to worry about the synchronous issues
static __thread int entered = 0; 
//static __thread PyObject *pins=NULL;

static pthread_mutex_t lock;

// initialize the python runtime for each thread, as for the finalize, 
// it should be done in the exit signal handler of the program.

static int init_python(void);
static void final_python(void);

static int load_python_runtime(void);
static void unload_python_runtime(void);

static int load_concoord_module(void);
static void unload_concoord_module(void);

//init the python runtime 
int load_python_runtime(void){
    if(!Py_IsInitialized()){
        Py_Initialize();       
    }
    if(Py_IsInitialized()){
#if DEBUG
        fprintf(stderr,"Python Runtime Have Been Initialized\n");
#endif
        flag_python_runtime = 1;
        return 0;
    }else{
        return -1;
    }
}

void unload_python_runtime(void){
    if(Py_IsInitialized()){
        Py_Finalize();       
        flag_python_runtime = 0;
    }
}

//init the concoord module
int load_concoord_module(void){
    int ret=-1;

    pname = PyString_FromString(module_name);  
    if(!pname)   
    {  
        fprintf(stderr,"can't build python string object\n");  
        goto load_module_error;  
    }  
    flag_pname = 1;

    pmodule = PyImport_Import(pname);  
    if(!pmodule)
    {  
        fprintf(stderr,"can't find concoord module\n");  
        goto load_module_error;  
    }  
    flag_pmodule = 1;

    pdict = PyModule_GetDict(pmodule);  
    if(!pdict)           
    {  
        fprintf(stderr,"can't get the dict of concoord module\n");  
        goto load_module_error;  
    }  
    flag_pdict = 1; 

    pclass = PyDict_GetItemString(pdict, "SimpleConcoordServer");  
    if(!pclass)           
    {  
        fprintf(stderr,"can't find class SimpleConcoordServer\n");  
        goto load_module_error;  
    }  
    flag_pclass = 1;

    #if DEBUG
    if(PyClass_Check(pclass)){
        fprintf(stderr,"we have found the class\n");
    }
    #endif

    flag_concoord_module = 1;
    ret = 0;
    goto load_module_exit;

load_module_error:
    if(flag_pname)Py_DECREF(pname);
    if(flag_pmodule)Py_DECREF(pmodule);
    if(flag_pdict)Py_DECREF(pdict);
    if(flag_pclass)Py_DECREF(pclass);
load_module_exit:
    return ret;
}

void unload_concoord_module(void){
    if(flag_concoord_module){
        if(flag_pname)Py_DECREF(pname);
        if(flag_pmodule)Py_DECREF(pmodule);
        if(flag_pdict)Py_DECREF(pdict);
        if(flag_pclass)Py_DECREF(pclass);
        flag_concoord_module = 0;
    }
}

int init_python(void){
    int ret=-1;
    pthread_mutex_lock(&lock);
    if(!flag_python_runtime){
        if(-1==load_python_runtime()){
            goto init_exit;
        }
    }
    if(!flag_concoord_module){
        if(-1==load_concoord_module()){
            goto init_exit;
        }
    }
    ret = 0;
    ready = 1;
init_exit:
    pthread_mutex_unlock(&lock);
    return ret;
}

void final_python(void){
    pthread_mutex_lock(&lock);
    if(flag_concoord_module){
        unload_concoord_module();
    }
    if(flag_python_runtime){
        unload_python_runtime();
    }
    ready = 0;
    pthread_mutex_unlock(&lock);
}


int sc_socket(int domain,int type,int protocol){
    int ret = -1;
    PyObject *pargs1=NULL,*pargs2=NULL,*pretval=NULL,*pins=NULL;  
    int flag_pargs1=0,flag_pargs2=0,flag_pretval=0,flag_pins=0;  

    if(!ready){
        if(-1==init_python()){
            goto sc_socket_error;
        }
    }

	pargs1 = PyTuple_New(1);
    if(!pargs1){
		fprintf(stderr,"we cannot create the first tuple\n");  
        goto sc_socket_error;
    }
    flag_pargs1=1;
	PyTuple_SetItem(pargs1,0,Py_BuildValue("s",replica_group));
	fprintf(stderr,"%p\n",pclass);  

	pins = PyInstance_New(pclass,pargs1,NULL);

	if(!pins){
		fprintf(stderr,"we cannot create the instance\n");  
        goto sc_socket_error;
	}
    flag_pins=1;
	sleep(2);
    
#if DEBUG
	if(PyInstance_Check(pins)){
		fprintf(stderr,"Sure, We have created an instance\n");  
	}
#endif

//	pargs2 = Py_BuildValue("(i,i,i)",domain,type,protocol);
//    if(!pargs2){
//		fprintf(stderr,"we cannot create the first tuple\n");  
//        goto sc_socket_error;
//    }
//    flag_pargs2=1;

	pretval=PyObject_CallMethod(pins,"sc_socket","(i,i,i)",domain,type,protocol);

	if(!pretval){
		fprintf(stderr,"we cannot create the second tuple\n");  
        goto sc_socket_error;
	}

    flag_pretval=1;
    ret = (int)PyInt_AsLong(pretval);
    fprintf(stderr,"we call the sc_socket method, and the return value is %d\n",ret);

sc_socket_error:
    if(flag_pins)Py_DECREF(pins);
sc_socket_exit:
    if(flag_pargs1)Py_DECREF(pargs1);
    if(flag_pargs2)Py_DECREF(pargs2);
    if(flag_pretval)Py_DECREF(pretval);
    return -1;
}




// for this function actually we 
static int (*fp_socket)(int domain, int type, int protocol);
int socket(int domain, int type, int protocol){
    int ret =-1;
#if DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if DEBUG
        fprintf(stderr,"now I am calling the fake socket function\n");
#endif
        enter_sys();
        ret = sc_socket(domain,type,protocol);
        leave_sys();
    }else{
#if DEBUG
        fprintf(stderr,"now I am calling the real socket function\n");
#endif
        RESOLVE(socket);
        ret = fp_socket(domain,type,protocol);
        return ret;
    }
    // we enter the sys area avoiding intercept other system call
#if 0
    enter_sys();
    if(check_sys()){
        fprintf(stderr,"now we have entered the sys\n");
    }else{
        fprintf(stderr,"now we have not entered the sys\n");
    }
    leave_sys();
    if(check_sys()){
        fprintf(stderr,"now we have entered the sys\n");
    }else{
        fprintf(stderr,"now we have not entered the sys\n");
    }
#endif
//   errno = 22;
socket_exit:
    return -1;
}


//connect
//
#if 0
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
#endif
