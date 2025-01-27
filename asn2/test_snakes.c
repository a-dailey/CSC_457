#include <stdio.h>
#include "lwp.h"
#include "snakes.h" // Include snake-related functions
#include "scheduler.h"

// Wrapper function to run a snake as a thread
int snake_thread(void *arg) {
    snake *s = (snake *)arg;

    // Run the snake (this function is defined in libsnakes.a)
    run_snake(s);

    // Free the snake when done
    free_snake(*s);
    return 0; // Return success
}

int main() {
    // Initialize the snake display
    if (!start_windowing()) {
        fprintf(stderr, "Failed to start snake display\n");
        return 1;
    }

    printf("Starting Snake Program Test\n");

    // Set the scheduler to RoundRobin
    lwp_set_scheduler(RoundRobin);

    // Create snakes
    snake s1 = new_snake(5, 10, 5, E, 1);  // Snake at (5,10), length 5, direction E, color 1
    snake s2 = new_snake(10, 15, 7, W, 2); // Snake at (10,15), length 7, direction W, color 2
    snake s3 = new_snake(15, 20, 10, N, 3); // Snake at (15,20), length 10, direction N, color 3

    // Create threads for snakes
    lwp_create(snake_thread, &s1);
    lwp_create(snake_thread, &s2);
    lwp_create(snake_thread, &s3);

    // Start the lightweight process system
    lwp_start();

    // Wait for all threads to finish
    int status;
    while (lwp_wait(&status) != NO_THREAD) {
        printf("A snake exited with status: %d\n", status);
    }

    // End the snake display
    end_windowing();

    printf("All snakes have exited\n");
    return 0;
}
