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

#define PROJECT_TAG "XTERN"
#define RESOLVE(x)	if (!fp_##x && !(fp_##x = dlsym(RTLD_NEXT, #x))) { fprintf(stderr, #x"() not found!\n"); exit(-1); }
#define DO_LOGGING 1 // If it is 1, enable logging (logging sync ops, and updating times).
#define MAXSIZE 1024


// Per thread variables.
static int num_threads = 0;
static int turn = 0;
static __thread int self_turn = -1;
static __thread int self_tid = -1;
static __thread struct timespec my_time;
static __thread int entered_sys = 0; // Avoid recursively enter backtrace(), since backtrace() will call pthread_mutex_lock().
static __thread FILE *log = NULL;
static pthread_mutex_t lock;


//if(self_tid==0){fprintf(stderr,"%d here OPERATION start\n",self_tid);fflush(stderr);fprintf(stderr,"There is %d threads in total\n",num_threads);} 
#define OPERATION_START \
  initTid(); \
  void *eip; \
  struct timespec app_time; \
  if (DO_LOGGING) { \
    updateTurn(); \
    update_time(&app_time); \
  }
  //fprintf(stderr, "START: %s: %s(): tid %d\n", PROJECT_TAG, __FUNCTION__, self());


#define OPERATION_END \
  struct timespec syscall_time; \
  if (DO_LOGGING && !entered_sys) { \
    entered_sys = 1; \
    update_time(&syscall_time); \
    eip = get_eip(); \
    logOp(__FUNCTION__, eip, self_tid, self_turn, &app_time, &syscall_time); \
    entered_sys = 0; \
  }

//if(self_tid==0){fprintf(stderr,"I am here OPERATION END\n");fflush(stderr);}

void update_time(struct timespec *ret);
int internal_mutex_lock(pthread_mutex_t *mutex);
int internal_mutex_unlock(pthread_mutex_t *mutex);

int self() {
  assert(self_tid >= 0);
  return self_tid;
}

void initTid() {
  //fprintf(stderr,"%d here OPERATION start\n",self_tid);
  if (self_tid == -1) {
    internal_mutex_lock(&lock);
    self_tid = num_threads;
    num_threads++;
    if (num_threads == 1) // To match the idle thread in xtern.
      num_threads++;
    if (self_tid == 0) // Init the out directory using the main thread.
      mkdir("./out", 0777);
    internal_mutex_unlock(&lock);
    struct timespec tmp;
    update_time(&tmp); // Init my_time.
  }
}

void updateTurn() {
  internal_mutex_lock(&lock);
  self_turn = turn;
  turn++;
  internal_mutex_unlock(&lock);
}

void time_diff(const struct timespec *start, const struct timespec *end, struct timespec *diff)
{
  if ((end->tv_nsec-start->tv_nsec) < 0) {
    diff->tv_sec = end->tv_sec - start->tv_sec - 1;
    diff->tv_nsec = 1000000000 - start->tv_nsec + end->tv_nsec;
  } else {
    diff->tv_sec = end->tv_sec - start->tv_sec;
    diff->tv_nsec = end->tv_nsec - start->tv_nsec;
  }
}

void update_time(struct timespec *ret)
{
  struct timespec start_time;
  clock_gettime(CLOCK_REALTIME , &start_time);
  time_diff(&my_time, &start_time, ret);
  my_time.tv_sec = start_time.tv_sec;
  my_time.tv_nsec = start_time.tv_nsec;
}

void *get_eip()
{
  const int SIZE = 5;
  void *tracePtrs[SIZE];
  //if(self_tid==0)
  //fprintf(stderr,"I am here get eip start \n");
  //fflush(stderr);
  int ret = backtrace(tracePtrs, SIZE);
  assert(ret >= 0);
//  if(self_tid==0)
//  fprintf(stderr,"I am here get eip end\n");
//  fflush(stderr);
  //fprintf(stderr,"I am %d\n",self_tid);
#if 0
  /* For most of applications, returning the [2] is fine, which shows the locations calling sync op.
  But for some applications that use sync wrapper (e.g., __db_pthread_mutex_lock in bdb), we can manually
  modify this to return tracePtrs[3], which could help us easily identify the location calling the sync wrappers. */
  if (tracePtrs[3] == 0x4336db) {
    fprintf(stderr, "\n6db: Tid %d: EIP: %p <- %p <- %p <- %p <- %p : <- %p <- %p <- %p <- %p <- %p <- %p\n\n",
      self(),
      tracePtrs[3], tracePtrs[4], tracePtrs[5], tracePtrs[6], tracePtrs[7],
      tracePtrs[8], tracePtrs[9], tracePtrs[10], tracePtrs[11], tracePtrs[12],
      tracePtrs[13]);
  }
  if (tracePtrs[3] == 0x004e1d46) {
    fprintf(stderr, "\nd46: Tid %d: EIP: %p <- %p <- %p <- %p <- %p : <- %p <- %p <- %p <- %p <- %p : <- %p <- %p <- %p <- %p <- %p\n\n",
      self(),
      tracePtrs[3], tracePtrs[4], tracePtrs[5], tracePtrs[6], tracePtrs[7],
      tracePtrs[8], tracePtrs[9], tracePtrs[10], tracePtrs[11], tracePtrs[12],
      tracePtrs[13],  tracePtrs[14],  tracePtrs[15],  tracePtrs[16],  tracePtrs[17]);
  }
  return tracePtrs[3];
#endif
  return tracePtrs[2];  //  this is ret_eip of my caller
}

void logOp(const char *op, void *eip, int tid, int self_turn, struct timespec *app_time, struct timespec *syscall_time) {
  if (!log) {
    mkdir("./out", 0777);
    char buf[1024] = {0};
    sprintf(buf, "./out/tid-%d-%d.txt", getpid(), self());
    log = fopen(buf, "w");
    assert(log);
  }
  fprintf(log, "%s %p %d %f %f 0.0 %d n/a\n", op, eip, self_turn,
    (double)app_time->tv_sec + ((double)app_time->tv_nsec)/1000000000.0,
     (double)syscall_time->tv_sec + ((double)syscall_time->tv_nsec)/1000000000.0,
    tid);
  fflush(log);
}

static int (*fp_pthread_mutex_lock)(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_lock\n");
      fflush(stderr);
  }
  OPERATION_START;
  //fprintf(stderr, "tid %d before mutex lock %p\n", self(), (void *)mutex);fflush(stderr);
  RESOLVE(pthread_mutex_lock);
  int ret = fp_pthread_mutex_lock(mutex);
  int cur_errno = errno;
  OPERATION_END;
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_lock end\n");
      fflush(stderr);
  }
  //fprintf(stderr, "Self %u, tid %d after mutex locked %p, eip %p\n\n", (unsigned)pthread_self(), self(), (void *)mutex, eip);fflush(stderr);
  errno = cur_errno;
  return ret;
}

int internal_mutex_lock(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here internal_mutex_lock\n");
      fflush(stderr);
  }
  RESOLVE(pthread_mutex_lock);
  int ret = fp_pthread_mutex_lock(mutex);
  return ret;
}

static int (*fp_pthread_mutex_unlock)(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_unlock\n");
      fflush(stderr);
  }
  OPERATION_START;
  //fprintf(stderr, "tid %d before mutex un lock %p\n", self(), (void *)mutex);fflush(stderr);
  RESOLVE(pthread_mutex_unlock);
  int ret = fp_pthread_mutex_unlock(mutex);
  int cur_errno = errno;
  OPERATION_END;
  //fprintf(stderr, "tid %d after mutex un locked %p, eip %p\n\n", self(), (void *)mutex, eip);fflush(stderr);
  errno = cur_errno;
  return ret;
}

int internal_mutex_unlock(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here internal_mutex_unlock\n");
      fflush(stderr);
  }
  RESOLVE(pthread_mutex_unlock);
  int ret = fp_pthread_mutex_unlock(mutex);
  if(self_tid==-2){
      fprintf(stderr, "I am here internal_mutex_unlock END\n");
      fflush(stderr);
  }
  return ret;
}

static int (*fp_pthread_mutex_trylock)(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_trylock\n");
      fflush(stderr);
  }
  OPERATION_START;
  RESOLVE(pthread_mutex_trylock);
  int ret = fp_pthread_mutex_trylock(mutex);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_mutex_init)(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_init\n");
      fflush(stderr);
  }
  OPERATION_START;
  RESOLVE(pthread_mutex_init);
  //fprintf(stderr, "before mutex init %p, %p\n", (void *)mutex, (void *)attr);
  int ret = fp_pthread_mutex_init(mutex, attr);
  int cur_errno = errno;
  OPERATION_END;
  //fprintf(stderr, "after mutex init %p, %p, eip %p\n\n", (void *)mutex, (void *)attr, eip);
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_mutex_destroy)(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_destroy\n");
      fflush(stderr);
  }
  OPERATION_START;
  RESOLVE(pthread_mutex_destroy);
  int ret = fp_pthread_mutex_destroy(mutex);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;  
}

static int (*fp_pthread_create)(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
  if(self_tid==-2){
      fprintf(stderr, "I am here pthread_mutex_create\n");
      fflush(stderr);
  }
  OPERATION_START;
  RESOLVE(pthread_create);
  int ret = fp_pthread_create(thread, attr, start_routine, arg);
  int cur_errno = errno;
  OPERATION_END;
  //usleep(1); // Sleep for a while in order to make the child thread run first, this is to make thread id deterministic. Not a guarantee (and not need to).
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_join)(pthread_t thread, void **retval);
int pthread_join(pthread_t thread, void **retval) {
  OPERATION_START;
  RESOLVE(pthread_join);
  int ret = fp_pthread_join(thread, retval);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static void (*fp_pthread_exit)(void *retval);
void pthread_exit(void *retval) {
  //fprintf(stderr, "thread %d am here pthread_exit\n",self_tid);
  //fflush(stderr);
  if (entered_sys == 0) { // This is a little bit tricky, if not do this, deadlock, since pthread_exit() will call pthread_mutex_lock...
    OPERATION_START;
    RESOLVE(pthread_exit);
    OPERATION_END;
    entered_sys = 1;
  }
  fp_pthread_exit(retval);
}
static int (*fp_pthread_cancel)(pthread_t thread);
int pthread_cancel(pthread_t thread){
  //fprintf(stderr, "thread %d am here pthread_cancel\n",self_tid);
  //fflush(stderr);
  int ret=0;
  if (entered_sys == 0) { // This is a little bit tricky, if not do this, deadlock, since pthread_exit() will call pthread_mutex_lock...
    OPERATION_START;
    RESOLVE(pthread_cancel);
    OPERATION_END;
    entered_sys = 1;
  }
  ret=fp_pthread_cancel(thread);
  return ret;
};
/*
static int (*fp_pthread_cond_init)(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  OPERATION_START;
  fprintf(stderr, "before cond init %p, %p\n", (void *)cond, (void *)attr);
  RESOLVE(pthread_cond_init);
  int ret = fp_pthread_cond_init(cond, attr);
  int cur_errno = errno;
  OPERATION_END;
  fprintf(stderr, "after cond init %p, %p, eip %p, ret %d\n", (void *)cond, (void *)attr, eip, ret);
  if (ret == EAGAIN)
    fprintf(stderr, "\n\nEAGAIN\n\n");
  if (ret == ENOMEM)
    fprintf(stderr, "\n\nENOMEM\n\n");
  if (ret == EBUSY)
    fprintf(stderr, "\n\nEBUSY\n\n");
  if (ret == EINVAL)
    fprintf(stderr, "\n\nEINVAL\n\n");
  errno = cur_errno;
  return ret;
}
*/
static int (*fp_pthread_cond_wait)(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
  OPERATION_START;
  //fprintf(stderr, "tid %d before cond wait: cond %p, mutex %p\n", self(), (void *)cond, (void *)mutex);fflush(stderr);
  RESOLVE(pthread_cond_wait);
  int ret = fp_pthread_cond_wait(cond, mutex);
  int cur_errno = errno;
  OPERATION_END;
  //fprintf(stderr, "tid %d after cond wait: cond %p, mutex %p, eip %p\n\n", self(), (void *)cond, (void *)mutex, eip);fflush(stderr);
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_cond_destroy)(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond) {
  OPERATION_START;
  RESOLVE(pthread_cond_destroy);
  int ret = fp_pthread_cond_destroy(cond);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;  
}

static int (*fp_pthread_cond_timedwait)(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
  OPERATION_START;
  RESOLVE(pthread_cond_timedwait);
  int ret = fp_pthread_cond_timedwait(cond, mutex, abstime);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_cond_signal)(pthread_cond_t *cond);
int pthread_cond_signal(pthread_cond_t *cond) {
  OPERATION_START;
  RESOLVE(pthread_cond_signal);
  int ret = fp_pthread_cond_signal(cond);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_cond_broadcast)(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond) {
  OPERATION_START;
  RESOLVE(pthread_cond_broadcast);
  int ret = fp_pthread_cond_broadcast(cond);
  int cur_errno = errno; OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_trywrlock);
  int ret = fp_pthread_rwlock_trywrlock(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_wrlock);
  int ret = fp_pthread_rwlock_wrlock(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_rdlock);
  int ret = fp_pthread_rwlock_rdlock(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_tryrdlock);
  int ret = fp_pthread_rwlock_tryrdlock(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_unlock)(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_unlock);
  int ret = fp_pthread_rwlock_unlock(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_destroy)(pthread_rwlock_t *rwlock);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_destroy);
  int ret = fp_pthread_rwlock_destroy(rwlock);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_init)(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_init(pthread_rwlock_t * rwlock, const pthread_rwlockattr_t * attr) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_init);
  int ret = fp_pthread_rwlock_init(rwlock, attr);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_timedwrlock)(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_timedwrlock);
  int ret = fp_pthread_rwlock_timedwrlock(rwlock, abs_timeout);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_pthread_rwlock_timedrdlock)(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, const struct timespec *abs_timeout) {
  OPERATION_START;
  RESOLVE(pthread_rwlock_timedrdlock);
  int ret = fp_pthread_rwlock_timedrdlock(rwlock, abs_timeout);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}


static int (*fp_socket)(int domain, int type, int protocol);
int socket(int domain, int type, int protocol){
  fprintf(stderr,"domain : %d , type : %d , protocol : %d \n",domain,type,protocol);
  OPERATION_START;
  RESOLVE(socket);
  int ret = fp_socket(domain,type,protocol);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_close)(int fd);
int close(int fd) {
#if  0     /* ----- backtrace info ----- */
	void* high_address = 0x7f0000000000;
	void *EIP[MAXSIZE],*eip; 
	int backtrace_size,index=0,intercept_flag=0;
	char** backtrace_symbols_strs = NULL;	

	fprintf(stderr,"we are calling close\n");
	backtrace_size = backtrace(EIP,MAXSIZE);
	backtrace_symbols_strs = backtrace_symbols(EIP,backtrace_size);

	for ( index = 0; index < backtrace_size; index += 1 ) {
		fprintf(stderr,"%s\n",backtrace_symbols_strs[index]);
	}
	fprintf(stderr,"------------------------------\n");  
	free(backtrace_symbols_strs);
	eip = get_eip();
	fprintf(stderr,"current EIP is %p\n",eip);

	if(eip>high_address){
		intercept_flag = 0;
		fprintf(stderr,"this is a shared library calling\n");
	}else{
		fprintf(stderr,"we should intercept this function\n");
		intercept_flag = 1;
	}
#endif     /* ----- #if 0 : If0Label_1 ----- */


  if(self_tid==-2){
      fprintf(stderr,"I am here close start\n"); 
      fprintf(stderr, "current file descriptor is %d\n",fd);
      fflush(stderr);
  }
  int ret=0;
  int cur_errno=0;

  OPERATION_START;
  RESOLVE(close);
  ret = fp_close(fd);

  if(self_tid==-2){
      fprintf(stderr,"I am here close start end\n"); 
      fflush(stderr);
  }
  OPERATION_END;
  cur_errno = errno;
  errno = cur_errno;
  return ret;
}


static ssize_t (*fp_send)(int socket, const void *buffer, size_t length, int flags);

ssize_t send(int socket, const void *buffer, size_t length, int flags){
    
  fprintf(stderr,"this is thread %d\n",self_tid);
  fprintf(stderr,"socket : %d , content : %s , length : %d \n",socket,(char*)buffer,(int)length);
  OPERATION_START;
  RESOLVE(send);
  ssize_t ret = fp_send(socket, buffer, length, flags);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
};

//ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

//ssize_t sendmsg(int socket, const struct msghdr *message, int flags);

static int (*fp_recv)(int socket, void *buffer, size_t length, int flags);
ssize_t recv(int socket, void *buffer, size_t length, int flags) {
  OPERATION_START;
  RESOLVE(recv);
  int ret = fp_recv(socket, buffer, length, flags);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

static int (*fp_connect)(int socket, const struct sockaddr *address, socklen_t address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len){
  struct sockaddr_in* in_address=address;
  fprintf(stderr,"socket : %d , port : %u , addr : %u , socklen : %d \n",socket,in_address->sin_port,in_address->sin_addr.s_addr,(int)address_len);
    
  OPERATION_START;
  RESOLVE(connect);
  int ret = fp_connect(socket,address,address_len);
  int cur_errno = errno;
  OPERATION_END;
  errno = cur_errno;
  return ret;
}

//ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len);

//ssize_t recvmsg(int socket, struct msghdr *message, int flags);

//int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout);

//ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset); 

//ssize_t read(int fildes, void *buf, size_t nbyte);

//ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset); 

//ssize_t write(int fildes, const void *buf, size_t nbyte);


//int accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);

//int bind(int socket, const struct sockaddr *address, socklen_t address_len);

//int listen(int socket, int backlog);

//int shutdown(int socket, int how);

