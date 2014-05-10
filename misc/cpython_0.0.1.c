// C代码调，用上面的add函数  
#include <stdio.h>  
#include <stdlib.h>  
#include "Python.h"  
  
int main(int argc, char** argv)     // 初始化Python  
{  
    //在使用Python系统前，必须使用Py_Initialize对其  
    //进行初始化。它会载入Python的内建模块并添加系统路     //是否初始化成功需要使用Py_IsInitialized。  
  
    char * string;  
    PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pRetVal;  
    PyObject *pClass,*pIns,*pRetVal2,*pRetVal3;  
  
    Py_Initialize();       
    if ( !Py_IsInitialized() )           
        return -1;  
  
    // 载入名为pytest的脚本(注意：不是pytest.py)  
    pName = PyString_FromString("counter");  
    pModule = PyImport_Import(pName);  
    if ( !pModule )   
    {  
        printf("can't find counter.py");  
        getchar();  
        return -1;  
    }  
    pDict = PyModule_GetDict(pModule);  
    if ( !pDict )           
    {  
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

	if(pIns){
		pRetVal=PyObject_CallMethod(pIns,"getvalue",NULL);
	}

	if(pRetVal){
		printf("we call the getvalue method, and the return value is %ld\n",PyInt_AsLong(pRetVal));
	}

	//Py_DECREF(pRetVal);

	pRetVal2=PyObject_CallMethod(pIns,"increment",NULL);
	if(pRetVal2){
		printf("we call the increment method, and the return value is %ld\n",PyInt_AsLong(pRetVal));
		//Py_DECREF(pRetVal);
	}
//
//	sleep(2);
	pRetVal3=PyObject_CallMethod(pIns,"getvalue",NULL);

	if(pRetVal3){
		printf("we call the getvalue method, and the return value is %ld\n",PyInt_AsLong(pRetVal3));
	}
//
//	if(pRetVal==NULL){
//		printf("why I am null\n");
//	}
//	if(!pDict){
//		printf("now pDict is empty\n");
//		if(pDict==NULL){
//			printf("and pDict is set to NULL \n");
//		}
//	}
//	pDict = NULL;
//    pDict = PyModule_GetDict(pIns);  
//	if(!pDict){
//		printf("we cannot get what in the Instance\n");  
//	}
//  
//    // 参数进栈  
//    pArgs = PyTuple_New(2);  
//  
//    // PyObject* Py_BuildValue(char *format, ...)  
//    // 把C++的变量转换成一个Python对象。当需要从  
//    // C++传递变量到Python时，就会使用这个函数。此函数  
//    // 有点类似C的printf，但格式不同。常用的格式有  
//    // s 表示字符串，  
//    // i 表示整型变量，  
//    // f 表示浮点数，  
//    // O 表示一个Python对象。b=f(a) 0.1-0.01=0.09  
//  
//    PyTuple_SetItem(pArgs, 0, Py_BuildValue("i",3));   
//    PyTuple_SetItem(pArgs, 1, Py_BuildValue("i",4));   
//  
//    // 调用Python函数  
//    pRetVal = PyObject_CallObject(pFunc, pArgs);  
//    printf("function return value : %d\r\n", PyInt_AsLong(pRetVal));  
  
    Py_DECREF(pName);  
    Py_DECREF(pModule);  
	Py_DECREF(pIns);
	Py_DECREF(pClass);
    Py_DECREF(pArgs);  
	//if(pRetVal!=NULL)
	Py_DECREF(pRetVal);  
	Py_DECREF(pRetVal2);  
	Py_DECREF(pRetVal3);  
  
    // 关闭Python  
    Py_Finalize();  
    return 0;  
}  
