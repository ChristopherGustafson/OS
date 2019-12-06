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

    //place waiting (joining) thread in ready queue

    if(this->join != NULL){
        enqueueReady(this->join);
    }

    // save result of execution
    this->retval = result;

    // free allocated memory structures
    free(this->context->uc_stack.ss_sp);

    // we're a zombie
    this->zombie = TRUE;

    // find the next thread to run
    running = dequeueReady();

    setcontext(running->context);    
}

int green_create(green_t *new, void *(*fun)(void*), void *arg) {

    ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
    getcontext(cntx);

    void *stack = malloc(STACK_SIZE);

    cntx->uc_stack.ss_sp = stack;
    cntx->uc_stack.ss_size = STACK_SIZE;
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
    struct green_t *susp = running;
    

    // add susp to ready queue
    sigprocmask(SIG_BLOCK, &block, NULL);

    enqueueReady(susp);
    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);

    sigprocmask(SIG_UNBLOCK, &block, NULL);
    return 0;
}

int green_join(green_t *thread, void **res) {

    if(thread->zombie){
        //collect the result
        res = thread->retval;
        return 0;
    }

    struct green_t *susp = running;
    // add as joining thread
    thread->join = susp;

    sigprocmask(SIG_BLOCK, &block, NULL);

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

    if(mutex != NULL){
        // release the lock if we have a mutex
        green_mutex_unlock(mutex);

        //schedule the suspended threads
        enqueueReady(susp);
    }

    // Select next thread for execution
    struct green_t *next = dequeueReady();
    running = next;
    swapcontext(susp->context, next->context);
    
    if(mutex != NULL){
        // try to take the lock
        if(mutex->taken) {
            //bad luck, suspend
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



