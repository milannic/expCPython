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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <execinfo.h> 
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/syscall.h>
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

#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }

// this should be in a configuration file in the later time
//static char* module_name = "sc_serverproxy";
//static char* class_name = "SimpleConcoordServer";
//static char* replica_group = "127.0.0.1:14001";

// with embedded python, we must carefully check every PyObject's reference to keep python runtime work correctly

// to maintain a python 
// each thread has its only copy, so we don't need to worry about the synchronous issues

#ifndef DEFAULT_MEM_LENGTH
#define DEFAULT_MEM_LENGTH 4096
#endif

#ifndef ALPHA_DICT
#define ALPHA_DICT 26
#endif

#ifndef SHARED_MEM_KEY_LEN
#define SHARED_MEM_KEY_LEN 20
#endif



#define INIT_OP(op) if(shared_memory_ptr==NULL){sc_create_shared_mem();} \
    if(shared_memory_ptr==NULL){goto sc_##op##_exit;} \
    if(check_subprocess_dead()){waitpid(child_pid,NULL,0);goto sc_##op##_exit;}

#if 0
#define SIMPLE_LOOP_OP(op) while(*(int*)shared_memory_ptr==-1){ \
        fprintf(stderr,"haha\n");\
        getchar();\
        if(check_subprocess_dead()){ \
            waitpid(child_pid,NULL,0); \
            goto sc_##op##_exit; \
        }}
#endif

#if 1
#define SIMPLE_LOOP_OP(op) while(*(int*)shared_memory_ptr==-1){ \
        if(check_subprocess_dead()){ \
            waitpid(child_pid,NULL,0); \
            goto sc_##op##_exit; \
        }}
#endif

static int count=100;
static pthread_mutex_t count_lock;
static __thread char* shared_memory_key=NULL;
static __thread int shared_memory_fd=-1;
static __thread void* shared_memory_ptr=NULL;
static __thread size_t total_length;
static __thread pid_t child_pid=-1;

static void sc_clean_shared_mem(void);

//Function communicates with python script by send a 4 bytes integer in the first 4 bytes of the shared memory,and each request send by the c program will write the first 4 bytes integer to a [1,2,3,4,5] corresponding api_type,and when python finish writing, it will return a -1 to the first 4 bytes and with the following return code.
//
//notice that there maybe a buffer data more than 4096*4 bytes(16kb),then when we check the length is more than 16kb, there will be multiple sending.

static int random_gen(char* buffer,int length){
    int index;
    int temp;
    pthread_mutex_lock(&count_lock);
    srand(time(NULL)+count);
    count+=100;
    for( index = 0; index < length-1; index += 1 ) {
        temp = rand()%ALPHA_DICT+97;
        buffer[index] = temp;
    }
    buffer[length-1] = '\0';
    pthread_mutex_unlock(&count_lock);
    return 0;

}

static int sc_create_shared_mem(void){
    int op_ret;

    total_length = sizeof(int)*DEFAULT_MEM_LENGTH;
    shared_memory_key = (char*)malloc(sizeof(char)*SHARED_MEM_KEY_LEN);
    random_gen(shared_memory_key,SHARED_MEM_KEY_LEN);

#if PY_DEBUG
    fprintf(stderr,"my random string is %s\n",shared_memory_key);
#endif

    shared_memory_fd = shm_open(shared_memory_key,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH);

    if(shared_memory_fd!=-1){
#if PY_DEBUG
        fprintf(stderr,"the shared memory descriptor is %d\n",shared_memory_fd);
        fprintf(stderr,"creating the shared memory succeed\n");
#endif
        ;
    }else{
        fprintf(stderr,"creating the shared memory failed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        // no need to clean
        goto sc_create_shared_mem_error_exit;
    }

    op_ret = ftruncate(shared_memory_fd,total_length);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
    }else{
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_create_shared_mem_error_exit;
    }
    shared_memory_ptr = mmap(NULL,total_length,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);
    if(shared_memory_ptr==MAP_FAILED){
        goto sc_create_shared_mem_error_exit;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
            exit(1);
        }
    }
    if(kill(child_pid,0)<0){
        goto sc_create_shared_mem_error_exit;
    }
    atexit(sc_clean_shared_mem);

    return 0;
sc_create_shared_mem_error_exit:
    shared_memory_ptr=NULL;
    if(shared_memory_fd>0){
        shm_unlink(shared_memory_key);
        close(shared_memory_fd);
    }
    shared_memory_fd=-1;
    free(shared_memory_key);
    shared_memory_key=NULL;
    return 1;
}



static void sc_clean_shared_mem(void){
#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif
    kill(child_pid,SIGKILL);
    shared_memory_ptr=NULL;
    if(shared_memory_fd>0){
        shm_unlink(shared_memory_key);
        close(shared_memory_fd);
    }
    shared_memory_fd=-1;
    free(shared_memory_key);
    shared_memory_key=NULL;
    //return 0;
}


static int check_subprocess_dead(void){
    int count = 1;
    while(count<100000){
        count += 1;
    }
    return *(int*)shared_memory_ptr==-99;
}



int sc_socket(int domain,int type,int protocol){

    int func_ret=-1;
#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif

    INIT_OP(socket)

    *((int*)shared_memory_ptr+1) = 0;
    *((int*)shared_memory_ptr+2) = domain;
    *((int*)shared_memory_ptr+3) = type;
    *((int*)shared_memory_ptr+4) = protocol;
    *(int*)shared_memory_ptr = -1;
    //wait subprocess to finish operation

    SIMPLE_LOOP_OP(socket)

    func_ret=*((int*)shared_memory_ptr+1);
#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",func_ret);
#endif

sc_socket_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}


int sc_connect(int socket, const struct sockaddr *address, socklen_t address_len){

    int func_ret=-1;
#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif

    INIT_OP(connect)

    *((int*)shared_memory_ptr+1) = 1;
    *((int*)shared_memory_ptr+2) = socket;
    *(int*)shared_memory_ptr = -1;

    //wait subprocess to finish operation
    SIMPLE_LOOP_OP(connect)

    func_ret=*((int*)shared_memory_ptr+1);
#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",func_ret);
#endif

sc_connect_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}

ssize_t sc_send(int socket, const void *buffer, size_t length, int flags){
    int func_ret=-1;

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif

    INIT_OP(send)
// not implemented yet
#if PY_DEBUG
        fprintf(stderr,"the total length is %ld\n",total_length);
        fprintf(stderr,"the current length is %ld\n",length);
#endif
    if(length+sizeof(int)*3+sizeof(size_t)>total_length){

        return -1;
    }else{
        *((int*)shared_memory_ptr+1) = 2;
        *((int*)shared_memory_ptr+2) = socket;
        *((int*)shared_memory_ptr+3) = flags;
        *((size_t*)((int*)shared_memory_ptr+4)) = length;
        memcpy((void*)(((size_t*)((int*)shared_memory_ptr+4))+1),buffer,length);
        *(int*)shared_memory_ptr = -1;
        SIMPLE_LOOP_OP(send)
        func_ret=*((int*)shared_memory_ptr+1);
#if PY_DEBUG
        fprintf(stderr,"the ret value from the python script is %d\n",func_ret);
#endif
    }

sc_send_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}


ssize_t sc_recv(int socket, const void *buffer, size_t length, int flags){
    int func_ret=-1;

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif

    INIT_OP(recv)
// not implemented yet
#if PY_DEBUG
        fprintf(stderr,"the total length is %ld\n",total_length);
        fprintf(stderr,"the current length is %ld\n",length);
#endif
    if(length+sizeof(int)*3+sizeof(size_t)>total_length){
        return -1;
    }else{
        *((int*)shared_memory_ptr+1) = 3;
        *((int*)shared_memory_ptr+2) = socket;
        *((int*)shared_memory_ptr+3) = flags;
        *((size_t*)((int*)shared_memory_ptr+4)) = length;
        memcpy((void*)(((size_t*)((int*)shared_memory_ptr+4))+1),buffer,length);
        *(int*)shared_memory_ptr = -1;
        SIMPLE_LOOP_OP(recv)
        func_ret=*((int*)shared_memory_ptr+1);
#if PY_DEBUG
        fprintf(stderr,"the ret value from the python script is %d\n",func_ret);
#endif
        if(func_ret!=-1){
            memcpy(buffer,(void*)((int*)shared_memory_ptr+2),length);
        }
    }

sc_recv_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}

static int (*fp_close)(int fd);
int sc_close(int fd){
    int func_ret=-1;
#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif
    if(fd<10000){
        RESOLVE(close);
        func_ret=fp_close(fd);
    }else{
        INIT_OP(close)

        *((int*)shared_memory_ptr+1) = 4;
        *((int*)shared_memory_ptr+2) = fd;
        *(int*)shared_memory_ptr = -1;

        //wait subprocess to finish operation
        SIMPLE_LOOP_OP(close)

        func_ret=*((int*)shared_memory_ptr+1);
#if PY_DEBUG
        fprintf(stderr,"the ret value from the python script is %d\n",func_ret);
#endif
    }

sc_close_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}

static void (*fp_pthread_exit)(void* retval);
void sc_pthread_exit(void* retval){
#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif
    sc_clean_shared_mem();
    RESOLVE(pthread_exit);
    fp_pthread_exit(retval);
}



