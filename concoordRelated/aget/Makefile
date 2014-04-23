all:: install

install::
	gcc -g -rdynamic -Wall -shared -fpic -o rand-intercept.so rand-intercept.c -lpthread -lrt -ldl

.PHONY: clean
clean:
	rm rand-intercept.so
