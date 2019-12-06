#include <stdio.h>
#include <unistd.h>
#include "green.h"

int flag = 0;
int count = 0;
green_cond_t cond;
green_mutex_t mutex;

void *test(void *arg) {
    int i = *(int*)arg;
    int loop = 100000;
    while(loop > 0) {
        printf("thread %d: %d\n", i, loop);
        loop--;
        green_yield();
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
    int loop = 10000000;
    int id = *(int*)arg;

    while(loop > 0){
        printf("thread %d: %d\n", id, loop);
        loop--;

        green_mutex_lock(&mutex);

        count++;

        green_mutex_unlock(&mutex);
    }
}

void *test_mutex_wait(void *arg){

    int id = *(int*)arg;
    int loop = 100000;
   
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
    green_t g0, g1;
    green_cond_init(&cond);
    green_mutex_init(&mutex);

    int a0 = 0;
    int a1 = 1;

    green_create(&g0, test_mutex_wait, &a0);
    green_create(&g1, test_mutex_wait, &a1);

    green_join(&g0, NULL);
    green_join(&g1, NULL);

    printf("done\n");
    return 0;
}