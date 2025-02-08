#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "dine.h"


#define SHARED_SEM 1
#define SEM_START_VAL 1

#define CHANGE_STR "       "

#define DEFAULT_CYCLES 10  

static int cycles;

static sem_t forks[NUM_PHILOSOPHERS];

static sem_t print_status_lock;


static sem_t update_phil_lock;

philosopher *philosophers[NUM_PHILOSOPHERS];

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s <number of eat-think cycles>\n", argv[0]);
        return 1;
    }
    else if (argc == 2) {
        cycles = atoi(argv[1]);
    }
    else {
        cycles = DEFAULT_CYCLES;
    }

    // Initialize philosopher data
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i] = malloc(sizeof(philosopher));
        if (philosophers[i] == NULL) {
            perror("Error: malloc() failed");
            return 1;
        }
        philosophers[i]->state = CHANGE_STR;
        memset(philosophers[i]->fork_representation, '-', NUM_PHILOSOPHERS);
        philosophers[i]->fork_representation[NUM_PHILOSOPHERS] = '\0';
        philosophers[i]->left_fork = 0;
        philosophers[i]->right_fork = 0;
    }

    // Initialize semaphores
    if (sem_init(&update_phil_lock, SHARED_SEM, SEM_START_VAL) == -1) {
        perror("Error: sem_init(update_phil_lock) failed");
        return 1;
    }
    if (sem_init(&print_status_lock, SHARED_SEM, SEM_START_VAL) == -1) {
        perror("Error: sem_init(print_status_lock) failed");
        return 1;
    }
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&forks[i], SHARED_SEM, SEM_START_VAL) == -1) {
            perror("Error: sem_init(forks[i]) failed");
            return 1;
        }
    }

    // Print header
    print_header();

    // Create threads
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        int *arg = malloc(sizeof(int));
        if (arg == NULL) {
            perror("Error: malloc() failed");
            return 1;
        }
        *arg = i;
        if (pthread_create(&philosophers[i]->thread, NULL, dine, arg) != 0) {
            perror("Error: pthread_create() failed");
            return 1;
        }
    }

    // Join threads
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_join(philosophers[i]->thread, NULL) != 0) {
            perror("Error: pthread_join() failed");
            return 1;
        }
    }

    // Destroy semaphores
    if (sem_destroy(&update_phil_lock) == -1) {
        perror("Error: sem_destroy(update_phil_lock) failed");
        return 1;
    }
    if (sem_destroy(&print_status_lock) == -1) {
        perror("Error: sem_destroy(print_status_lock) failed");
        return 1;
    }
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_destroy(&forks[i]) == -1) {
            perror("Error: sem_destroy(forks[i]) failed");
            return 1;
        }
    } 

    // Free memory
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        free(philosophers[i]);
    }

    return 0;
}


void *dine(void *arg) {

    //philosopher number
    int philosopher_num = *(int *)arg;
    free(arg);  // Free the allocated memory for the argument

    //philosopher *philosopher = philosophers[philosopher_num];

    //left and right forks
    int left_fork = philosopher_num;
    int right_fork = (philosopher_num + 1) % NUM_PHILOSOPHERS;

    //fork representations


    //eat-think cycles
    int i;  
    for (i = 0; i < cycles; i++) {
        //deciding to try and eat
        update_philosopher(philosopher_num, CHANGE_STR, 0, 0);

        //if even philosopher
        if (philosopher_num % 2 == 0) {
            //even philosophers pick up right fork first
            if (sem_wait(&forks[right_fork]) == -1) {
                perror("Error: sem_wait(forks[right_fork]) failed");
                return NULL;
            }
            update_philosopher(philosopher_num, CHANGE_STR, 0, 1);

            //pick up left fork
            if (sem_wait(&forks[left_fork]) == -1) {
                perror("Error: sem_wait(forks[left_fork]) failed");
                return NULL;
            }
            update_philosopher(philosopher_num, CHANGE_STR, 1, 1);
            //print_status();
            //has both forks, is eating
            update_philosopher(philosopher_num, "Eat    ", 1, 1);
            //print_status();
            dawdle();
            //put down forks
            update_philosopher(philosopher_num, CHANGE_STR, 1, 1);
            sem_post(&forks[right_fork]);
            update_philosopher(philosopher_num, CHANGE_STR, 0, 1);
            //print_status();  
            sem_post(&forks[left_fork]);
            update_philosopher(philosopher_num, CHANGE_STR, 0, 0);
            //print_status();
            update_philosopher(philosopher_num, "Think  ", 0, 0);
            dawdle();
            

        }
        //if odd philosopher
        else {
            //odd philosophers pick up left fork first
            if (sem_wait(&forks[left_fork]) == -1) {
                perror("Error: sem_wait(forks[left_fork]) failed");
                return NULL;
            }
            update_philosopher(philosopher_num, CHANGE_STR, 1, 0);

            //pick up right fork
            if (sem_wait(&forks[right_fork]) == -1) {
                perror("Error: sem_wait(forks[right_fork]) failed");
                return NULL;
            }
            update_philosopher(philosopher_num, CHANGE_STR, 1, 1);
            //has both forks, is eating
            update_philosopher(philosopher_num, "Eat    ", 1, 1);
            //print_status();
            dawdle();
            update_philosopher(philosopher_num, CHANGE_STR, 1, 1);
            //put down forks
            sem_post(&forks[left_fork]);
            update_philosopher(philosopher_num, CHANGE_STR, 0, 1);
            //print_status();
            sem_post(&forks[right_fork]);
            update_philosopher(philosopher_num, CHANGE_STR, 0, 0);
            //print_status();
            update_philosopher(philosopher_num, "Think  ", 0, 0);
            //print_status();
            dawdle();
        }
    }
    return NULL;
}



void dawdle() {
    /*
    * sleep for a random amount of time between 0 and 999
    * milliseconds. This routine is somewhat unreliable, since it
    * doesn’t take into account the possiblity that the nanosleep
    * could be interrupted for some legitimate reason.
    *
    * nanosleep() is part of the realtime library and must be linked
    * with –lrt
    */
    struct timespec tv;
    int msec = (int)(((double)random() / RAND_MAX) * 1000);
    tv.tv_sec = 0;
    tv.tv_nsec = 1000000 * msec;
    if (-1 == nanosleep(&tv,NULL) ) {
    perror("nanosleep");
    }
}

void update_philosopher(int philosopher_num, 
char *state, int left_fork, int right_fork) {
    sem_wait(&update_phil_lock);

    //When picking up the left fork (fork i),
    //clear the left philosopher’s right fork.
    if (left_fork == 1) {
        int leftNeighbor = (philosopher_num + NUM_PHILOSOPHERS - 1) 
        % NUM_PHILOSOPHERS;
        philosophers[leftNeighbor]->right_fork = 0;
        philosophers[leftNeighbor]->state = CHANGE_STR;
    }

    //When picking up the right fork (fork (i+1)), 
    //clear the right philosopher’s left fork.
    if (right_fork == 1) {
        int rightNeighbor = (philosopher_num + 1) % NUM_PHILOSOPHERS;
        philosophers[rightNeighbor]->left_fork = 0;
        philosophers[rightNeighbor]->state = CHANGE_STR;
    }

    // Now update this philosopher's state and fork flags.
    philosophers[philosopher_num]->state = state;
    philosophers[philosopher_num]->left_fork = left_fork;
    philosophers[philosopher_num]->right_fork = right_fork;

    print_status();
    sem_post(&update_phil_lock);
}

void print_status(int calling_philosopher) {
    sem_wait(&print_status_lock);
    for ( int i = 0; i < NUM_PHILOSOPHERS; i++) {
        update_fork_representation(i);
    }
    int i;
    for (i=0; i < NUM_PHILOSOPHERS; i++) {
        printf("| %s %s ", philosophers[i]->fork_representation, 
        philosophers[i]->state);
    }
    //printf("| Called by %d ", calling_philosopher);
    printf("|\n");
    
    sem_post(&print_status_lock);
}

void update_fork_representation(int philosopher_num) {

    int left_fork_idx = philosopher_num;
    int right_fork_idx = (philosopher_num + 1) % NUM_PHILOSOPHERS;

    //If holding left fork
    if ((philosophers[philosopher_num]->left_fork)) {
        philosophers[philosopher_num]->fork_representation[left_fork_idx] = 
        '0' + philosopher_num;
    }
    else {
        philosophers[philosopher_num]->fork_representation[left_fork_idx] = 
        '-';
    }
    //If holding right fork
    if ((philosophers[philosopher_num]->right_fork)) {
        philosophers[philosopher_num]->fork_representation[right_fork_idx] = 
        '0' + ((philosopher_num + 1) % NUM_PHILOSOPHERS);
    }
    else {
        philosophers[philosopher_num]->fork_representation[right_fork_idx] = 
        '-';
    }
}

void print_header() {
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("| ============= ");
    }
    printf("|\n");

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("|       %c       ", 'A' + i);
    }
    printf("|\n");

    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        printf("| ============= ");
    }
    printf("|\n");
}

