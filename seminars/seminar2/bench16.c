
/* Benchmark program for own implementation of malloc and free */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dlmalli.h"

#define MIN 16
#define MAX 256
#define BLOCKS 1000
#define ROUNDS 50000
#define LOOP 1000

int main() {

    int *arr[BLOCKS];

    for(int i = 0; i < BLOCKS; i++){
        arr[i] = dalloc(16);
    }

    for(int i = 0; i < ROUNDS; i++){
        for(int j = 0; j < BLOCKS; j++){
            *arr[j] = 1234;
        }
    }
}

