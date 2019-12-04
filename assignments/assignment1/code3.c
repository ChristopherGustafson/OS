#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
    
    int pid = getpid();    

    unsigned long p = 0x1;

    foo(&p);

   back:
    printf("   p: %p \n", &p);
    printf("   back: %p \n", &&back);

    printf("\n\n /proc/%d/maps \n\n", pid);
    char command[50];
    sprintf(command, "cat /proc/%d/maps", pid);
    system(command);

    return 0;
}

void zot(unsigned long *stop, int a1, int a2, int a3, int a4, int a5, int a6 ){
    unsigned long r = 0x3;

    unsigned long *i;
    for(i = &r; i <= stop; i++){
        printf(" %p   0x%1x\n", i, *i);
    }
}

void foo(unsigned long *stop ){
    unsigned long q = 0x2;

    zot(stop, 1, 2, 3, 4, 5, 6);
}