#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

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
    for( index = 0; index < length-1; index += 1 ) {
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
    int socket_args[3]={2,1,0};
    int ret=-1;

    my_string = (char*)malloc(sizeof(char)*20);
    random_gen(my_string,20);
    printf("my random string is %s\n",my_string);
    fp = fopen("hehe","w+");
    if(fp==NULL){
        printf("error is %d\n",errno);
        goto failed_exit;
    }
    fwrite((void*)socket_args,sizeof(int),3,fp);
    fflush(fp);
    //fclose(fp);
    my_pid = fork();
    if(!my_pid){
        if(execl("./py-proxy.py","./py-proxy.py","-f","hehe","-t","0",NULL)<0){
            printf("error\n");
            printf("error is %d\n",errno);
        }
    }else{
        sleep(3);
        waitpid(my_pid,NULL,0);
    }
    //fp = fopen("hehe","r");
    fseek(fp,0,SEEK_SET);
    fread(&ret,sizeof(int),1,fp);
    printf("the return value is %d\n",ret);
    if(ret>0){
        return EXIT_SUCCESS;
    }else{
failed_exit:
        return EXIT_FAILURE;
    }
}				/* ----------  end of function main  ---------- */
