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
#include <sys/socket.h> 
#include <pthread.h>
#include <execinfo.h> 
#include "pythonapi.h"

#ifndef PROJECT_TAG
#define PROJECT_TAG "PCTERN"
#endif

#ifndef MAXSIZE
#define MAXSIZE 1024
#endif

#ifndef PY_DEBUG
#define PY_DEBUG 0
#endif

#ifndef REG_FILE_NAME
#define REG_FILE_NAME 24
#endif


// this should be in a configuration file in the later time
static char* module_name = "sc_serverproxy";
static char* class_name = "SimpleConcoordServer";
static char* replica_group = "127.0.0.1:14001";

// with embedded python, we must carefully check every PyObject's reference to keep python runtime work correctly

// to maintain a python 
// each thread has its only copy, so we don't need to worry about the synchronous issues




static int random_gen(char* buffer,int length){
    int index;
    int temp;
    srand(time(NULL));
    for( index = 0; index < length-1; index += 1 ) {
        temp = rand()%ALPHA_DICT+97;
        buffer[index] = temp;
//        printf("the current alpha dict is %d\n",temp);
//        printf("the current alpha dict is %c\n",temp);
    }
    buffer[length-1] = '\0';
    return 0;
}


int sc_socket(int domain,int type,int protocol){
    int ret = -1;
    FILE* fp=NULL;
    pid_t sub_python;
    char* temp_file_name = (char*)malloc(sizeof(char)*REG_FILE_NAME);
    random_gen(temp_file_name,REG_FILE_NAME);
    sub_python = fork();
    assert(sub_python>=0&&"fork error");
    // child process
    if(!sub_python){
        if(execl("./python_network_proxy.py","./python_network_proxy.py","socket",temp_file_name,NULL)<0){
            fprintf(std,"error when execute the python script\n");
            fprintf(std,"the error code is %d\n",errno);
            return EXIT_FAILURE;
        }else{
            return EXIT_SUCCESS;
        }
    }else{
        fp=fopen(REG_FILE_NAME,"r");
        if(fscanf(fp,"%d\n",&ret)<0){
            fprintf(std,"error when reading the temporary file\n");
            fprintf(std,"the error code is %d\n",errno);
            goto sc_scoket_exit;
        }else{
            //remove(REG_FILE_NAME);
        }
    }
    
sc_scoket_exit;
    return -1;
    //return ret;
}


int sc_connect(int socket, const struct sockaddr *address, socklen_t address_len){
    int ret = -1;
    PyObject *pretval=NULL;  
#if PY_DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_connect_error;
        }
    }
     
#if PY_DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
    pthread_spin_lock(&spin_lock);
	pretval=PyObject_CallMethod(pins,"sc_connect","(i)",socket);
    pthread_spin_unlock(&spin_lock);
	if(!pretval){
#if PY_DEBUG
		fprintf(stderr,"%s runs improperly \n",__FUNCTION__);  
#endif
        goto sc_connect_error;
	}
    pthread_spin_lock(&spin_lock);
    ret = (int)PyInt_AsLong(pretval);
    pthread_spin_unlock(&spin_lock);
#if PY_DEBUG
    fprintf(stderr,"we call the sc_connect method, and the return value is %d\n",ret);
#endif
    //goto sc_connect_exit;

sc_connect_error:
    pthread_spin_lock(&spin_lock);
    if(NULL!=pretval){Py_DECREF(pretval);};
    pthread_spin_unlock(&spin_lock);
sc_connect_exit:
    return ret;
}



ssize_t sc_send(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret = -1;
    PyObject *pretval=NULL;  
#if PY_DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_send_error;
        }
    }
#if PY_DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
    pthread_spin_lock(&spin_lock);
	pretval=PyObject_CallMethod(pins,"sc_send","(i,s,i)",socket,buffer,flags);
    pthread_spin_unlock(&spin_lock);
	if(!pretval){
#if PY_DEBUG
		fprintf(stderr,"%s runs improperly \n",__FUNCTION__);  
#endif
        goto sc_send_error;
	}
    pthread_spin_lock(&spin_lock);
    ret = (int)PyInt_AsLong(pretval);
    pthread_spin_unlock(&spin_lock);
#if PY_DEBUG
    fprintf(stderr,"we call the sc_send method, and the return value is %d\n",ret);
#endif
    //goto sc_send_exit;

sc_send_error:
    pthread_spin_lock(&spin_lock);
    if(NULL!=pretval){Py_DECREF(pretval);};
    pthread_spin_unlock(&spin_lock);
sc_send_exit:
    return ret;
}


ssize_t sc_recv(int socket, const void *buffer, size_t length, int flags){
    ssize_t ret = -1;
    PyObject *pretval=NULL;  
    PyObject *pretcode=NULL;
    PyObject *pretstring=NULL;

#if PY_DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
#if PY_DEBUG
            fprintf(stderr,"we cannot create the ins\n");  
#endif
            goto sc_recv_error;
        }
    }
#if PY_DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
    pthread_spin_lock(&spin_lock);
	pretval=PyObject_CallMethod(pins,"sc_recv","(i,i,i)",socket,length,flags);
    pthread_spin_unlock(&spin_lock);
	if(NULL==pretval){
#if PY_DEBUG
		fprintf(stderr,"%s runs improperly\n",__FUNCTION__);  
#endif
        goto sc_recv_error;
	}
    // and PyTuple_GetItem returns borrowed reference, then we don't need to Py_DECREF them
    pthread_spin_lock(&spin_lock);
    pretcode = PyTuple_GetItem(pretval,0); 
    pthread_spin_unlock(&spin_lock);
    if(NULL==pretcode){
#if PY_DEBUG
		fprintf(stderr,"%s return code error\n",__FUNCTION__);  
#endif
        goto sc_recv_error;
    }
    pthread_spin_lock(&spin_lock);
    ret = (int)PyInt_AsLong(pretcode);
    pthread_spin_unlock(&spin_lock);

    if(-1!=ret){
        pthread_spin_lock(&spin_lock);
        pretstring = PyTuple_GetItem(pretval,1);
        pthread_spin_unlock(&spin_lock);
        if(NULL==pretstring){
            fprintf(stderr,"%s return string error\n",__FUNCTION__);  
            goto sc_recv_error;
        }
#if PY_DEBUG
        fprintf(stderr,"we call the sc_recv method, and the return value is %s\n",PyString_AsString(pretstring));
#endif
        pthread_spin_lock(&spin_lock);
        memcpy(buffer,(const void*)PyString_AsString(pretstring),length);
        pthread_spin_unlock(&spin_lock);
#if PY_DEBUG
        fprintf(stderr,"we call the sc_recv method, and the return value is\n %s\n",buffer);
    }
    fprintf(stderr,"we call the sc_recv method, and the return value is %d\n",ret);
#else
    }
#endif
    //goto sc_recv_exit;
sc_recv_error:
    pthread_spin_lock(&spin_lock);
    if(NULL!=pretval){Py_DECREF(pretval);};
    if(NULL!=pretcode){Py_DECREF(pretcode);};
    if(NULL!=pretstring){Py_DECREF(pretstring);};
    pthread_spin_unlock(&spin_lock);
sc_recv_exit:
    return ret;
}


int sc_close(int fd){
    int ret = -1;
    PyObject *pretval=NULL;  
#if PY_DEBUG
    fprintf(stderr,"I am here %s\n",__FUNCTION__);
#endif
    if(NULL==pins){
        if(-1==create_ins()){
        // we cannot create the instance then we cannot do the next
            fprintf(stderr,"we cannot create the ins\n");  
            goto sc_close_error;
        }
    }
#if PY_DEBUG
    fprintf(stderr,"I am here %s 2\n",__FUNCTION__);
    fprintf(stderr,"%p\n",pins);
#endif
    pthread_spin_lock(&spin_lock);
	pretval=PyObject_CallMethod(pins,"sc_close","(i)",fd);
    pthread_spin_unlock(&spin_lock);
	if(!pretval){
#if PY_DEBUG
		fprintf(stderr,"%s runs improperly \n",__FUNCTION__);  
#endif
        goto sc_close_error;
	}
    pthread_spin_lock(&spin_lock);
    ret = (int)PyInt_AsLong(pretval);
    pthread_spin_unlock(&spin_lock);
#if PY_DEBUG
    fprintf(stderr,"we call the sc_close method, and the return value is %d\n",ret);
#endif
    //goto sc_close_exit;

sc_close_error:
    pthread_spin_lock(&spin_lock);
    if(NULL!=pretval){Py_DECREF(pretval);};
    pthread_spin_unlock(&spin_lock);
sc_close_exit:
    return ret;
}
