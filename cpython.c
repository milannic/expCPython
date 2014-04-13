#include <stdio.h>  
#include <stdlib.h>  
#include "Python.h"  
  
int main(int argc, char** argv)     // 初始化Python  
{  
  
    char * string;  
    PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pRetVal;  
  
    Py_Initialize();       
    if ( !Py_IsInitialized() )           
        return -1;  
  
    // 载入名为pytest的脚本(注意：不是pytest.py)  
    pName = PyString_FromString("pytest");  
    pModule = PyImport_Import(pName);  
    if ( !pModule )   
    {  
        printf("can't find pytest.py");  
        getchar();  
        return -1;  
    }  
    pDict = PyModule_GetDict(pModule);  
    if ( !pDict )           
    {  
        return -1;  
    }  
  
    // 找出函数名为add的函数  
    pFunc = PyDict_GetItemString(pDict, "add");  
    if ( !pFunc || !PyCallable_Check(pFunc) )           
    {  
        printf("can't find function [add]");  
        getchar();  
        return -1;  
    }  
  
    // 参数进栈  
    pArgs = PyTuple_New(2);  
  
    // PyObject* Py_BuildValue(char *format, ...)  
    // 把C++的变量转换成一个Python对象。当需要从  
    // C++传递变量到Python时，就会使用这个函数。此函数  
    // 有点类似C的printf，但格式不同。常用的格式有  
    // s 表示字符串，  
    // i 表示整型变量，  
    // f 表示浮点数，  
    // O 表示一个Python对象。b=f(a) 0.1-0.01=0.09  
  
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("i",3));   
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("i",4));   
  
    // 调用Python函数  
    pRetVal = PyObject_CallObject(pFunc, pArgs);  
    printf("function return value : %d\r\n", PyInt_AsLong(pRetVal));  
  
    Py_DECREF(pName);  
    Py_DECREF(pArgs);  
    Py_DECREF(pModule);  
    Py_DECREF(pRetVal);  
  
    // 关闭Python  
    Py_Finalize();  
    return 0;  
}  
