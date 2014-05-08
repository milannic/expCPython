#include <stdio.h>  
#include <stdlib.h>  
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "Python.h"  

#define STRINGFY(i) #i
#define DEFINE_MYSTRING_ARRAY(i) char* my_string[i]
#define ASSIGN_STRING(i) my_string[i]=(char*)malloc(sizeof(char)*100); \
                                      my_string[i]= "mystring"#i
#define GET_STRING(index) my_string[index]

pthread_mutex_t my_mutex;


pid_t gettid(void){
    return syscall(SYS_gettid);
}


void* test_python(void* args){
    int count=0;
    PyThreadState* t_t_state=NULL;
    PyThreadState* t_g_state=NULL;
    PyInterpreterState* t_i_state = (PyInterpreterState*)args;
    PyGILState_STATE gil_lock;
    pthread_mutex_lock(&my_mutex);
    printf("Is Python Initialized? %s\n",Py_IsInitialized()?"Yes":"No");
    if(!Py_IsInitialized()){
        printf("I will initialize it.\n");
        Py_Initialize();
        PyEval_InitThreads();
    }
    printf("the tid now is %d\n",(int)gettid());
    printf("Now I am trying to crash the python\n");

//    gil_lock=PyGILState_Ensure();
//    t_g_state = PyThreadState_Get();
//    printf("Now I am trying to crash the python\n");
//    t_t_state = Py_NewInterpreter();
//    printf("%p\n",t_t_state);
//    printf("Now I am trying to crash the python\n");
//    PyThreadState_Swap(t_g_state);
//    printf("Now I am trying to crash the python\n");
//    PyGILState_Release(gil_lock);

    pthread_mutex_unlock(&my_mutex);
    while(count<1000){
        gil_lock=PyGILState_Ensure();
        //PyThreadState_Swap(t_t_state);
        printf("the tid now is %d\n",(int)gettid());
        PyRun_SimpleString("print \"hahahaha\"");
        //PyThreadState_Swap(t_g_state);
        PyGILState_Release(gil_lock);
        count+=1;
    }
    //PyEval_SaveThread();
    return EXIT_SUCCESS;
}


int main(int argc, char** argv){
    DEFINE_MYSTRING_ARRAY(5);
    pthread_t test[5];
    int index;
    int s;
    void * retval;
    int max_threads=5;
    PyThreadState* global_thread_state;
    PyInterpreterState* global_inter_state;
    ASSIGN_STRING(0);
    ASSIGN_STRING(1);
    ASSIGN_STRING(2);
    ASSIGN_STRING(3);
    ASSIGN_STRING(4);

    Py_Initialize();
    PyEval_InitThreads();
    //global_thread_state = PythreadState_Get();
    global_inter_state = PyThreadState_Get();
    if(!global_inter_state){
        printf("haha");
    }
    PyEval_ReleaseLock();

    for ( index = 0; index < max_threads; index += 1 ) {
        s = pthread_create(&test[index],NULL,&test_python,(void*)global_inter_state);
    }

    for( index = 0; index < max_threads; index += 1 ) {
        s = pthread_join(test[index],&retval);
    }
    printf("the main tid is %d\n",(int)gettid());
    return EXIT_SUCCESS;
}     // 初始化Python  


