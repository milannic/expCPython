CFLAGS=`python-config --cflags`
LDFLAGS=`python-config --ldflags`
CC=gcc

target = libpctern.so.1.0 \

all:$(target)

libpctern.so.1.0:pctern.o
	$(CC) $< -Wl,-soname,libpctern.so.1 -rdynamic -shared -fpic -o $@ $(LDFLAGS) 

pctern.o:pctern.c
	$(CC) $(CFLAGS) -fpic -g -c -o $@ $<


.PHONY: clean link

link:
	ln -sf libpctern.so.1.0 libpctern.so.1
	ln -sf libpctern.so.1.0 libpctern.so

clean:
	rm -rf *.o
	rm -rf *.out
	rm -rf libpctern*
	rm -rf $(target)
