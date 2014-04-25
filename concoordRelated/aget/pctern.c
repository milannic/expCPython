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



// this should be in a configuration file in the later time
static char* module_name = "sc_serverproxy";
static char* class_name = "SimpleConcoordServer";
static char* replica_group = "127.0.0.1:14001";

// with embedded python, we must carefully check every PyObject's reference to keep python runtime work correctly

static PyObject *pname=NULL,*pmodule=NULL,*pdict=NULL,*pclass=NULL;

static volatile int flag_python_runtime = 0;
static volatile int flag_concoord_module = 0;
static volatile int ready=0;
// to maintain a python 
// each thread has its only copy, so we don't need to worry about the synchronous issues

static __thread PyObject *pins=NULL;

static pthread_mutex_t init_lock;
static pthread_mutex_t check_lock;

// initialize the python runtime for each thread, as for the finalize, 
// it should be done in the exit signal handler of the program.
//
static int check_ready(void);
static int check_and_set_ready(void);

static int init_python(void);
static void final_python(void);

static int load_python_runtime(void);
static void unload_python_runtime(void);

static int load_concoord_module(void);
static void unload_concoord_module(void);

static int create_ins(void);
static void destory_ins(void);

//non static area
int sc_socket(int domain,int type,int protocol);



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

    pmodule = PyImport_Import(pname);  
    if(!pmodule)
    {  
        fprintf(stderr,"can't find concoord module\n");  
        goto load_module_error;  
    }  

    pdict = PyModule_GetDict(pmodule);  
    if(!pdict)           
    {  
        fprintf(stderr,"can't get the dict of concoord module\n");  
        goto load_module_error;  
    }  

    pclass = PyDict_GetItemString(pdict, class_name);  
    if(!pclass)           
    {  
        fprintf(stderr,"can't find class SimpleConcoordServer\n");  
        goto load_module_error;  
    }  

    #if DEBUG
    if(PyClass_Check(pclass)){
        fprintf(stderr,"we have found the class\n");
    }
    #endif

    flag_concoord_module = 1;
    ret = 0;
    goto load_module_exit;

load_module_error:
    if(NULL!=pname){Py_DECREF(pname);pname=NULL;}
    if(NULL!=pmodule){Py_DECREF(pmodule);pmodule=NULL;}
    if(NULL!=pdict){Py_DECREF(pdict);pdict=NULL;}
    if(NULL!=pclass){Py_DECREF(pclass);pclass=NULL;}
load_module_exit:
    return ret;
}

void unload_concoord_module(void){
    if(flag_concoord_module){
        if(NULL!=pname){Py_DECREF(pname);pname=NULL;}
        if(NULL!=pmodule){Py_DECREF(pmodule);pmodule=NULL;}
        if(NULL!=pdict){Py_DECREF(pdict);pdict=NULL;}
        if(NULL!=pclass){Py_DECREF(pclass);pclass=NULL;}
        flag_concoord_module = 0;
    }
}

int init_python(void){
    int ret=-1;
    pthread_mutex_lock(&init_lock);
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
    pthread_mutex_unlock(&init_lock);
    return ret;
}

void final_python(void){
    pthread_mutex_lock(&init_lock);
    if(ready==1){
        if(flag_concoord_module){
            unload_concoord_module();
        }
        if(flag_python_runtime){
            unload_python_runtime();
        }
        ready = 0;
    }
    pthread_mutex_unlock(&init_lock);
}


int check_ready(void){
    int ret;
    pthread_mutex_lock(&check_lock);
    ret = ready;
    pthread_mutex_unlock(&check_lock);
    return ret;
}



int check_and_set_ready(void){
    int ret;
    pthread_mutex_lock(&check_lock);
    ret = ready;
    if(ret==0){
        if(!init_python()){
            ret = 1;
        }
    }
    pthread_mutex_unlock(&check_lock);
    return ret;
}


//for now we don't know whether creating python class instance will do write to the class object, then no mutex used, we will check this in detail in the later time
//
int create_ins(void){
    int ret = -1;
    PyObject *pargs=NULL;

#if DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif

    if(!check_ready()){
        if(!check_and_set_ready()){
            goto create_ins_error;
        }
    }

	pargs = PyTuple_New(1);
    if(!pargs){
		fprintf(stderr,"we cannot create the first tuple\n");  
        goto create_ins_error;
    }


	PyTuple_SetItem(pargs,0,Py_BuildValue("s",replica_group));

	fprintf(stderr,"%p\n",pclass);  

	pins = PyInstance_New(pclass,pargs,NULL);

	if(!pins){
		fprintf(stderr,"we cannot create the instance\n");  
        goto create_ins_error;
	}
	if(PyInstance_Check(pins)){
		fprintf(stderr,"Sure, We have created an instance\n");  
	}else{
        goto create_ins_error;
    }
    ret = 0;
    goto create_ins_exit;
create_ins_error:
    if(NULL!=pargs){Py_DECREF(pargs);pargs=NULL;};
    if(NULL!=pins){Py_DECREF(pins);pins=NULL;};
create_ins_exit:
    return ret;
}


void destory_ins(void){
    if(NULL!=pins){
        Py_DECREF(pins);
        pins=NULL;
    }
}





int sc_socket(int domain,int type,int protocol){
    int ret = -1;
    PyObject *pretval=NULL;  
#if DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_socket_error;
        }
    }
     
#if DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
	pretval=PyObject_CallMethod(pins,"sc_socket","(i,i,i)",domain,type,protocol);
	if(!pretval){
		fprintf(stderr,"we cannot create the second tuple\n");  
        goto sc_socket_error;
	}
    ret = (int)PyInt_AsLong(pretval);
    fprintf(stderr,"we call the sc_socket method, and the return value is %d\n",ret);
    goto sc_socket_exit;

sc_socket_error:
    if(NULL!=pretval){Py_DECREF(pretval);};
sc_socket_exit:
    return ret;
}


int sc_connect(int socket, const struct sockaddr *address, socklen_t address_len){
    int ret = -1;
    PyObject *pretval=NULL;  
#if DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_connect_error;
        }
    }
     
#if DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
	pretval=PyObject_CallMethod(pins,"sc_connect","(i)",socket);
	if(!pretval){
		fprintf(stderr,"we cannot create the second tuple\n");  
        goto sc_connect_error;
	}
    ret = (int)PyInt_AsLong(pretval);
    fprintf(stderr,"we call the sc_connect method, and the return value is %d\n",ret);
    goto sc_connect_exit;

sc_connect_error:
    if(NULL!=pretval){Py_DECREF(pretval);};
sc_connect_exit:
    return ret;
}



ssize_t sc_send(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret = -1;
    PyObject *pretval=NULL;  
#if DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_send_error;
        }
    }
#if DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
	pretval=PyObject_CallMethod(pins,"sc_send","(i,s,i)",socket,buffer,flags);
	if(!pretval){
		fprintf(stderr,"we cannot create the second tuple\n");  
        goto sc_send_error;
	}
    ret = (int)PyInt_AsLong(pretval);
    fprintf(stderr,"we call the sc_send method, and the return value is %d\n",ret);
    goto sc_send_exit;

sc_send_error:
    if(NULL!=pretval){Py_DECREF(pretval);};
sc_send_exit:
    return ret;
}


ssize_t sc_recv(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret = -1;
    PyObject *pretval=NULL;  
#if DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_recv_error;
        }
    }
#if DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
	pretval=PyObject_CallMethod(pins,"sc_recv","(i,i,i)",socket,length,flags);
	if(!pretval){
		fprintf(stderr,"we cannot create the second tuple\n");  
        goto sc_recv_error;
	}

    ret = (int)PyInt_AsLong(pretval);
    fprintf(stderr,"we call the sc_recv method, and the return value is %s\n",ret);
    goto sc_recv_exit;

sc_recv_error:
    if(NULL!=pretval){Py_DECREF(pretval);};
sc_recv_exit:
    return ret;
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
    }
//   errno = 22;
     return ret;
}


//connect
static int (*fp_connect)(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len){
    int ret =-1;
#if DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_connect(socket,address,address_len);
        leave_sys();
    }else{
#if DEBUG
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
#if DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_send(socket,buffer,length,flags);
        leave_sys();
    }else{
#if DEBUG
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
#if DEBUG
    fprintf(stderr,"now check_sys() = %d \n",check_sys());
#endif
    if(!check_sys()){
#if DEBUG
        fprintf(stderr,"now I am calling the fake %s function\n",__FUNCTION__);
#endif
        enter_sys();
        ret = sc_recv(socket,buffer,length,flags);
        leave_sys();
    }else{
#if DEBUG
        fprintf(stderr,"now I am calling the real %s function\n",__FUNCTION__);
#endif
        RESOLVE(recv);
        ret = fp_recv(socket,buffer,length,flags);
    }
//   errno = 22;
     return ret;
}

#if 0
//close
static int (*fp_close)(int fd);
int close(int fd) {
  int ret=0;
  return ret;
}
#endif
