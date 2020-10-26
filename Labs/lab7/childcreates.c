#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int i = 0;
    int iterations;

    if (argc != 2) {
        fprintf(stderr, "Usage: forkloop <iterations>\n");
        exit(1);
    }

    iterations = strtol(argv[1], NULL, 10);

    while(i < iterations){
        int n = fork();
        
        if (n < 0) {
            perror("fork");
            exit(1);
        }
        
        printf("ppid = %d, pid = %d, i = %d\n", getppid(), getpid(), i++);
    
        if(n > 0){
            break;
        }
    }
    wait(NULL);
    return 0;
}
