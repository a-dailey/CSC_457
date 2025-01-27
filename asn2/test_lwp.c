#include <stdio.h>
#include "lwp.h"
#include "scheduler.h"

// Simple thread function
int thread_function(void *arg) {
    int id = lwp_gettid();
    printf("Thread %d started with arg: %s\n", id, (char *)arg);
    int i;
    for (i = 0; i < 5; i++) {
        printf("Thread %d executing iteration %d\n", id, i + 1);
        lwp_yield(); // Yield to other threads
    }
    printf("Thread %d exiting\n", id);
    return id * 10; // Return a unique value
}

int main() {
    printf("Starting LWP test program\n");

    // Set up a simple round-robin scheduler
    // scheduler RoundRobin = lwp_get_scheduler();
    // if (!RoundRobin) {
    //     fprintf(stderr, "Failed to retrieve the default scheduler\n");
    //     return 1;
    // }
    lwp_set_scheduler(RoundRobin);

    // Create threads
    lwp_create(thread_function, "Thread 1");
    lwp_create(thread_function, "Thread 2");
    lwp_create(thread_function, "Thread 3");

    // Start the LWP system
    lwp_start();

    // Wait for all threads to finish
    int status;
    while (lwp_wait(&status) != NO_THREAD) {
        printf("Thread exited with status: %d\n", status);
    }

    printf("All threads have exited\n");
    return 0;
}
