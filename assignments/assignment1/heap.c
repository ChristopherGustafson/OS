#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
    
    long *heap = (unsigned long*)calloc(400, sizeof(unsigned long));

    printf("heap[2]: 0x%1x\n", heap[2]);
    printf("heap[1]: 0x%1x\n", heap[1]);
    printf("heap[0]: 0x%1x\n", heap[0]);
    printf("heap[-1]: 0x%1x\n", heap[-1]);
    printf("heap[-2]: 0x%1x\n\n", heap[-2]);

    free(heap);

    printf("heap[2]: 0x%1x\n", heap[2]);
    printf("heap[1]: 0x%1x\n", heap[1]);
    printf("heap[0]: 0x%1x\n", heap[0]);
    printf("heap[-1]: 0x%1x\n", heap[-1]);
    printf("heap[-2]: 0x%1x\n", heap[-2]);
    


    /* Memory map */
    int pid = getpid(); 
    printf("\n\n /proc/%d/maps \n\n", pid);
    char command[50];
    sprintf(command, "cat /proc/%d/maps", pid);
    system(command);


    return 0;
}

