#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif



#ifndef PHILOSOPHER_H
#define PHILOSOPHER_H

#include <semaphore.h>




void *dine(void *arg);


typedef struct philosopher {
    pthread_t thread;
    char *state;
    char fork_representation[NUM_PHILOSOPHERS + 1];
    int *left_fork;
    int *right_fork;
} philosopher;






#endif
