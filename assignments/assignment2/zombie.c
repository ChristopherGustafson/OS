#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {

    int pid = fork();

    if(pid == 0) {
        
        printf("\nchild pid: %d\n", getpid());
        printf("check the status\n");
        sleep(10);
        printf("and again (after sleep)\n");

        return 42;
    }
    else{
        sleep(20);
        int res;
        wait(&res);
        printf("the result was %d\n", WEXITSTATUS(res));
        printf("and again\n");
        sleep(10);
    }

    printf("This is the end (%d)\n", getpid());
    
    return 0;
}