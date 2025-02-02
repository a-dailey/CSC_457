#include "lwp.h"
#include <stdio.h>

int test16_function(void *arg) {
    int count = (int)(long)arg;
    printf("%d\n", count);
    if (count > 1) {
        lwp_create(test16_function, (void *)(long)(count - 1));
    }
    lwp_exit(0);
    return 0;  // Ensure it returns an int
}

int main() {
    // Test 16: Spawning a single thread that 
    //recursively creates another and exits
    lwp_create(test16_function, (void *)(long)100);
    lwp_start();
    printf("Bye\n");
    return 0;
}