#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
   */

// execl in the child process only
// execl in the parent process will take over the parent process, after execl is done,
// parent just exit and can't do anything
  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
    if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  
  int fd[2];

  pipe(fd);

  int f = fork();

  if(f == 0){ 
    // in the child process
    
    // close the write end of the pipe (in child)
    if(close(fd[1]) == -1){
      perror("close failed in write of child");
      exit(1);
    }

    // make the child's STDIN from the read end of the pipe
    // dup2(redirect to, fd that will be redirected)
    dup2(fd[0], STDIN_FILENO);

    // exec the validate program
    // execl takes command line argument only, not stdin
    execl("./validate", "validate", NULL); 
  }
  else if(f > 0){
    // in the parent process

    // close the read end of the pipe (in parent)
    if(close(fd[0]) == -1){
      perror("close failed in read of parent");
      exit(1);
    };

    // write user_id and password into the pipe
    // the validate is going to read 10 bytes every time
    write(fd[1], user_id, 10);
    write(fd[1], password, 10);

    // close the write end of the pipe (in parent)
    if(close(fd[1]) == -1){
      perror("close failed in write of parent");
      exit(1);
    }

    int status;
    if(wait(&status) != -1){
      switch(WEXITSTATUS(status)){
        case 0:
          printf("%s", SUCCESS);
          break;
        case 1:
          break;
        case 2:
          printf("%s", INVALID);
          break;
        case 3:
          printf("%s", NO_USER);
          break;
        
      }
    }
  }
  else{
    // f < 0 
    perror("fork\n");
    exit(1);
  }

  return 0;
}