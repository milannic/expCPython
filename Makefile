#CFLAGS=`python-config --cflags`
LDFLAGS=`python-config --ldflags` -lrt
#LDFLAGS= -lrt
CC=gcc
LIB_NAME="./libpctern.so"

target = libpctern.so.1.0 \

all:$(target)

libpctern.so.1.0:pctern.o enter_sys.o pythonapi.o
	$(CC) $^ -Wl,-soname,libpctern.so.1 -rdynamic -shared -fpic -o $@ $(LDFLAGS) 

.c.o:
	$(CC) $(CFLAGS) -fpic -g -c -o $@ $^


.PHONY: clean link run

link:
	ln -sf libpctern.so.1.0 libpctern.so.1
	ln -sf libpctern.so.1.0 libpctern.so

clean:
	rm -rf *.o
	rm -rf *.out
	rm -rf libpctern*
	rm -rf $(target)
	rm -rf *.log
run:
	LD_PRELOAD=$(LIB_NAME) ./aget -f -n1 -p 8080 http://localhost/test.py &
	sleep 5
	LD_PRELOAD=$(LIB_NAME) ./aget -f -n1 -p 8080 http://localhost/haha.py &
