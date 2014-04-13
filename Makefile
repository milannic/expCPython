#PYTHONHEAD=/usr/include/python2.7
CFLAGS= -c `python-config --cflags`
LFLAGS= -L/usr/lib/python2.7/config -lpthread -ldl -lutil -lm -lpython2.7 -Xlinker -export-dynamic
CC=gcc

PROG=cpython.e

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $< -o $@

default: all

all: $(PROG)


cpython.e: cpython.o
	$(CC) $< $(LFLAGS)  -o $@

clean:
	$(RM) *.o
	$(RM) $(PROG)
	$(RM) *.pyc
	$(RM) *.e
