#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h> // For size_t
#include <stdint.h> // For uint8_t

/// STRUCTS ///
typedef struct Block {
    struct Block *next;
    struct Block *prev;
    size_t size;
    uint8_t free;
} Block;

/// DEFINES ///
#define BLOCK_SIZE sizeof(Block)

/// PROTOTYPES ///
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

Block *init_heap();
void *request_mem(size_t size);

#endif // MALLOC_H
