#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "green.h"

#define LOOPS 10000000    

int flag = 0;
int count;
green_cond_t cond;

green_mutex_t mutex;
pthread_mutex_t mutexp;

void *test(void *arg) {
    int i = *(int*)arg;
    int loop = LOOPS;
    while(loop > 0) {
        //printf("thread %d: %d\n", i, loop);
        loop--;
        green_yield();
    }
}

void *test_p(void *arg) {
    int i = *(int*)arg;
    int loop = LOOPS;
    while(loop > 0) {
        //printf("thread %d: %d\n", i, loop);
        loop--;
        pthread_yield();
    }
}

void *test_cond(void *arg){
    int id = *(int*)arg;
    int loop = 100000;
    while(loop > 0){
        if(flag == id) {
            printf("thread %d: %d\n", id, loop);
            loop--;
            flag = (id + 1) % 2;
            green_cond_signal(&cond);
        }
        else{
            //green_cond_wait(&cond);
        }
    }
}

void *test_mutex(void *arg){
    int loop = LOOPS;
    int id = *(int*)arg;

    while(loop > 0){
        //printf("thread %d: %d\n", id, loop);
        loop--;
        green_mutex_lock(&mutex);
        count++;
        green_mutex_unlock(&mutex);
    }
}


void *test_mutex_p(void *arg){
    int loop = LOOPS;
    int id = *(int*)arg;
 
    while(loop > 0){
        //printf("thread %d: %d\n", id, loop);
        loop--;
        pthread_mutex_lock(&mutexp);
        count++;
        pthread_mutex_unlock(&mutexp);
    }
}

void *test_mutex_wait(void *arg){

    int id = *(int*)arg;
    int loop = 1000000;
   
    while(loop > 0) {
        printf("thread %d: %d\n", id, loop);
        green_mutex_lock(&mutex);
        while(flag != id){
            green_mutex_unlock(&mutex);
            green_cond_wait(&cond, &mutex);
        }
        flag = (id+1) % 2;
        green_cond_signal(&cond);
        green_mutex_unlock(&mutex);
        loop--;
    }
}

int main() {

    printf("green thread\n");
    clock_t start = clock();

    green_t g0, g1;
    //green_cond_init(&cond);
    green_mutex_init(&mutex);
    int a0 = 0;
    int a1 = 1;
    count = 0;

    green_create(&g0, test_mutex, &a0);
    green_create(&g1, test_mutex, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);

    clock_t end = clock();
    float seconds = (float)(end - start) / CLOCKS_PER_SEC;
    printf("green thread time: %f\n", seconds);

    printf("\npthread\n");
    clock_t startp = clock();


    pthread_t p0, p1;
    pthread_mutex_init(&mutexp, NULL);
    
    int b0 = 0;
    int b1 = 1;
    count = 0;

    pthread_create(&p0, NULL, test_mutex_p, &b0);
    pthread_create(&p1, NULL, test_mutex_p, &b1);

    pthread_join(p0, NULL);
    pthread_join(p1, NULL);


    clock_t endp = clock();
    float secondsp = (float)(endp - startp) / CLOCKS_PER_SEC;
    printf("pthread time: %f\n", secondsp);

    printf("done\n");
    return 0;
}