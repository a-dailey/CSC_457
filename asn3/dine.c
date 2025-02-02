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

#define DEFAULT_CYCLES 10  

static int cycles;

static sem_t forks[NUM_PHILOSOPHERS];

static sem_t print_status_lock;

philosopher philosophers[NUM_PHILOSOPHERS];

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


    //initialize philosopher data
    int i;
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosophers[i]->thread = NULL;
        philosophers[i]->state = "";
        philosophers[i]->left_fork = 0;
        philosophers[i]->right_fork = 0;
    }


    //initialize semaphores
    sem_init(&print_status_lock, SHARED_SEM, SEM_START_VAL);
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&forks[i], SHARED_SEM, SEM_START_VAL) == -1) {
            perror("Error: sem_init() failed\n");
            return 1;
        }
    }

    //create threads
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        philosopher_ids[i] = i;  // Store philosopher index safely
        if (pthread_create(&philosophers[i].thread, NULL, dine, (void *)i) != 0) {
            perror("Error: pthread_create() failed");
            return 1;
        }
    }

    //join threads
    for (i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (pthread_join(philosophers[i].thread, NULL) != 0) {
            perror("Error: pthread_join() failed");
            return 1;
        }
    }
    
}


void *dine(void *arg) {

    //philosopher number
    int philosopher_num = (int)arg;

    philosopher *philosopher = philosophers[philosopher_num];

    //left and right forks
    int left_fork = philosopher;
    int right_fork = (philosopher + 1) % NUM_PHILOSOPHERS;

    //fork representations


    //eat-think cycles
    int i;  
    for (i = 0; i < cycles; i++) {
        //think
        if (philosopher_num % 2 == 0) {
            //even philosophers pick up right fork first
            sem_wait(&forks[right_fork]);
            philosopher->right_fork = 1;
            update_fork_representation(philosopher_num);
            print_status(philosopher_num);
            sem_wait(&forks[left_fork]);
            philosopher->left_fork = 1;
            update_fork_representation(philosopher_num);
            print_status(philosopher_num);
            //has both forks, is eating
            philosopher->state = "Eat  ";
            print_status(philosopher_num);
            dawdle();
            //put down forks
            sem_post(&forks[right_fork]);
            print_status(philosopher_num);
            

        }
        else {
            //odd philosophers pick up left fork first
            sem_wait(&forks[left_fork]);
            philosopher->left_fork = 1;
            update_fork_representation(philosopher_num);
            print_status(philosopher_num);
            sem_wait(&forks[right_fork]);
            philosopher->right_fork = 1;
            update_fork_representation(philosopher_num);
            print_status(philosopher_num);
            //has both forks, is eating
            philosopher->state = "Eat  ";
            print_status(philosopher_num);
            dawdle();
            //put down forks
            sem_post(&forks[left_fork]);
            print_status(philosopher_num);

        }
    }

}

void print_status(int philosopher) {
    sem_wait(&print_status_lock);
    int i;
    for (i=0; i < NUM_PHILOSOPHERS; i++) {
        printf("| %s %s ", philosophers[i]->fork_representation, philosophers[i]->state);
    }
    printf("|\n");
    sem_post(&print_status_lock);
}

void update_fork_representation(int philosopher_num) {
    strcpy(philosophers[philosopher_num]->fork_representation, "-----");  // Reset all forks to '-'

    int left_fork_idx = philosopher_id;
    int right_fork_idx = (philosopher_id + 1) % NUM_PHILOSOPHERS;

    // If holding left fork
    if ((philosophers[philosopher_num]->left_fork)) {
        philosophers[philosopher_num]->fork_representation[left_fork_idx] = '0' + philosopher_id;
    }
    // If holding right fork
    if ((philosophers[philosopher_num]->right_fork)) {
        philosophers[philosopher_num]->fork_representation[right_fork_idx] = '0' + philosopher_id;
    }
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
int msec = (int)(((double)random() / RAND MAX) * 1000);
tv.tv sec = 0;
tv.tv nsec = 1000000 * msec;
if ( −1 == nanosleep(&tv,NULL) ) {
perror("nanosleep");
}
}
