/* Copyright (c) 2013,  Regents of the Columbia University 
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

#define _GNU_SOURCE
//#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/socket.h>
#include <execinfo.h>
//Milannic
#include "Python.h"

//Milannic
int my_socket(int domain, int type, int protocol){
	PyObject *pName,*pModule,*pDict,*pClass,*pIns,*pArgs,*pRetval;
	int ret;
	if(!Py_IsInitialized()){
		Py_Initialize();       
	}
    if(!Py_IsInitialized()){
		fprintf(stderr,"cannot initialize Python Runntime\n");
		return -1;
	}
    pName = PyString_FromString("concoord.proxy.counter");  
    pModule = PyImport_Import(pName);  
    if (!pModule )   
    {  
        fprintf(stderr,"can't find concoord module");  
        return -1;  
    }  
    pDict = PyModule_GetDict(pModule);  
    if ( !pDict )           
    {  
        fprintf(stderr,"can't find concoord namespace");  
        return -1;  
    }  
  
    // 找出函数名为add的函数  
    pClass = PyDict_GetItemString(pDict, "Counter");  
    if ( !pClass)           
//    if ( !pClass || !PyCallable_Check(pFunc) )           
    {  
        printf("can't find class Counter");  
        getchar();  
        return -1;  
    }  
	if(PyClass_Check(pClass)){
		printf("we have found the class\n");
	}
	pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs,0,Py_BuildValue("s","127.0.0.1:14000"));
	pIns = PyInstance_New(pClass,pArgs,NULL);
	if(!pIns){
		printf("we cannot create the instance\n");  
	}
	sleep(2);
	if(PyInstance_Check(pIns)){
		printf("Sure, We have created an instance\n");  
	}
	Py_DECREF(pName);
	Py_DECREF(pModule);
	Py_DECREF(pClass);
	ret=(int)pIns;
	return ret;
}

int my_shutdown(int socket, int how){
	PyObject * py_socket;
	if(socket>0){
		py_socket= (PyObject*)(socket);
		Py_DECREF(py_socket);
	}
    Py_Finalize();  
	return 0;
}

