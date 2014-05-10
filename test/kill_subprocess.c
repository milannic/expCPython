#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */ #define ALPHA_DICT 26

int random_gen(char* buffer,int length){
    int index;
    int temp;
    srand(time(NULL));
    buffer[0]='/';
    for( index = 1; index < length-1; index += 1 ) {
        temp = rand()%ALPHA_DICT+97;
        buffer[index] = temp;
//        printf("the current alpha dict is %d\n",temp);
//        printf("the current alpha dict is %c\n",temp);
    }
    buffer[length-1] = '\0';
    return 0;

}


    int
main ( int argc, char *argv[] )
{
    pid_t my_pid;
    char *my_string;
    int size;
    FILE* fp=NULL;
    int sfd=-1;
    int socket_args[3]={2,1,0};
    int ret=-1;
    void* shared_memory_ptr=NULL;
    int* temp_ptr=NULL;
    int a;

    my_string = (char*)malloc(sizeof(char)*20);
    random_gen(my_string,20);
    printf("my random string is %s\n",my_string);
    sfd = shm_open(my_string,O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH);
    if(sfd!=-1){
        fprintf(stderr,"the shared memory descriptor is %d\n",sfd);
        fprintf(stderr,"creating the shared memory succeed\n");
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto error_exit;
    }
    ret = ftruncate(sfd,sizeof(int));
    if(!ret){
        fprintf(stderr,"truncating the shared memory succeed\n");
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto error_exit;
    }
    shared_memory_ptr = mmap(NULL,sizeof(int)*2,PROT_WRITE|PROT_READ,MAP_SHARED,sfd,0);
    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = 2;
        *((int*)shared_memory_ptr+1) = 1;
        *((int*)shared_memory_ptr+2) = 0;
    }
    
    for ( a = 0; a < 1000; a += 1 ) {
        printf("%d\n",a);
        *((int*)shared_memory_ptr+a) = a;
    }
    ret = ftruncate(sfd,sizeof(int)*4096);
    if(!ret){
        fprintf(stderr,"truncating the shared memory succeed\n");
    }else{
        //handle error
        fprintf(stderr,"the error code is %d\n",errno);
        goto error_exit;
    }
    if(shared_memory_ptr!=MAP_FAILED){
        *((int*)shared_memory_ptr) = 3;
        *((int*)shared_memory_ptr+1) = 2;
        *((int*)shared_memory_ptr+2) = 1;
    }
    shared_memory_ptr = mmap(NULL,sizeof(int)*2,PROT_WRITE|PROT_READ,MAP_SHARED,sfd,0);
    printf("the %d parameter is %d %d %d \n",3,*((int*)shared_memory_ptr),*((int*)shared_memory_ptr+1),*((int*)shared_memory_ptr+2));
    printf("then I will unlink the shared memory\n");
    if(shm_unlink(my_string)){
        printf("unlink error with %d",errno);
    }
    printf("the %d parameter is %d %d %d \n",3,*((int*)shared_memory_ptr),*((int*)shared_memory_ptr+1),*((int*)shared_memory_ptr+2));
    getchar();
    my_pid=fork();
    if(!my_pid){
// child process
        if(execl("./should-be-killed.py","./should-be-killed.py",NULL)<0){
            printf("error\n");
            printf("error is %d\n",errno);
        }
    }else{
        sleep(5);
        if(kill(my_pid,SIGKILL)){
            printf("error with %d\n",errno);
        }
    }
    return EXIT_SUCCESS;
error_exit:
    return EXIT_FAILURE;
}				/* ----------  end of function main  ---------- */
