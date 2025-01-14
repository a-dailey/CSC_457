#include <stdio.h>
#include <stdlib.h>
#include "malloc.h"
#include "pp.h"

int main() {
    void *ptr1 = malloc(100);
    pp(stdout, "allocated 100 bytes at %p\n", ptr1);

    void *ptr2 = calloc(10, 20);
    pp(stdout, "allocated 10 * 20 bytes at %p\n", ptr2);

    ptr1 = realloc(ptr1, 200);
    pp(stdout, "reallocated 200 bytes at %p\n", ptr1);

    free(ptr1);
    pp(stdout, "freed ptr1\n");

    free(ptr2);
    pp(stdout, "freed ptr2\n");

    printf("done\n");

    return 0;
    
}