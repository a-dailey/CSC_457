# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -fPIC
INCLUDE = -I.
LIBPATH = -L.

# Target names
TARGET_STATIC = libmalloc.a
TARGET_SHARED = libmalloc.so

# Default target: builds both libraries
all: malloc

# Build static and shared libraries
malloc: $(TARGET_STATIC) $(TARGET_SHARED)

# Static library
$(TARGET_STATIC): malloc.o
	ar rcs $@ $^

# Shared library
$(TARGET_SHARED): malloc.o
	$(CC) -shared -o $@ $^ $(LIBPATH)

# Object file
malloc.o: malloc.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

# Create 32-bit and 64-bit versions of libmalloc.so
intel-all: lib/libmalloc.so lib64/libmalloc.so

lib/libmalloc.so: malloc.c
	mkdir -p lib
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc.c

lib64/libmalloc.so: malloc.c
	mkdir -p lib64
	$(CC) $(CFLAGS) -m64 -shared -o $@ malloc.c

# Clean up build files
clean:
	rm -f *.o $(TARGET_STATIC) $(TARGET_SHARED)
	rm -rf lib lib64

