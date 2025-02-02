#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>

int thread_function(void *arg) {
    int *counter = (int *)arg;  // Access counter by reference

    if (*counter > 0) {
        printf("Counter is %d.  Exiting\n", *counter);

        // Decrement the counter
        (*counter)--;

        // Spawn a new thread
        lwp_create(thread_function, counter);
    }
    return 0;  // Exit the thread
}


int main() {
    int counter1 = 50;
    int counter2 = 50;

    // Create two initial threads
    lwp_create(thread_function, &counter1);
    lwp_create(thread_function, &counter2);

    // Start the lightweight process system
    lwp_start();

    printf("Bye\n");
    return 0;
}
