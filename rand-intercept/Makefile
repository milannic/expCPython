CFLAGS=`python-config --cflags`
LDFLAGS=`python-config --ldflags`

target = libconcoord.so.1.0 \
		 libinde.so.1.0\
		 libinde.so \
		 test_inde.out \
		 test.out \



all:$(target)

libconcoord.so.1.0:rand-intercept.c
	gcc $< $(CFLAGS) -Wl,-soname,libconcoord.so.1 -rdynamic -shared -fpic -o $@ $(LDFLAGS) 

libinde.so.1.0: inde.c
	gcc $(CFLAGS)  -fPIC -shared -rdynamic -Wl,-soname,libinde.so.1 -o $@  $(LDFLAGS) $<

test.out:test.o
	gcc -Wall -g -rdynamic -o $@ $<

test_inde.out:test_inde.o libinde.so
	gcc  $(CFLAGS) $< -o $@  -L. -Xlinker "-(" -linde -lpython2.7 -Xlinker "-)"
.PHONY: clean link

libinde.so: libinde.so.1.0
	ln -sf libinde.so.1.0 libinde.so.1
	ln -sf libinde.so.1.0 libinde.so

clean:
	rm $(target)
	rm *.o
	rm *.out
	rm lib*
