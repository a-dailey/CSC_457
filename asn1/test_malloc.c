#include <stdio.h>
#include <stdlib.h>
#include "malloc.h"

int main() {
    void *ptr1 = malloc(100);
    pp("allocated 100 bytes at %p\n, ptr1");

    void *ptr2 = calloc(10, 20);
    pp("allocated 10 * 20 bytes at %p\n", ptr2);

    ptr1 = realloc(ptr1, 200);
    pp("reallocated 200 bytes at %p\n", ptr1);

    free(ptr1);
    pp("freed ptr1\n");

    free(ptr2);
    pp("freed ptr2\n");

    return 0;
    
}