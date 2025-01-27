#include "lwp.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>

#define DEBUG_LWP getenv("DEBUG_LWP")

#define STACK_SIZE 8 * 1024 * 1024
#define ALIGNMENT_MASK 0xF

static scheduler current_scheduler = NULL;
static thread head = NULL;
static thread tail = NULL;
static thread current_thread = NULL;

static waiting_thread *waiting_head = NULL; 



static tid_t current_tid_cnt = 1;


tid_t lwp_create(lwpfun func, void *args) {
    if (func == NULL) {
        perror("Error: lwp_create() called with NULL function pointer\n");
        return NO_THREAD;
    }
    //allocate space for new thread
    thread new_thread = (thread)malloc(sizeof(context));
    if (new_thread == NULL) {
        perror("Error: lwp_create() failed to allocate thread\n");
        return NO_THREAD;
    }

    if (current_scheduler == NULL) {
        lwp_set_scheduler(RoundRobin);

    }
    //give thread a tid
    new_thread->tid = current_tid_cnt;
    current_tid_cnt++; 

    //allocate space for stack
    size_t page_size = sysconf(_SC_PAGESIZE);

    size_t stack_size = 0;
    struct rlimit rlim;
    size_t stack_resource_limit = getrlimit(RLIMIT_STACK, &rlim);
    if (stack_resource_limit == -1) {
        perror("Error: lwp_create() failed to get stack resource limit\n");
        free(new_thread);
        return NO_THREAD;
    }
    if (stack_resource_limit == 0 || stack_resource_limit == RLIM_INFINITY) {
        //no stack limit, use default
        stack_size = STACK_SIZE;
    } else {
        //use stack limit
        stack_size = rlim.rlim_cur;
    }
    
    //alignh stack size to page size
    stack_size = ((stack_size + page_size - 1) / page_size) * page_size;
    if (DEBUG_LWP) {
        fprintf(stderr, "Thread: %lu stack size: %lu\n", new_thread->tid, stack_size);
    }

    //allocate stack
    void *stack = mmap(
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
    new_thread->stack = (unsigned long *) stack;
    new_thread->stacksize = stack_size;
    new_thread->status = MKTERMSTAT(LWP_LIVE, 0);
    new_thread->exited = NULL;
    new_thread->lib_one = NULL;
    new_thread->lib_two = NULL;
    new_thread->sched_one = NULL;
    new_thread->sched_two = NULL;



    unsigned long *stack_top = (unsigned long *) ((unsigned long) stack + stack_size);

    stack_top = (unsigned long *) ((unsigned long) stack_top & ~ALIGNMENT_MASK);
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

    //add to scheduler
    if (current_scheduler != NULL && current_scheduler->admit != NULL) {
        //add to scheduler
        current_scheduler->admit(new_thread);
        //add to library list
        add_thread(new_thread);
    } else {
        fprintf(stderr, "Error: Scheduler %p does not have an admit function\n", current_scheduler);
        exit(1);
    }

    return new_thread->tid; 
}

void  lwp_exit(int status){
    if (DEBUG_LWP) {
        fprintf(stderr, "Thread %lu exited with status %d\n", current_thread->tid, status);
    }
    thread curr = current_thread; 
    //set status to terminated
    curr->status = MKTERMSTAT(LWP_TERM, status);
    if (DEBUG_LWP) {
        fprintf(stderr, "Thread %lu status: %d\n", curr->tid, curr->status);
    }
    //remove from scheduler
    if (current_scheduler != NULL && current_scheduler->remove != NULL) {
        current_scheduler->remove(curr);
        thread replacement = pop_thread_waiting();
        if (replacement != NULL) {
            replacement->exited = curr;
            current_scheduler->admit(replacement);
        } 
    } else {
        fprintf(stderr, "Error: Scheduler %p does not have a remove function\n", current_scheduler);
        exit(1);
    }

    lwp_yield();
    
}
tid_t lwp_gettid(void){
    if (current_thread == NULL) { 
        return NO_THREAD;
    }
    return current_thread->tid;
}
void  lwp_yield(void){
    thread next = current_scheduler->next();

    thread prev = current_thread;

    if (next == NULL) {
       exit(LWPTERMSTAT(prev->status));
    }

    current_thread = next;

    //remove prev from scheduler
    // scheduler->remove(prev);

    swap_rfiles(&(prev->state), &(next->state));
    
    return;
}
void  lwp_start(void){
    thread main_thread = (thread)malloc(sizeof(context));
    main_thread->tid = 0;
    main_thread->stack = NULL;
    main_thread->stacksize = 0;
    main_thread->status = MKTERMSTAT(LWP_LIVE, 0);
    main_thread->exited = NULL;
    main_thread->lib_one = NULL;
    main_thread->lib_two = NULL;
    main_thread->sched_one = NULL;
    main_thread->sched_two = NULL;

    if (current_scheduler == NULL) {
        fprintf(stderr, "Error: No scheduler set\n");
        exit(1);
    }

    current_scheduler->admit(main_thread);
    
    thread next = current_scheduler->next();
    current_thread = next;

    //set current context to next thread, save context of main thread
    swap_rfiles(&(main_thread->state), &(next->state));
    return;
}
tid_t lwp_wait(int *status){
    thread curr = current_thread;

    //check if any threads can be continued, if not return NO_THREAD
    if (current_scheduler->qlen() <= 1) {
        return NO_THREAD;
    }

    //check if any threads terminated
    thread og_head = curr;
    while (curr != NULL && curr != og_head) {
        if (LWPTERMINATED(curr->status)) {
            if (status != NULL){
                *status = LWP_TERM;
            }
            //thread terminated, deallocate and return
            //first connect lib_one and lib_two
            if (curr == head) {
                head = curr->lib_two;
            }
            if (curr == tail) {
                tail = curr->lib_one;
            }
            if (curr->lib_one != NULL) {
                curr->lib_one->lib_two = curr->lib_two;
            }
            if (curr->lib_two != NULL) {
                curr->lib_two->lib_one = curr->lib_one;
            }
            munmap(curr->stack, curr->stacksize);
            free(curr);
            return curr->tid;
        }
        curr = curr->lib_two;
    }
    if (status != NULL) {
        *status = LWP_LIVE;
    }
    //no threads terminated, put in waiting list and block current thread
    current_scheduler->remove(current_thread);
    add_thread_waiting(current_thread);

    //block+
    lwp_yield();

    //deallocate terminated thread once returned
    if (current_thread->exited != NULL) {
        thread terminated = current_thread->exited;
        remove_thread(terminated);
        if (terminated->stack != NULL){
            munmap(terminated->stack, terminated->stacksize);
        }
        
        tid_t tid = terminated->tid;
        free(terminated); 
        return tid;
    }

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
    if (current_scheduler != NULL && current_scheduler->init != NULL) {
        current_scheduler->init();
        return;
    } 
    fprintf(stderr, "Error: Scheduler %p does not have an init function\n", current_scheduler);
    exit(1);
}

scheduler lwp_get_scheduler(void){
    return current_scheduler;
}

thread tid2thread(tid_t tid){
    thread curr = head;
    thread initial_head = head;
    if (curr == NULL) {
        return NULL;
    }
    if (curr->tid == tid) {
        return curr;
    }
    curr = curr->lib_two;
    while (curr != NULL && curr != initial_head) {
        if (curr->tid == tid) {
            return curr;
        }
        curr = curr->lib_two;
    }
    return NULL;
}

void lwp_wrap(lwpfun fun, void *args){
    int ret = fun(args);
    lwp_exit(ret);
}

void add_thread(thread new) {
    if (head == NULL) {
        head = new;
        new->lib_one = new; //prev
        new->lib_two = new; //next
        tail = new;

    } else {
        //new thread is now tail 
        new->lib_one = tail;
        new->lib_two = head;
        tail->lib_two = new;
        head->lib_one = new;
        tail = new;
    }
}

void add_thread_waiting(thread new) {
    waiting_thread *new_waiting_thread = (waiting_thread *)malloc(sizeof(waiting_thread));
    if (new_waiting_thread == NULL) {
        perror("Error: add_thread_waiting() failed to allocate new_waiting_thread\n");
        return;
    }
    new_waiting_thread->thread = new;
    new_waiting_thread->next = NULL;

    if (waiting_head == NULL) {
        waiting_head = new_waiting_thread;
    } else {
        waiting_thread *curr = waiting_head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_waiting_thread;
    }
}

thread pop_thread_waiting(void) {
    if (waiting_head == NULL) {
        return NULL;
    }
    waiting_thread *oldest = waiting_head;
    thread oldest_thread = oldest->thread;
    waiting_head = waiting_head->next;
    free(oldest);
    return oldest_thread;  
}
void remove_thread(thread victim) {
    if (head == NULL) {
        return;
    }
    if (victim == head && victim == tail) {
        head = NULL;
        tail = NULL;
        return;
    } else {
        thread prev = victim->lib_one;
        thread next = victim->lib_two;

        if (prev != NULL) {
            prev->lib_two = next;
        }
        if (next != NULL) {
            next->lib_one = prev;
        }

        if (victim == head) {
            head = next;
        }
        if (victim == tail) {
            tail = prev;
        }
        return;
    }
}