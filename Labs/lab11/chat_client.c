#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#ifndef PORT
  #define PORT 53035
#endif
#define BUF_SIZE 128

int main(void) {
    // Create the socket FD.
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(sock_fd);
        exit(1);
    }

    // Get the user to provide a name.
    char buf[2 * BUF_SIZE + 1];            // 2x to allow for usernames
    printf("Please enter a username: ");

    // remove the fflush ? 
    fgets(buf, BUF_SIZE, stdin);
    // set null determinator at the enter point
    for(int i = 0; i < BUF_SIZE; i++){
        if(buf[i] == '\n'){
            buf[i] = '\0';
            break;
        }
    }
   
    // Write the username to the sever after successful connection
    int writeBytes;
    writeBytes = write(sock_fd, buf, BUF_SIZE);
    if(writeBytes != BUF_SIZE){
        perror("fail to write username");
        exit(1);
    }


    // since select() will modify the list of fds for listening
    // we make a copy of all fds we need to listen to that is unchanged
    // for a client, it only have to listen to the user input and the input from the server
    fd_set all_fds;
    // set up all_fds for select()
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);
    FD_SET(STDIN_FILENO, &all_fds); // set the FD for the reader's input

    //  check link : https://blog.csdn.net/qimi123456/article/details/52356377 for the usage of maxfd and nfd in select()
    int max_fd;
    if (sock_fd > STDIN_FILENO){
        max_fd = sock_fd;
    }
    else{
        max_fd = STDIN_FILENO;
    }

    char message[BUF_SIZE + 1];
    // Read input from the user, send it to the server, and then accept the
    // echo that returns. Exit when stdin is closed.
    while (1) {
        fd_set curr_listen_fds = all_fds;
        int fds_with_contents;
        // select() returns the number of file descriptors that have contents ready for reading
        fds_with_contents = select(max_fd + 1, &curr_listen_fds, NULL, NULL, NULL);

        // select() return -1 
        if (fds_with_contents < 0){
            perror("select");
            exit(1);
        }
        if (FD_ISSET(STDIN_FILENO, &curr_listen_fds)){
            // the user gives some input  
            int num_read = read(STDIN_FILENO, message, BUF_SIZE);
            if (num_read == 0) {
                break;
            }
            message[num_read] = '\0';

            /* 
            * We should really send "\r\n" too, so the server can identify partial 
            * reads, but you are not required to handle partial reads in this lab.
            */
           
            // write user input to the server
            int num_written = write(sock_fd, message, num_read);
            if (num_written != num_read) {
                perror("client: write");
                close(sock_fd);
                exit(1);
            }
        }
        
        if (FD_ISSET(sock_fd, &curr_listen_fds)){
            // the server echos messages
            int num_read = read(sock_fd, message, BUF_SIZE);
            if(num_read < 0){
                perror("failed to receive echos from server");
                exit(1);
            }
            if(num_read == 0){
                break;
            }
            message[num_read] = '\0';
            printf("%s", message);
        }
        
    }

    close(sock_fd);
    return 0;
}
