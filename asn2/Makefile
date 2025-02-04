CC = gcc
CFLAGS = -Wall -g -fpic
INCLUDE = -I ~pn-cs453/Given/Asgn2/include -I ~/csc453/asn2
LIBPATH = -L ~pn-cs453/Given/Asgn2/lib64 -L ~/csc453/asn2


TARGET_STATIC = liblwp.a 
TARGET_SHARED = liblwp.so

all: $(TARGET_STATIC) $(TARGET_SHARED)

$(TARGET_STATIC): lwp.o
	ar rcs $@ $^

$(TARGET_SHARED): lwp.o scheduler.o magic64.o
	$(CC) -shared -o liblwp.so lwp.o scheduler.o magic64.o

lwp.o: lwp.c lwp.h scheduler.h fp.h
	$(CC) $(CFLAGS) $(INCLUDE) -c lwp.c 

scheduler.o: scheduler.c scheduler.h lwp.h
	$(CC) $(CFLAGS) $(INCLUDE) -c scheduler.c

magic64.o: magic64.S
	$(CC) -Wall -c magic64.S

clean:
	rm -f *.o liblwp.so

