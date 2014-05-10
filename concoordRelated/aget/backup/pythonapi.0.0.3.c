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


// this should be in a configuration file in the later time
static char* module_name = "sc_serverproxy";
static char* class_name = "SimpleConcoordServer";
static char* replica_group = "127.0.0.1:14001";

// with embedded python, we must carefully check every PyObject's reference to keep python runtime work correctly

// to maintain a python 
// each thread has its only copy, so we don't need to worry about the synchronous issues



#ifndef ALPHA_DICT
#define ALPHA_DICT 26
#endif

#ifndef SHARED_MEM_KEY_LEN
#define SHARED_MEM_KEY_LEN 20
#endif

static int count=100;
static pthread_mutex_t count_lock;

static int random_gen(char* buffer,int length){
    int index;
    int temp;
    pthread_mutex_lock(&count_lock);
    srand(time(NULL)+count);
    count+=100;
    for( index = 0; index < length-1; index += 1 ) {
        temp = rand()%ALPHA_DICT+97;
        buffer[index] = temp;
//        printf("the current alpha dict is %d\n",temp);
//        printf("the current alpha dict is %c\n",temp);
    }
    buffer[length-1] = '\0';
    pthread_mutex_unlock(&count_lock);
    return 0;

}


int sc_socket(int domain,int type,int protocol){
    pid_t child_pid;
    char *shared_memory_key=NULL;
    void* shared_memory_ptr=NULL;
    int shared_memory_fd=-1;
    int op_ret=-1;
    int child_ret=-1;
    int func_ret=-1;


#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
#endif

    shared_memory_key = (char*)malloc(sizeof(char)*SHARED_MEM_KEY_LEN);
    random_gen(shared_memory_key,SHARED_MEM_KEY_LEN);

#if PY_DEBUG
    fprintf(stderr,"my random string is %s\n",shared_memory_key);
#endif

    //getchar();
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
        goto sc_socket_exit;
    }

    op_ret = ftruncate(shared_memory_fd,sizeof(int)*3);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
        ;
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_socket_clean;
    }
    shared_memory_ptr = mmap(NULL,sizeof(int)*3,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);

    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = domain;
        *((int*)shared_memory_ptr+1) = type;
        *((int*)shared_memory_ptr+2) = protocol;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,"-t","0",NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
        }
    }else{
        //sleep(2);
        waitpid(child_pid,&child_ret,0);
        if(WIFEXITED(child_ret)){
            func_ret = *(int*)shared_memory_ptr;
        }else{
            errno = 3;
        }
    }

#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",*(int*)shared_memory_ptr);
#endif

sc_socket_clean:
    op_ret = shm_unlink(shared_memory_key);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"unlinking the shared memory succeed\n");
#endif
        close(shared_memory_fd);
    }else{
        fprintf(stderr,"unlinking the shared memory not succeed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        func_ret = op_ret;
    }
sc_socket_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}


int sc_connect(int socket, const struct sockaddr *address, socklen_t address_len){
    pid_t child_pid;
    char *shared_memory_key=NULL;
    void* shared_memory_ptr=NULL;
    int shared_memory_fd=-1;
    int op_ret=-1;
    int child_ret=-1;
    int func_ret=-1;
    //getchar();

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
    fprintf(stderr,"and the socket num is %d\n",socket);
#endif

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
        goto sc_connect_exit;
    }

    op_ret = ftruncate(shared_memory_fd,sizeof(int)*3);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
        ;
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_connect_clean;
    }
    shared_memory_ptr = mmap(NULL,sizeof(int)*3,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);

    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = socket;
        //*((int*)shared_memory_ptr+1) = type;
        //*((int*)shared_memory_ptr+2) = protocol;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,"-t","1",NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
        }
    }else{
        //sleep(2);
        waitpid(child_pid,&child_ret,0);
        if(WIFEXITED(child_ret)){
            func_ret = *(int*)shared_memory_ptr;
        }else{
            errno = 3;
        }
    }

#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",*(int*)shared_memory_ptr);
#endif

sc_connect_clean:
    op_ret = shm_unlink(shared_memory_key);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"unlinking the shared memory succeed\n");
#endif
        close(shared_memory_fd);
    }else{
        fprintf(stderr,"unlinking the shared memory not succeed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        func_ret = op_ret;
    }
sc_connect_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}




ssize_t sc_send(int socket, const void *buffer, size_t length, int flags){
    pid_t child_pid;
    char *shared_memory_key=NULL;
    void* shared_memory_ptr=NULL;
    int shared_memory_fd=-1;
    int op_ret=-1;
    int child_ret=-1;
    int func_ret=-1;
    int memory_length = 0;
    //getchar();

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
    fprintf(stderr,"and the socket num is %d\n",socket);
#endif

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
        goto sc_send_exit;
    }

    memory_length = sizeof(int)*2+sizeof(size_t)+length;
    op_ret = ftruncate(shared_memory_fd,memory_length);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
        ;
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_send_clean;
    }
    shared_memory_ptr = mmap(NULL,memory_length,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);

    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = socket;
        *((int*)shared_memory_ptr+1) = flags;
        *((size_t*)((int*)shared_memory_ptr+2)) = length;

        memcpy((void*)(((size_t*)((int*)shared_memory_ptr+2))+1),buffer,length);
        //*((int*)shared_memory_ptr+2) = protocol;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,"-t","2",NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
        }
    }else{
        //sleep(2);
        waitpid(child_pid,&child_ret,0);
        if(WIFEXITED(child_ret)){
            func_ret = *(int*)shared_memory_ptr;
        }else{
            errno = 3;
        }
    }

#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",*(int*)shared_memory_ptr);
#endif

sc_send_clean:
    op_ret = shm_unlink(shared_memory_key);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"unlinking the shared memory succeed\n");
#endif
        close(shared_memory_fd);
    }else{
        fprintf(stderr,"unlinking the shared memory not succeed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        func_ret = op_ret;
    }
sc_send_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}


ssize_t sc_recv(int socket, const void *buffer, size_t length, int flags){
    pid_t child_pid;
    char *shared_memory_key=NULL;
    void* shared_memory_ptr=NULL;
    int shared_memory_fd=-1;
    int op_ret=-1;
    int child_ret=-1;
    int func_ret=-1;
    int memory_length = 0;
    //getchar();

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
    fprintf(stderr,"and the socket num is %d\n",socket);
#endif

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
        goto sc_recv_exit;
    }

    memory_length = sizeof(int)*2+sizeof(size_t)+length;
    op_ret = ftruncate(shared_memory_fd,memory_length);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
        ;
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_recv_clean;
    }
    shared_memory_ptr = mmap(NULL,memory_length,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);

    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = socket;
        *((int*)shared_memory_ptr+1) = flags;
        *((size_t*)((int*)shared_memory_ptr+2)) = length;
        //*((int*)shared_memory_ptr+2) = protocol;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,"-t","3",NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
        }
    }else{
        //sleep(2);
        waitpid(child_pid,&child_ret,0);
        if(WIFEXITED(child_ret)){
            func_ret = *(int*)shared_memory_ptr;
        }else{
            errno = 3;
        }
    }
    if(func_ret!=-1){
        memcpy(buffer,(void*)((int*)shared_memory_ptr+1),length);
    }

#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",*(int*)shared_memory_ptr);
#endif

sc_recv_clean:
    op_ret = shm_unlink(shared_memory_key);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"unlinking the shared memory succeed\n");
#endif
        close(shared_memory_fd);
    }else{
        fprintf(stderr,"unlinking the shared memory not succeed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        func_ret = op_ret;
    }
sc_recv_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}


int sc_close(int fd){
    pid_t child_pid;
    char *shared_memory_key=NULL;
    void* shared_memory_ptr=NULL;
    int shared_memory_fd=-1;
    int op_ret=-1;
    int child_ret=-1;
    int func_ret=-1;
    int memory_length = 0;
    //getchar();

#if PY_DEBUG
    fprintf(stderr,"%s is called \n",__FUNCTION__);
    fprintf(stderr,"and the socket num is %d\n",fd);
#endif

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
        goto sc_close_exit;
    }

    memory_length = sizeof(int);
    op_ret = ftruncate(shared_memory_fd,memory_length);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"truncating the shared memory succeed\n");
#endif
        ;
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto sc_close_clean;
    }
    shared_memory_ptr = mmap(NULL,memory_length,PROT_WRITE|PROT_READ,MAP_SHARED,shared_memory_fd,0);

    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = fd;
        //*((int*)shared_memory_ptr+2) = protocol;
    }
    if(!(child_pid=fork())){
// child process
//      
        unsetenv("LD_PRELOAD");
        if(execl("./sm-proxy.py","./sm-proxy.py","-k",shared_memory_key,"-t","4",NULL)<0){
            fprintf(stderr,"error\n");
            fprintf(stderr,"error is %d\n",errno);
        }
    }else{
        //sleep(2);
        waitpid(child_pid,&child_ret,0);
        if(WIFEXITED(child_ret)){
            func_ret = *(int*)shared_memory_ptr;
        }else{
            errno = 3;
        }
    }
#if PY_DEBUG
    fprintf(stderr,"the ret value from the python script is %d\n",*(int*)shared_memory_ptr);
#endif

sc_close_clean:
    op_ret = shm_unlink(shared_memory_key);
    if(!op_ret){
#if PY_DEBUG
        fprintf(stderr,"unlinking the shared memory succeed\n");
#endif
        close(shared_memory_fd);
    }else{
        fprintf(stderr,"unlinking the shared memory not succeed\n");
        fprintf(stderr,"the error code is %d\n",errno);
        func_ret = op_ret;
    }
sc_close_exit:
    if(func_ret==-1){
        errno = 5;
    }
    return func_ret;
}
