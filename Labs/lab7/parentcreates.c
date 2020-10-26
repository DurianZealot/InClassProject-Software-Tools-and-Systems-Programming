#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int iterations;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: forkloop <iterations>\n");
        exit(1);
    }

    iterations = strtol(argv[1], NULL, 10);
    
    for(int i = 0; i < iterations; i++){
	int c = fork();
	wait(NULL);
	printf("ppid = %d, pid = %d, i = %d\n", getppid(), getpid(), i);
	if (c == 0){
		// at child
		exit(0);
        }
    }
    return 0;
}
