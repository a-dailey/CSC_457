#include "lwp.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#define STACK_SIZE 8 * 1024 * 1024

static scheduler current_scheduler = NULL;
static thread head = NULL;

tid_t current_tid_cnt = 1;

tid_t lwp_create(lwpfun func, void *args) {
    if (lwpfun == NULL) {
        perror("Error: lwp_create() called with NULL function pointer\n");
        return NO_THREAD;
    }
    //allocate space for new thread
    thread new_thread = (thread)malloc(sizeof(context));
    if (new_thread == NULL) {
        perror("Error: lwp_create() failed to allocate thread\n");
        return NO_THREAD;
    }
    //give thread a tid
    new_thread->tid = current_tid_cnt;
    current_tid_cnt++;

    //allocate space for stack
    size_t page_size = sysconf(_SC_PAGESIZE);

    struct rlimit rlim;
    size_t stack_resource_limit = getrlimit(RLIMIT_STACK, &rlim);
    if (stack_resource_limit == NULL || stack_resource_limit == 0 || stack_resource_limit == RLIM_INIFITY) {
        //no stack limit, use default
        stack_size = STACK_SIZE;
    } else {
        //use stack limit
        stack_size = rlim.rlim_cur;
    }
    
    //alignh stack size to page size
    size_t stack_size = ((stack_size + page_size - 1) / page_size) * page_size;

    //allocate stack
    unsigned long *stack = (unsigned long *) mmap(
        NULL, 
        stack_size, 
        PROT_READ | PROT_WRITE, 
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, 
        -1, 
        0
    );

    if (stack == MAP_FAILED) {
        perror("Error: lwp_create() failed to allocate stack\n");
        munmap(new_thread->stack, stack_size);
        free(new_thread);
        return NO_THREAD;
    }
    new_thread->stack = stack;
    new_thread->stacksize = stack_size;
    new_thread->status = MKTERMSTAT(LWP_LIVE, 0);
    new_thread->exited = NULL;
    new_thread->lib_one = NULL;
    new_thread->lib_two = NULL;
    new_thread->sched_one = NULL;
    new_thread->sched_two = NULL;

    //add to list
    // if (head == NULL) {
    //     //first thread, make head
    //     head = new_thread;
    //     new_thread->lib_one = new_thread;
    //     new_thread->lib_two = new_thread;
    // } else {
    //     /*lib1 == prev thread, lib2 == next thread
    //     inster new thread at end of the list, prev should be last thread, 
    //     next should be head*/

    //     new_thread->lib_one = head->lib_one;
    //     new_thread->lib_two = head;
    //     head->lib_one->lib_two = new_thread;
    //     head->lib_one = new_thread;
    // }
    
    //add to scheduler
    if (current_scheduler != NULL && current_scheduler->admit != NULL) {
        current_scheduler->admit(new_thread);
        add_thread(new_thread);
    } else {
        fprintf(stderr, "Error: Scheduler %p does not have an admit function\n", current_scheduler);
        exit(1);
    }

    unsigned long *stack_top = (unsigned long *) new_thread->stack + stack_size;

    stack_top--;

    *stack_top = (unsigned long) lwp_exit;
    stack_top--;
    *stack_top = (unsigned long) lwp_wrap;
    stack_top--;
    *stack_top = (unsigned long) 0;


    //initialize thread state
    //put args in rsi reg
    new_thread->state.rsi = (unsigned long) args;
    //put function in rdi reg
    new_thread->state.rdi = (unsigned long) func;
    //base pointer to bottom of stack
    new_thread->state.rbp = (unsigned long) stack_top;
    //stack pointer to bottom of stack
    new_thread->state.rsp = (unsigned long) stack_top;
    //save floatinng pt state
    new_thread->state.fxsave = FPU_INIT;

    //
    return new_thread->tid;
}

void  lwp_exit(int status){
    return;
}
tid_t lwp_gettid(void){
    return 0;
}
void  lwp_yield(void){
    return;
}
void  lwp_start(void){
    
}
tid_t lwp_wait(int *){
    return 0;
}
void  lwp_set_scheduler(scheduler fun){
    //shutdown other schedule if it exists
    if (current_scheduler != NULL) {
        current_scheduler->shutdown();
    }
    //set new scheduler
    current_scheduler = fun;
    //initialize new scheduler
    if (current_scheduler != NULL and new_scheduler->init != NULL) {
        current_scheduler->init();
        return;
    } 
    fprintf(stderr, "Error: Scheduler %p does not have an init function\n", current_scheduler);
    exit(1);
}

scheduler lwp_get_scheduler(void){
    return current_scheduler
}

thread tid2thread(tid_t tid){
    thread curr = head;
    while (curr != NULL) {
        if (curr->tid == tid) {
            return curr;
        }
        curr = curr->lib_two;
    }
    return NULL;
}

void add_thread(thread new) {
    if (head == NULL) {
        head = new;
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