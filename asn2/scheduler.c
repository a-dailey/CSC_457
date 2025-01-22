#include <stdlib.h>
#include "lwp.h"

static thread head = NULL;
static thread tail = NULL;

void rr_init(void) {
    head = NULL;
    tail = NULL;
}

void rr_shutdown(void) {
    head = NULL;
    tail = NULL;
}

void rr_admit(thread new) {
    if (head == NULL) {
        head = new;
        tail = new;
        new->sched_one = new;
        new->sched_two = new;
    } else {
        new->sched_one = tail;
        new->sched_two = head;
        tail->sched_two = new;
        head->sched_one = new;
        tail = new;
    }
}

void rr_remove(thread victim) {
    if (head == NULL) return;


    if (victim == head && victim == tail) {
        //head and tail are same, only one thread in the list
        head = NULL;
        tail = NULL;
    } else {
        //remove victim from list
        thread prev = victim->sched_one;
        thread next = victim->sched_two;

        //getting rid of the middle man
        prev->sched_two = next;
        next->sched_one = prev;

        if (victim == head){
            head = next;
        } 
        if (victim == tail){
            tail = prev;
        }
    }
}

thread rr_next(void) {
    if (head == NULL) return NULL;
    //get next thread
    thread next_thread = head;
    //move head to next thread
    head = head->sched_two;
    return next_thread;
}

int rr_qlen(void) {
    if (head == NULL) return 0;

    int count = 0;
    thread curr = head;
    while (curr != tail) {
        count++;
        curr = curr->sched_two;
    }
    return count;
}

static struct scheduler rr_publish = {
    .init = rr_init,
    .shutdown = rr_shutdown,
    .admit = rr_admit,
    .remove = rr_remove,
    .next = rr_next,
    .qlen = rr_qlen
};

scheduler RoundRobin = &rr_publish;
