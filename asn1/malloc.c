#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "pp.h"

/// STRUCTS ///
typedef struct Block {
    struct Block *next;
    struct Block *prev;
    size_t size;
    uint8_t free;
} Block;

/// DEFINES ///
#define BLOCK_SIZE sizeof(Block)

/// GLOBALS ///
static Block *head = NULL;

/// FUNCTION PROTOTYPES ///
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
Block *init_heap();
void *request_mem(size_t size);

/// FUNCTIONS ///
void *malloc(size_t size) {
    size_t aligned_size = (size + 15) & ~15; // Align size to 16 bytes

    if (head == NULL) {
        // Initialize the heap if it doesn't exist
        head = init_heap();
        if (head == NULL) {
            return NULL;
        }
    }

    Block *current = head;
    Block *prev = NULL;

    while (current) {
        if (current->free && current->size >= aligned_size) {
            current->free = 0; // Mark block as allocated

            // If there's enough space, split the block
            if (current->size >= aligned_size + BLOCK_SIZE + 16) {
                Block *new_block = 
                (Block *)((uintptr_t)current+BLOCK_SIZE+aligned_size);
                new_block->size = current->size - aligned_size - BLOCK_SIZE;
                new_block->free = 1;
                new_block->next = current->next;
                new_block->prev = current;

                if (current->next) {
                    current->next->prev = new_block;
                }

                current->size = aligned_size;
                current->next = new_block;
            }

            pp(stderr, "MALLOC: malloc(%zu) => (ptr=%p, size=%zu)\n", size, 
            (void *)((uintptr_t)current + BLOCK_SIZE), current->size);
            return (void *)((uintptr_t)current + BLOCK_SIZE);
        }

        prev = current;
        current = current->next;
    }

    // No suitable block found, request more memory
    void *mem = request_mem(64 * 1024);
    if (mem == NULL) {
        return NULL;
    }

    Block *new_block = (Block *)mem;
    new_block->next = NULL;
    new_block->prev = prev;
    new_block->size = 64 * 1024 - BLOCK_SIZE;
    new_block->free = 1;

    if (prev) {
        prev->next = new_block;
    } else {
        head = new_block;
    }

    return malloc(size); // Retry malloc with new memory added
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    Block *block = (Block *)((uintptr_t)ptr - BLOCK_SIZE);
    block->free = 1;

    // Merge with next block if free
    if (block->next && block->next->free) {
        block->size += block->next->size + BLOCK_SIZE;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    // Merge with previous block if free
    if (block->prev && block->prev->free) {
        block->prev->size += block->size + BLOCK_SIZE;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }

    pp(stderr, "FREE: free(%p)\n", ptr);
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    // Prevent multiplication overflow
    if (size != 0 && nmemb > SIZE_MAX / size) {
        return NULL;
    }

    size_t total_size = nmemb * size;
    void *mem = malloc(total_size);
    if (mem == NULL) {
        return NULL;
    }

    memset(mem, 0, total_size);
    pp(stderr, "CALLOC: calloc(%zu, %zu) => (ptr=%p, size=%zu)\n", 
    nmemb, size, mem, total_size);
    return mem;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    Block *block = (Block *)((uintptr_t)ptr - BLOCK_SIZE);
    if (block->size >= size) {
        return ptr;
    }

    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    free(ptr);

    pp(stderr, "REALLOC: realloc(%p, %zu) => (ptr=%p, size=%zu)\n", 
    ptr, size, new_ptr, size);
    return new_ptr;
}

Block *init_heap() {
    void *mem = request_mem(64 * 1024);
    if (mem == NULL) {
        return NULL;
    }

    Block *block = (Block *)mem;
    block->next = NULL;
    block->prev = NULL;
    block->size = 64 * 1024 - BLOCK_SIZE;
    block->free = 1;

    return block;
}

void *request_mem(size_t size) {
    void *mem = sbrk(size);
    if (mem == (void *)-1) {
        return NULL;
    }
    return mem;
}
