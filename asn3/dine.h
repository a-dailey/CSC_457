#ifndef NUM_PHILOSOPHERS
#define NUM_PHILOSOPHERS 5
#endif



#ifndef PHILOSOPHER_H
#define PHILOSOPHER_H

#include <semaphore.h>




void *dine(void *arg);

void update_fork_representation(int philosopher_num);

void print_status();

void dawdle();

void print_header();

void update_philosopher(int philosopher_num, char *state, 
int left_fork, int right_fork);

typedef struct philosopher {
    pthread_t thread;
    char *state;
    char fork_representation[NUM_PHILOSOPHERS + 1];
    int left_fork;
    int right_fork;
} philosopher;






#endif
