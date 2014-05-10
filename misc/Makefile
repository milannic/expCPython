#PYTHONHEAD=/usr/include/python2.7
CFLAGS= -c `python-config --cflags`
LFLAGS= `python-config --ldflags`
CC=gcc

PROG=cpython.out \
	 cpython_localtest.out \
	 cpython_0.0.1.out \

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $< -o $@

default: all

all: $(PROG)


cpython_localtest.out: cpython_localtest.o
	$(CC) $< $(LFLAGS)  -o $@

cpython_0.0.1.out: cpython_0.0.1.o
	$(CC) $< $(LFLAGS)  -o $@

cpython.out: cpython.o
	$(CC) $< $(LFLAGS)  -o $@

.PHONY:clean
clean:
	$(RM) *.o
	$(RM) $(PROG)
	$(RM) *.pyc
