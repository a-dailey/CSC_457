#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "malloc.h" // Include your custom malloc header

void test_malloc() {
    printf("Testing malloc...\n");
    int *arr = (int *)malloc(10 * sizeof(int)); 
    // Allocate memory for 10 integers
    if (!arr) {
        fprintf(stderr, "malloc failed!\n");
        return;
    }

    int i;
    // Write to the allocated memory
    for (i = 0; i < 10; i++) {
        arr[i] = i * i;
    }

    // Read and print the values
    printf("Data stored in malloc'ed array:\n");
    for (i = 0; i < 10; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    free(arr); // Free the allocated memory
    printf("malloc test completed.\n\n");
}

void test_calloc() {
    printf("Testing calloc...\n");
    int *arr = (int *)calloc(10, sizeof(int)); 
    // Allocate zero-initialized memory
    if (!arr) {
        fprintf(stderr, "calloc failed!\n");
        return;
    }

    // Verify zero initialization
    printf("Data stored in calloc'ed array (should be all zeros):\n");
    int i;
    for (i = 0; i < 10; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    free(arr); // Free the allocated memory
    printf("calloc test completed.\n\n");
}

void test_realloc() {
    printf("Testing realloc...\n");
    int *arr = (int *)malloc(5 * sizeof(int)); 
    // Allocate memory for 5 integers
    if (!arr) {
        fprintf(stderr, "malloc failed!\n");
        return;
    }

    // Write to the allocated memory
    int i;
    for (i = 0; i < 5; i++) {
        arr[i] = i + 1;
    }

    // Resize the memory to hold 10 integers
    arr = (int *)realloc(arr, 10 * sizeof(int));
    if (!arr) {
        fprintf(stderr, "realloc failed!\n");
        return;
    }

    // Write additional values
    for (i = 5; i < 10; i++) {
        arr[i] = i + 1;
    }

    // Print all the values
    printf("Data stored in realloc'ed array:\n");
    for (i = 0; i < 10; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    free(arr); // Free the allocated memory
    printf("realloc test completed.\n\n");
}

void test_stress() {
    printf("Testing stress with multiple allocations...\n");
    void *blocks[50];

    // Allocate multiple small blocks
    int i;
    for (i = 0; i < 50; i++) {
        blocks[i] = malloc(100); // Allocate 100 bytes
        if (!blocks[i]) {
            fprintf(stderr, "malloc failed for block %d!\n", i);
            return;
        }
        memset(blocks[i], i, 100); // Fill with a pattern
    }

    // Free every other block to simulate fragmentation
    for (i = 0; i < 50; i += 2) {
        free(blocks[i]);
    }

    // Allocate more memory to test fragmentation handling
    for (i = 0; i < 25; i++) {
        void *ptr = malloc(200); // Allocate 200 bytes
        if (!ptr) {
            fprintf(stderr, "malloc failed during fragmentation test!\n");
            return;
        }
        free(ptr);
    }

    // Free remaining blocks
    for (i = 1; i < 50; i += 2) {
        free(blocks[i]);
    }

    void *ptr = malloc(2^24); // Allocate a large block
    if (!ptr) {
        fprintf(stderr, "malloc failed for large block!\n");
        return;
    }

    printf("Stress test completed.\n\n");
}

int main() {
    printf("Starting tests for custom malloc implementation...\n\n");

    test_malloc();   // Test basic malloc functionality
    test_calloc();   // Test calloc and zero initialization
    test_realloc();  // Test realloc and resizing memory
    test_stress();   // Stress test with fragmentation

    printf("All tests completed successfully.\n");
    return 0;
}
