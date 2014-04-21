CFLAGS=`python-config --cflags`
#CFLAGS=-I/usr/include/python2.7 -I/usr/include/x86_64-linux-gnu/python2.7  -fno-strict-aliasing -D_FORTIFY_SOURCE=2 -g -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O2 -Wall -Wstrict-prototypes
LDFLAGS=`python-config --ldflags`
#LDFLAGS= -lutil -lm  -lpython2.7 -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions -lpthread -lrt -ldl

target = test libinde.so.1.0 test_inde
#target = ldi.so test libinde.so.1.0 test_inde

all:$(target)

ldi.so:rand-intercept.c
	gcc $< $(CFLAGS) -o $@ -util -lm -lpython2.7 -lpthread -lrt -ldl -rdynamic -shared -fpic 

libinde.so.1.0: inde.c
	gcc $(CFLAGS) $< -fPIC -shared -rdynamic -Wl,-soname,libinde.so.1 -o $@  $(LDFLAGS)

test:test.o

test_inde:test_inde.o
	gcc  $(CFLAGS) $< -o $@  -L. -Xlinker "-(" -linde -lpython2.7 -Xlinker "-)"
.PHONY: clean

clean:
	rm $(target)
	rm test.o
	rm test_inde
	rm lib*
