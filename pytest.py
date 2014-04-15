#!/usr/bin/env python
def add(a,b):  
    print "in python function add"  
    print "a = " + str(a)  
    print "b = " + str(b)  
    print "ret = " + str(a+b)  
    return a + b  

class MyTest():
    def __init__(self):
        self.num1 = 1
        self.num2 = 2
    def myAdd(self):
        return self.num1+self.num2
