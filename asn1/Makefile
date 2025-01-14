# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -fpic
INCLUDE = -I.
LIBPATH = -L.
LIBS = -lpp

# Target names
TARGET_STATIC = libmalloc.a
TARGET_SHARED = libmalloc.so
TEST_EXECUTABLE = test_malloc

# Default target: builds both static and shared libraries
all: $(TARGET_STATIC) $(TARGET_SHARED)

# Static library
$(TARGET_STATIC): malloc.o
	ar rcs $@ $^

# Shared library
$(TARGET_SHARED): malloc.o
	$(CC) -shared -o $@ $^ $(LIBPATH) $(LIBS)

# Object file
malloc.o: malloc.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

# Create 32-bit and 64-bit versions of libmalloc.so
intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib64/libmalloc.so: lib64 malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o

lib:
	mkdir -p lib

lib64:
	mkdir -p lib64

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

# Test program
test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): test_malloc.c $(TARGET_STATIC)
	$(CC) -o $@ test_malloc.c $(LIBPATH) -lmalloc $(LIBS)

# Clean up build files
clean:
	rm -f *.o $(TARGET_STATIC) $(TARGET_SHARED) $(TEST_EXECUTABLE)
	rm -rf lib lib64
