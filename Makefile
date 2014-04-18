#PYTHONHEAD=/usr/include/python2.7
CFLAGS= -c `python-config --cflags`
LFLAGS= `python-config --ldflags`
CC=gcc

PROG=cpython.e \
	 cpython_localtest.e \
	 cpython_0.0.1 \

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $< -o $@

default: all

all: $(PROG)


cpython_localtest.e: cpython_localtest.o
	$(CC) $< $(LFLAGS)  -o $@

cpython.e: cpython.o
	$(CC) $< $(LFLAGS)  -o $@

.PHONY:clean
clean:
	$(RM) *.o
	$(RM) $(PROG)
	$(RM) *.pyc
