#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pid = fork();

    if(pid == 0) {

        int child = getpid();
        printf("child %d: parent %d, session %d\n", child, getppid(), getsid(child));
    }
    else{
        int parent = getpid();
        printf("parent %d: parent %d, session %d\n", parent, getppid(), getsid(parent));
        sleep(2);
        
    }
    return 0;   
}