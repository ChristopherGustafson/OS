
/* Benchmark program for own implementation of malloc and free */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dlmalli.h"

#define MIN 16
#define MAX 256
#define BLOCKS 256
#define ROUNDS 50
#define LOOP 100

int main() {

    printf("Test");
    int flistSizes[ROUNDS];
    void *arr[BLOCKS];
    int index;
    for(int i = 0; i < BLOCKS; i++){
        arr[i] = NULL;
    }    

 

    for(int j = 0; j < ROUNDS; j++){        
        for(int i = 0; i < LOOP; i++){

            index = rand() % BLOCKS;
            if(arr[index] != NULL){
                dfree(arr[index]);
            }

            int size = ((rand() % (MAX-MIN+1)) + MIN);
            int *a;
            a = dalloc(size);

            if(a == NULL) {
                fprintf(stderr, "malloc failed\n");
                return(1);
            }

            arr[index] = a; 
        }

        printf("Round: %d\n", j+1);
        int count = printFlist();
        flistSizes[j] = count;
        printf("\n");
        
    }

    printf("Sizes of flist during the rounds: ");
    int sum = 0;
    for(int i = 0; i < ROUNDS; i++){
        printf("%d ", flistSizes[i]);
        sum += flistSizes[i];
    }
    printf("\nAvg: %d\n", sum/ROUNDS);

   

    return 0;
}

