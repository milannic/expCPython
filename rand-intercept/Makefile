CFLAGS=`python-config --cflags`
LDFLAGS=`python-config --ldflags`

all:: install test

.PHONY:install
install:
	gcc -g -fpic -rdynamic -shared $(CFLAGS) $(LDFLAGS) -lrt -o ldi.so rand-intercept.c 

test:test.o
