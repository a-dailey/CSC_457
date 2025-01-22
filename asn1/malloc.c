/*Malloc library implementation for csc453 assignment 1*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
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
#define DEBUG_MALLOC getenv("DEBUG_MALLOC")
#define STD_CHUNK_SIZE 64 * 1024
#define ALIGNMENT 16

/// GLOBALS ///
static Block *head = NULL;

/// FUNCTION PROTOTYPES ///
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void align_heap();
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

        if (DEBUG_MALLOC){
            pp(stderr, "Initialized heap\n");
        }
            
    }

    align_heap(); // Align the heap to 16 bytes

    Block *current = head;
    Block *prev = NULL;

    while (current) {
        if (current->free && current->size >= aligned_size) {
            current->free = 0; //Mark as allocated

            //If there's enough space, split the block
            if (current->size >= aligned_size + BLOCK_SIZE + ALIGNMENT) {
                Block *new_block = 
                (Block *)((uintptr_t)current + BLOCK_SIZE + aligned_size);
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

            if (DEBUG_MALLOC){
                pp(stderr, "MALLOC: malloc(%d) => (ptr=%p, size=%d)\n", size, 
                (void *)((uintptr_t)current + BLOCK_SIZE), current->size);
            }
            return (void *)((uintptr_t)current + BLOCK_SIZE);
        }

        prev = current;
        current = current->next;
    }

    // No suitable block found, request more memory
    size_t new_size = STD_CHUNK_SIZE;
    if (aligned_size > new_size) {
        new_size = aligned_size;
    }
    void *mem = request_mem(new_size);
    if (mem == NULL) {
        return NULL;
    }
    //pp(stderr, "Got memory\n");

    Block *new_block = (Block *)mem;
    new_block->next = NULL;
    new_block->prev = prev;
    new_block->size = new_size - BLOCK_SIZE;
    new_block->free = 1;

    if (prev) {
        prev->next = new_block;
    } else {
        head = new_block;
    }

    if (prev && prev->free) {
        // Merge with previous block if free
        prev->size += new_block->size + BLOCK_SIZE;
        prev->next = new_block->next;
        if (new_block->next) {
            new_block->next->prev = prev;
        }
        new_block = prev;
    }

    return malloc(size); // Retry malloc with new memory added
}

void align_heap() {
    //get current break
    uintptr_t current_break = (uintptr_t) sbrk(0);
    if (current_break == (uintptr_t)-1) {
        errno = ENOMEM;
        pp(stderr, "Failed to align memory, sbrk failed\n");
        return;
    }
    if (current_break % ALIGNMENT != 0) {
        //alignment is off, fix it
        //calculate how much to align by
        size_t align_size = ALIGNMENT - (current_break % ALIGNMENT);
        //request memory for align block
        void *align_block = sbrk(align_size);
        if (align_block == (void *)-1) {
            errno = ENOMEM;
            pp(stderr, "Failed to align memory, sbrk failed\n");
            return;
        }

        //add align block to end of blocks
        if (head == NULL) {
            //if no blocks yet, this align block is first
            head = (Block *)align_block;
            head->next = NULL;
            head->prev = NULL;
            head->size = align_size - BLOCK_SIZE;
            head->free = 1;
        } 
        
        else {
            //if blocks exist, add align block to the end
            Block *current = head;
            //get to last block
            while (current->next) {
                current = current->next; 
            }
            //if last block is free, align block can be added to it
            if (current->free) {
                current->size += align_size;
            } 
            else {
            //if last block is not free, add align block as new block
            current->next = (Block *)align_block;
            current->next->prev = current;
            current->next->next = NULL;
            current->next->size = align_size - BLOCK_SIZE;
            current->next->free = 1;
            }
        }
    }        
    return;
}

void free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    Block *current = head;

    //handle pointers in the middle of a block
    while (current) {
        if ((uintptr_t) ptr >= (uintptr_t) current + BLOCK_SIZE && 
        (uintptr_t) ptr < (uintptr_t) current + BLOCK_SIZE + current->size) {
            //ptr is in this section, so free it
            ptr = (void *)((uintptr_t) current + BLOCK_SIZE);
            break;
        }
        current = current->next;
    }

    // Find block to free
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

    if (DEBUG_MALLOC){
        pp(stderr, "MALLOC: free(%p)\n", ptr);
    }
}

void *calloc(size_t nmemb, size_t size) {
    //check for invalid input
    if (nmemb == 0 || size == 0) {
        return NULL;
    }

    //stop multiplication overflow
    if (size != 0 && nmemb > SIZE_MAX / size) {
        return NULL;
    }

    //allocate memory
    size_t total_size = nmemb * size;
    void *mem = malloc(total_size);
    if (mem == NULL) {
        return NULL;
    }

    //zero out memory
    memset(mem, 0, total_size);

    if (DEBUG_MALLOC){
        pp(stderr, "MALLOC: calloc(%d, %d) => (ptr=%p, size=%d)\n", 
        nmemb, size, mem, total_size);
    }
    return mem;
}

void *realloc(void *ptr, size_t size) {
    //if ptr is NULL, realloc is the same as malloc
    if (ptr == NULL) {
        return malloc(size);
    }

    //if size is 0, realloc is the same as free
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    Block *current = head;

    //handle pointers in the middle of a block
    //iterate through all blocks, find block ptr is from, use start of block
    while (current) {
        if ((uintptr_t) ptr >= (uintptr_t) current + BLOCK_SIZE && 
        (uintptr_t) ptr < (uintptr_t) current + BLOCK_SIZE + current->size) {
            //ptr is in this section, so realloc this section
            ptr = (void *)((uintptr_t) current + BLOCK_SIZE);
            break;
        }
        current = current->next;
    }

    //check if current block already has enough space
    Block *block = (Block *)((uintptr_t)ptr - BLOCK_SIZE);
    if (block->size >= size) {
        return ptr;
    }

    //check if we can expand in place forwards
    if (block->next != NULL && block->next->free && 
    block->size + block->next->size + BLOCK_SIZE >= size) {
        //enough space in next block to expand
        block->size += block->next->size + BLOCK_SIZE;
        if (block->next->next != NULL) {
            block->next->next->prev = block;
        }
        block->next = block->next->next;
        
        return ptr;
    }

    //check if we can expand in place backwards
    if (block->prev != NULL && block->prev->free && 
    block->prev->size + block->size + BLOCK_SIZE >= size) {
        //enough space in previous block to expand
        block->prev->size += block->size + BLOCK_SIZE;
        block->prev->next = block->next;
        block->next->prev = block->prev;
        return (void *)((uintptr_t)block->prev + BLOCK_SIZE);
    }

    //tried to extend in place, couldnt so just malloc
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    //copy data from old pointer to new pointer
    memcpy(new_ptr, ptr, block->size);
    //free old pointer
    free(ptr);

    if (DEBUG_MALLOC){
        pp(stderr, "MALLOC: realloc(%p, %d) => (ptr=%p, size=%d)\n", 
        ptr, size, new_ptr, size);
    }
    return new_ptr;
}

Block *init_heap() {
    //get current break
    uintptr_t current_break = (uintptr_t) sbrk(0);
    if (current_break == (uintptr_t)-1) {
        errno = ENOMEM;
        pp(stderr, "Failed to align memory, sbrk failed\n");
        return NULL;
    }
    //check if heap is aligned
    if (current_break % ALIGNMENT != 0) {
        //not aligned, move break to next multiple of 16
        void *temp = sbrk(ALIGNMENT - current_break % ALIGNMENT);
        if (temp == (void *)-1) {
            errno = ENOMEM;
            pp(stderr, "Failed to align memory, sbrk failed\n");
            return NULL;
        }
    }

    //request memory for first block
    void *mem = request_mem(STD_CHUNK_SIZE);
    if (mem == NULL) {
        return NULL;
    }

    //put first block at start of heap
    Block *block = (Block *)mem;
    block->next = NULL;
    block->prev = NULL;
    block->size = STD_CHUNK_SIZE - BLOCK_SIZE;
    block->free = 1;

    return block;
}

void *request_mem(size_t size) {
    void *mem = sbrk(size);
    if (mem == (void *)-1) {
        //sbrk failed
        errno = ENOMEM;
        pp(stderr, "Failed to request memory, sbrk failed\n");
        return NULL;
    }
    return mem;
}
