#define _GNU_SOURCE
#include <stdio.h>  
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096

#define PERIOD 100

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;
static green_t *readyFirst = NULL;
static green_t *readyLast = NULL;

static sigset_t block;

void timer_handler(int);
void enqueueReady(green_t*) ;
struct green_t *dequeueReady();

static void init() __attribute__((constructor));

void init() {
    getcontext(&main_cntx);

    sigemptyset(&block);
    sigaddset(&block, SIGVTALRM);

    struct sigaction act = {0};
    struct timeval interval;
    struct itimerval period;

    act.sa_handler = timer_handler;
    assert(sigaction(SIGVTALRM, &act, NULL) == 0);

    interval.tv_sec = 0;
    interval.tv_usec = PERIOD;
    period.it_interval = interval;
    period.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &period, NULL);
}  

void timer_handler(int sig){
    green_t *susp = running;
    enqueueReady(susp);

    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);
}


void enqueueReady(green_t * element) {

    if(readyFirst == NULL){
        readyFirst = element;
        readyLast = element;
    }
    else{
        readyLast->next = element;
        readyLast = element;
    }
}

struct green_t *dequeueReady() {

    if(readyFirst == NULL){
        readyLast = NULL;
        return NULL;
    }

    struct green_t *dequeued = readyFirst;
    readyFirst = dequeued->next;
    dequeued->next = NULL;
    return dequeued;
}

void printReadyQ() {

    struct green_t *current = readyFirst;
    while(current != NULL){
        printf("Ready queue element: %p\n", current->arg);
        current = current->next;
    }

}


void green_thread() {

    struct green_t *this = running;
    void *result = (*this->fun)(this->arg);

    //place waiting (joining) thread in ready queue, this thread is waiting for my result
    if(this->join != NULL){
        enqueueReady(this->join);
    }

    // save result of execution
    this->retval = result;

    /*
    We cannot do this, we cannot free our own stack cause otherwise we cant return from the function
    // free allocated memory structures
    free(this->context->uc_stack.ss_sp);
    // Free context?
    free(this->context);
    */

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    green_t *next = dequeueReady();
    running = next;
    setcontext(running->context);    
}


// Populate the green_t datastructure and then add to ready queue
int green_create(green_t *new, void *(*fun)(void*), void *arg) {

    // Malloc space for context
    ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    // Malloc space for the stack
    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
    //When context start executing, start exucuting function "green_thread"
    makecontext(cntx, green_thread, 0);

    new->context = cntx;
    new->fun = fun;
    new->arg = arg;
    new->next = NULL;
    new->join = NULL;
    new->retval = NULL;
    new->zombie = FALSE;

    //add new to the ready queue
    enqueueReady(new);

    return 0;
}

int green_yield() {
    sigprocmask(SIG_BLOCK, &block, NULL);

    struct green_t *susp = running;
    // add susp to ready queue
    enqueueReady(susp);
    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_join(green_t *thread, void **res) {

    sigprocmask(SIG_BLOCK, &block, NULL);

    if(thread->zombie){
        //collect the result
        res = thread->retval;
        return 0;
    }

    struct green_t *susp = running;
    // add as joining thread
    thread->join = susp;

    // select the next thread for execution
    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

// Initialize a green condition variable
void green_cond_init(green_cond_t *cond_var){
    cond_var->first = NULL;
}

// Suspend the current thread to cond_var
int green_cond_wait(green_cond_t *cond_var, green_mutex_t *mutex){
    sigprocmask(SIG_BLOCK, &block, NULL);

    // Suspend the running thread on condition
    green_t *susp = running;
    green_t *current = cond_var->first;
    if(current != NULL){
        while(current->next != NULL){
            current = current->next;
        }
        current->next = susp;
    }
    else{
        cond_var->first = susp;
    }

    /* SOMETHING IS PROBABLY WRONG HERE */

    if(mutex != NULL){
        // release the lock if we have a mutex
        green_mutex_unlock(mutex);

        //schedule the suspended threads
        enqueueReady(mutex->firstWaiting);
    }

    // Select next thread for execution
    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);
    
    if(mutex != NULL){
        // try to take the lock
        if(mutex->taken) {
            //bad luck, suspend on the lock
            if(mutex->firstWaiting == NULL){
                mutex->firstWaiting = susp;
            }
            else{
                green_t *current = mutex->firstWaiting;
                while(current->next != NULL){
                    current = current->next;
                }
                current->next = susp;
            }
            green_t *next = dequeueReady();
            running = next;
            swapcontext(susp->context, next->context);
        }
        else{
            // take lock
            mutex->taken = TRUE;
        }
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

// move the first suspended thread to the ready queue
void green_cond_signal(green_cond_t *cond_var){
    if(cond_var->first != NULL){
        enqueueReady(cond_var->first);
        cond_var->first = cond_var->first->next;
    }
}

int green_mutex_init(green_mutex_t *mutex){
    mutex->taken = FALSE;
    mutex->firstWaiting = NULL;
    return 0;
}

int green_mutex_lock(green_mutex_t *mutex){
    sigprocmask(SIG_BLOCK, &block, NULL);

    struct green_t *susp = running;
    if(mutex->taken){
        if(mutex->firstWaiting == NULL){
            mutex->firstWaiting = susp;
        }
        else{
            green_t *current = mutex->firstWaiting;
            while(current->next != NULL){
                current = current->next;
            }
            current->next = susp;
        }
        green_t *next = dequeueReady();
        running = next;
        swapcontext(susp->context, next->context);
    }
    else{
        //Take the lock
        mutex->taken = TRUE;
    }
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_mutex_unlock(green_mutex_t *mutex){
    sigprocmask(SIG_BLOCK, &block, NULL);

    if(mutex->firstWaiting != NULL){
        enqueueReady(mutex->firstWaiting);
        mutex->firstWaiting = mutex->firstWaiting->next;
    }
    else{
        mutex->taken = FALSE;
    }
    sigprocmask(SIG_BLOCK, &block, NULL);
    return 0;
}



