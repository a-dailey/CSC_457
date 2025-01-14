#include <stdlib.h>
#include <string.h>
#include "malloc.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "pp.h"

// /// STRUCTS ///
// typedef struct Block {
//     struct Block *next;
//     struct Block *prev;
//     size_t size;
//     uint8_t free;
// } Block;

/// DEFINES ///
#define BLOCK_SIZE sizeof(Block)


/// GLOBALS ///
static Block *head = NULL;



/// FUNCTIONS ///
void *malloc(size_t size) {
    size_t aligned_size = (size + 15) & ~15;

    if (head == NULL) {
        //initialize heap if doesnt exist
        head = init_heap();
        if (head == NULL) {
            return NULL;
        }
    }
    
    

    Block *current = head;
    Block *prev = NULL;

    while (current) {
        if (current->free && current->size >= (aligned_size + BLOCK_SIZE)) {
            //found a block that will work
            current->free = 0; //block no longer free

            //add a new block the the requested size away from the current block
            //this new block marks the end of the current block, which is now of the requested size
            Block *new_block = (Block *)((uintptr_t)current + aligned_size + BLOCK_SIZE);
            new_block->size = current->size - aligned_size - BLOCK_SIZE; //new block size is the remaining size of the current block
            new_block->free = 1;
            new_block->next = current->next;
            new_block->prev = current;

            current->size = aligned_size;
            current->next = new_block;


            pp(stderr, "MALLOC: malloc(%d) => (ptr=%p, size=%d)\n", size, (void*)((uintptr_t)current + BLOCK_SIZE), current->size);

            return (void*)((uintptr_t)current + BLOCK_SIZE);


        }
        prev = current;
        current = current->next;
    }

    //no big enough block found, request more memory
    void *mem = request_mem(64 * 1024);
    if (mem == NULL) {
        return NULL;
    }
    Block *new_block = (Block *)mem;
    new_block->next = NULL;
    new_block->size = 64 * 1024 - BLOCK_SIZE;
    new_block->free = 1;

    if (prev) {
        prev->next = new_block;
    } else {
        head = new_block;
    }

    pp(stderr, "MALLOC: malloc(%d) => (ptr=%p, size=%d)\n", size, (void*)((uintptr_t)new_block + BLOCK_SIZE), new_block->size);
    return (void *) ((uintptr_t)new_block + BLOCK_SIZE);

}

void free(void *ptr) {

    if (ptr == NULL) {
        return;
    }
    
    Block *block = (Block *)((uintptr_t)ptr - BLOCK_SIZE);
    if (block->prev != NULL && block->prev->free && block->next != NULL && block->next->free) {
        //both surrounding blocks are free, merge with them
        //prev gets size of next two blocks and two block sizes
        block->prev->size += block->size + block-> next->size + 2 * BLOCK_SIZE;
        block->prev->next = block->next->next;
        if (block->next->next != NULL) {
            block->next->next->prev = block->prev;
        }
    }
    else if (block->prev != NULL && block->prev->free) {
        //previous block is free, merge with it
        //prev gets size of current block and block size
        block->prev->size += block->size + BLOCK_SIZE;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
        block = block->prev;

    }
    else if (block->next != NULL && block->next->free) {
        //next block is free, merge with it
        //current block gets size of next block and block size
        block->size += block->next->size + BLOCK_SIZE;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    block->free = 1;
    pp(stderr, "FREE: free(%p)\n", ptr);
    return;
}

void *calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    if (head == NULL) {
        head = init_heap();
        if (head == NULL) {
            return NULL;
        }
    }

    size_t total_size = nmemb * size;
    void *mem = malloc(total_size);
    if (mem == NULL) {
        return NULL;
    }
    memset(mem, 0, total_size);
    return mem;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    //regular pointer is at end of last block, subtract block size to get block
    Block *block = (Block *)((uintptr_t)ptr - BLOCK_SIZE);

    if (block->size >= size) {
        return ptr;
    }
    free(ptr);
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    memcpy(new_ptr, ptr, block->size);
    return new_ptr;


}

Block *init_heap()
{
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

//get mem from os
void *request_mem(size_t size) {
    void *mem = sbrk(size);
    if (mem == (void *)-1) {
        return NULL;
    }
    return mem;
}