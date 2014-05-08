#PYTHONHEAD=/usr/include/python2.7
CFLAGS= -c `python-config --cflags` -pthread
LDFLAGS= -lpthread `python-config --ldflags` -lrt
CC=gcc

SOURCE=$(shell find -name "*.c" -exec basename \{} \;)
PROG=$(SOURCE:.c=.out)
OBJ=$(SOURCE:.c=.o)

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $< -o $@

default: all

all: $(PROG) $(OBJ)

%.out:%.o
	$(CC) -o $@ $< $(LDFLAGS)   


.PHONY:clean test run blockrun
clean:
	$(RM) -rf $(OBJ)
	$(RM) -rf $(PROG)
	$(RM) -rf *.pyc
test:
	@echo $(PROG)
	@echo $(SOURCE)
	@echo $(OBJ)
run:
	@for i in $(PROG);do \
		./$$i > ./out_$${i}; \
	done \

blockrun:
	@for i in $(PROG);do \
		./$$i ; \
	done \
