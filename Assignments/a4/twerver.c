#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
#define PORT 53035
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: \r\n"
#define USERNAME_MSG    "The username you enter is not valid. Please choose another username: \r\n"
#define SEND_MSG    "send"
#define SHOW_MSG    "show"
#define FOLLOW_MSG  "follow"
#define UNFOLLOW_MSG    "unfollow"
#define QUIT_MSG    "quit"
#define INVALID_COMMAND "Invalid command\r\n"
#define BUF_SIZE    256
#define MSG_LIMIT   8
#define FOLLOW_LIMIT    5

#define NOT_ACTIVE_USR_ERR  2
#define ALREADY_FOLLOWED    3
#define NO_SPACE_ERR        4
#define NOT_FOLLOWED        5
struct client
{
    int     fd;
    struct in_addr  ipaddr;
    char        username[BUF_SIZE];
    char        message[MSG_LIMIT][BUF_SIZE];
    struct client   *following[FOLLOW_LIMIT];       /* Clients this user is following, a list of pointers to structs */
    struct client   *followers[FOLLOW_LIMIT];       /* Clients who follow this user, a list of pointers to structs */
    char        inbuf[BUF_SIZE];                /* Used to hold input from the client */
    char        *in_ptr;                        /* A pointer into inbuf to help with partial reads */
    struct client   *next;
};


/* Provided functions. */
void add_client( struct client **clients, int fd, struct in_addr addr );


void remove_client( struct client **clients, int fd );


/* Helper functions. */
int delete_from_list_of_clients( struct client **clients_arr, int fd );


int find_network_newline( const char *buf, int n );


int validate_username( char * username, struct client * active_clients );


int read_client_input( struct client * p );


void reset_client_buf( struct client * p );


struct client * is_active( char * to_follow_username, struct client * active_clients );


int not_exist( char * to_follow_username, struct client ** list_of_clients );


int enough_space_for_clients( struct client ** list_of_clients );


void follow_user( struct client * followed, struct client * follow );


void send_message( struct client ** active_clients, struct client *sender, char * msg_to_send );


void unfollow_user( struct client * unfollowed, struct client * follow );


int enough_space_for_msg( struct client * client );


void show_sent_msg( struct client **active_clients, struct client *curr_client, struct client *show_to );


void show_prev_msg( struct client **active_clients, struct client *show_to );




/*
 * These are some of the function prototypes that we used in our solution
 * You are not required to write functions that match these prototypes, but
 * you may find them helpful when thinking about operations in your program.
 */

/* Send the message in s to all clients in active_clients. */
void announce( struct client *active_clients, char *s );


/* Move client c from new_clients list to active_clients list. */
void activate_client( struct client *c,
                      struct client **active_clients_ptr, struct client **new_clients_ptr );


void activate_client( struct client *c, struct client **active_clients_ptr, struct client **new_clients_ptr )
{
    /* if client c is in the first place in the new_clients list */
    if ( c == *new_clients_ptr )
    {
        struct client * second_client = (*new_clients_ptr)->next;

        *new_clients_ptr = second_client;
    }
    else
    {
        struct client * curr_new_client = *new_clients_ptr;
        while ( curr_new_client->next != c )
        {
            curr_new_client = curr_new_client->next;
        }
        curr_new_client->next = curr_new_client->next->next;
    }

    c->next = NULL;

    if ( *active_clients_ptr == NULL )
    {
        *active_clients_ptr = c;
    }
    else
    {
        struct client * curr_active_client = *active_clients_ptr;
        while ( curr_active_client->next != NULL )
        {
            curr_active_client = curr_active_client->next;
        }
        curr_active_client->next = c;
    }

}


/*
 * The set of socket descriptors for select to monitor.
 * This is a global variable because we need to remove socket descriptors
 * from allset when a write to a socket fails.
 */
fd_set allset;


/*
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client( struct client **clients, int fd, struct in_addr addr )
{
    struct client *p = malloc( sizeof(struct client) );
    if ( !p )
    {
        perror( "malloc" );
        exit( 1 );
    }

    printf( "Adding client %s\n", inet_ntoa( addr ) );
    p->fd       = fd;
    p->ipaddr   = addr;
    p->username[0]  = '\0';
    p->in_ptr   = p->inbuf;
    p->inbuf[0] = '\0';
    p->next     = *clients;

    /* initialize an empty array for following and followers */
    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        p->following[i] = NULL;
        p->followers[i] = NULL;
    }

    /* initialize messages to empty strings */
    for ( int i = 0; i < MSG_LIMIT; i++ )
    {
        p->message[i][0] = '\0';
    }

    *clients = p;
}


/*
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client( struct client **clients, int fd )
{
    struct client **p;

    /* find the client we want to remove in the list `clients` */
    for ( p = clients; *p && (*p)->fd != fd; p = &(*p)->next )
        ;

    /*
     * Now, p points to (1) top, or (2) a pointer to another client
     * This avoids a special case for removing the head of the list
     */
    if ( *p )
    {
        /*
         * Remove the client from other clients' following/followers
         * lists
         */
        char to_remove_client_name[BUF_SIZE];
        strcpy( to_remove_client_name, (*p)->username );


        /*
         If A and B follow each other and A disconnects,
         I show message in server saying that 'B no longer follows A' and
         'A no longer has B as a follower'.
         In fact, A no longer follows B and B no longer has A as a follower.
         (It is done by code by not shown in order to keep simplicity)
         */

        for ( int i = 0; i < FOLLOW_LIMIT; i++ )
        {
            if ( (*p)->followers[i] != NULL )
            {
                int p_pos = delete_from_list_of_clients( ( (*p)->followers[i])->following, fd );

                /* check if p gets removed from the following */
                if ( p_pos < 0 )
                {
                    printf( "Error: can't find the client %s in the client %s's following list\r\n", to_remove_client_name, ( (*p)->followers[i])->username );
                }
                printf( "%s no longer follows %s since %s is diconnected\r\n", ( (*p)->followers[i])->username, to_remove_client_name, to_remove_client_name );
            }
            if ( (*p)->following[i] != NULL )
            {
                int p_pos = delete_from_list_of_clients( ( (*p)->following[i])->followers, fd );

                /* check if p gets removed from the followers */
                if ( p_pos < 0 )
                {
                    printf( "Error: can't find the client %s in the client %s's followers list\r\n", to_remove_client_name, ( (*p)->following[i])->username );
                }
                else
                {
                    printf( "%s no longer has %s as follower since %s is diconnected\r\n", to_remove_client_name, ( (*p)->following[i])->username, to_remove_client_name );
                }
            }
        }


        /* Remove the client */
        struct client *t = (*p)->next;
        printf( "Removing client %d %s\n", fd, inet_ntoa( (*p)->ipaddr ) );
        FD_CLR( (*p)->fd, &allset );
        close( (*p)->fd );
        free( *p );
        *p = t;
    }
    else
    {
        fprintf( stderr,
                 "Trying to remove fd %d, but I don't know about it\n", fd );
    }
}


/*
 *  Remove the client represented by fd from the clients_list
 */
int delete_from_list_of_clients( struct client **clients_arr, int fd )
{
    /*
     * clients_arr is the pointer to the first slot which stores a pointer of struct client in the array
     * not the first pointer in the array
     */
    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        /* clients_arr[i] is a pointer a struct */
        if ( clients_arr[i] != NULL && clients_arr[i]->fd == fd )
        {
            clients_arr[i] = NULL;
            return (i);
        }
    }
    /* never gets here */
    return (-1);
}


void announce( struct client *active_clients, char *s )
{
    if ( active_clients != NULL )
    {
        struct client *curr_client = active_clients;
        while ( curr_client != NULL )
        {
            if ( write( curr_client->fd, s, strlen( s ) ) != strlen( s ) )
            {
                /*
                 * When we failed to write to a file descriptor, the socket is closed
                 * Deal with this situation that the client is already disconnected
                 */
                int remove_client_fd = curr_client->fd;
                remove_client( &active_clients, remove_client_fd );
                printf( "Failed to send annoucement to client [%d]\r\n", remove_client_fd );
            }
            curr_client = curr_client->next;
        }
    }
}


/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 * strchr works with a string which must have a null terminator
 * in this case, we do not have a null terminator
 */
int find_network_newline( const char *buf, int n )
{
    int i = 0;
    while ( i + 1 < n )
    {
        if ( buf[i] == '\r' && buf[i + 1] == '\n' )
        {
            return (i + 2);
        }
        i++;
    }
    return (-1);
}


/*
 * Return 1 if the username is unique in all active_clients, otherwise return 0
 */
int validate_username( char * username, struct client * active_clients )
{
    /* empty string is not valid username */
    if ( !strcmp( username, "" ) )
    {
        return (0);
    }

    struct client * curr_client = active_clients;
    while ( curr_client != NULL )
    {
        if ( !strcmp( curr_client->username, username ) )
        {
            /* if two usernames are identical */
            printf( "%s is not a unique username\n", username );
            return (0);
        }
        curr_client = curr_client->next;
    }
    return (1);
}


/*
 *  Read input from the client p and store the input to the inbuf of the client.
 *  Return -1 if the client p is disconnected. Otherwise, return 0;
 */

int read_client_input( struct client * p )
{
    int inbuf   = p->in_ptr - p->inbuf;
    int room    = sizeof(p->inbuf) - inbuf;

    int nbytes;
    nbytes = read( p->fd, p->in_ptr, room );

    if ( nbytes <= 0 )
    {
        /* if read() return 0 byte, then the socket is closed */
        return (-1);
    }

    p->inbuf[nbytes] = '\0';
    printf( "[%d] Read %d bytes\n", p->fd, nbytes );
    inbuf += nbytes;
    while ( find_network_newline( p->inbuf, inbuf ) == -1 )
    {
        printf( "Can find network newline, continue to read\n" );
        int new_read_bytes = read( p->fd, p->in_ptr + nbytes, room - nbytes );
        if ( new_read_bytes <= 0 )
        {
            return (-1);
        }
        nbytes += new_read_bytes;

        p->inbuf[nbytes]    = '\0';
        inbuf           = nbytes;
    }

    p->inbuf[nbytes - 2]    = '\0';
    p->in_ptr       = p->inbuf + inbuf;
    room = sizeof(p->inbuf) - inbuf;

    printf( "[%d] Message read : %d bytes\n", p->fd, inbuf );

    return (0);
}


/*
 * Reset the inbuf of client p.
 */
void reset_client_buf( struct client * p )
{
    memset( p->inbuf, '\0', sizeof(p->inbuf) );
    p->in_ptr = p->inbuf;
}


int main( int argc, char **argv )
{
    int         clientfd, maxfd, nready;
    struct client       *p;
    struct sockaddr_in  q;
    fd_set          rset;

    /*
     * If the server writes to a socket that has been closed, the SIGPIPE
     * signal is sent and the process is terminated. To prevent the server
     * from terminating, ignore the SIGPIPE signal.
     */
    struct sigaction sa;
    sa.sa_handler   = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset( &sa.sa_mask );
    if ( sigaction( SIGPIPE, &sa, NULL ) == -1 )
    {
        perror( "sigaction" );
        exit( 1 );
    }

    /* A list of active clients (who have already entered their names). */
    struct client *active_clients = NULL;

    /*
     * A list of clients who have not yet entered their names. This list is
     * kept separate from the list of active clients, because until a client
     * has entered their name, they should not issue commands or
     * or receive announcements.
     */
    struct client *new_clients = NULL;

    struct sockaddr_in  *server     = init_server_addr( PORT );
    int         listenfd    = set_up_server_socket( server, LISTEN_SIZE );
    free( server );

    /*
     * Initialize allset and add listenfd to the set of file descriptors
     * passed into select
     */
    FD_ZERO( &allset );
    FD_SET( listenfd, &allset );

    /* maxfd identifies how far into the set to search */
    maxfd = listenfd;

    while ( 1 )
    {
        /* make a copy of the set before we pass it into select */
        rset = allset;

        nready = select( maxfd + 1, &rset, NULL, NULL, NULL );
        if ( nready == -1 )
        {
            perror( "select" );
            exit( 1 );
        }
        else if ( nready == 0 )
        {
            continue;
        }

        /* check if a new client is connecting */
        if ( FD_ISSET( listenfd, &rset ) )
        {
            printf( "A new client is connecting\n" );
            clientfd = accept_connection( listenfd, &q );

            FD_SET( clientfd, &allset );
            if ( clientfd > maxfd )
            {
                maxfd = clientfd;
            }
            printf( "Connection from %s\n", inet_ntoa( q.sin_addr ) );
            add_client( &new_clients, clientfd, q.sin_addr );
            char *greeting = WELCOME_MSG;
            if ( write( clientfd, greeting, strlen( greeting ) ) == -1 )
            {
                fprintf( stderr,
                         "Write to client %s failed\n", inet_ntoa( q.sin_addr ) );
                remove_client( &new_clients, clientfd );
            }
        }

        /*
         * Check which other socket descriptors have something ready to read.
         * The reason we iterate over the rset descriptors at the top level and
         * search through the two lists of clients each time is that it is
         * possible that a client will be removed in the middle of one of the
         * operations. This is also why we call break after handling the input.
         * If a client has been removed, the loop variables may no longer be
         * valid.
         */
        int cur_fd, handled;
        /* we iterate through all the clients connected to the server */
        for ( cur_fd = 0; cur_fd <= maxfd; cur_fd++ )
        {
            /*
             * if there is something to read in this fd
             * we don't know whether this fd belongs to one of new_clients or one of active_clients
             */

            if ( FD_ISSET( cur_fd, &rset ) )
            {
                /* handled == 1 iff we read username from the client */
                handled = 0;

                /* Check if any new clients are entering their names */
                for ( p = new_clients; p != NULL; p = p->next )
                {
                    if ( cur_fd == p->fd )
                    {
                        /*
                         * handle input from a new client who has not yet
                         * entered an acceptable name
                         */

                        /* Receive messages */
                        if ( read_client_input( p ) == -1 )
                        {
                            /*  Remove the client sincethe client is diconnected. */
                            int remove_client_fd = p->fd;
                            remove_client( &new_clients, p->fd );

                            /* notify all active clients that one client in the new_clients list */
                            char disconnect_mes[BUF_SIZE];
                            sprintf( disconnect_mes, "New client [%d] disconnected.\r\n", remove_client_fd );
                            announce( active_clients, disconnect_mes );

                            /* show in server that a client in the new_clients list is disconnected */
                            printf( "%s", disconnect_mes );

                            break;
                        }

                        printf( "New client [%d] : %s\r\n", p->fd, p->inbuf );

                        /* before we validate the username, we can't copy it into the username field */
                        if ( validate_username( p->inbuf, active_clients ) )
                        {
                            /*
                             * if the username is unique among all active clients
                             * remove the client from the new_clients and add it to the active_clients
                             */
                            strcpy( p->username, p->inbuf );

                            activate_client( p, &active_clients, &new_clients );

                            /* notify all active clients that we have a new client join in */
                            char join_in_message[2 * BUF_SIZE];
                            strcpy( join_in_message, p->username );
                            strcat( join_in_message, " has just joined.\r\n" );

                            announce( active_clients, join_in_message );

                            /* also show in server that a client has joined in */
                            printf( "%s", join_in_message );

                            /* clean what is previously written in inbuf */
                            reset_client_buf( p );

                            /* set handled to be true after the client enter a valid username */
                            handled = 1;
                            break;
                        }
                        else
                        {
                            /* clean what is previously written in inbuf */
                            reset_client_buf( p );

                            /* the client did not enter a unique username to register */
                            if ( write( p->fd, USERNAME_MSG, strlen( USERNAME_MSG ) ) != strlen( USERNAME_MSG ) )
                            {
                                int remove_client_fd = p->fd;
                                printf( "fail to write message to ask the client [%d] to type a new username\n", remove_client_fd );
                                remove_client( &new_clients, p->fd );

                                /* notify all active clients that one client in the new_clients list is disconnected */
                                char disconnect_mes[BUF_SIZE];
                                sprintf( disconnect_mes, "New client [%d] disconnected.\r\n", remove_client_fd );
                                announce( active_clients, disconnect_mes );

                                /* show in server that a client in the new_clients list is disconnected */
                                printf( "%s", disconnect_mes );

                            }
                        }
                    }
                }

                /* Check if any active client is entering command */
                if ( !handled )
                {
                    /*
                     * if not handled by the previous forloop, this means that no client in new_clients in entering username
                     * Check if this socket descriptor is an active client
                     */
                    for ( p = active_clients; p != NULL; p = p->next )
                    {
                        if ( cur_fd == p->fd )
                        {
                            /* handle input from an active client */

                            /* Receive messages */
                            if ( read_client_input( p ) == -1 )
                            {
                                char disconnect_mes[2 * BUF_SIZE];
                                sprintf( disconnect_mes, "%s is disconnected.\r\n", p->username );
                                /*  Remove the client since from the list of active_clients. */
                                remove_client( &active_clients, p->fd );

                                /* notify all active clients that one client in the new_clients list */
                                announce( active_clients, disconnect_mes );

                                /* show message in the server */
                                printf( "%s", disconnect_mes );

                                break;
                            }

                            printf( "%s : %s\r\n", p->username, p->inbuf );

                            if ( strstr( p->inbuf, QUIT_MSG ) == p->inbuf && strlen( p->inbuf ) == strlen( QUIT_MSG ) )
                            {
                                /* if the client enter command : quit */
                                char disconnect_mes[2 * BUF_SIZE];
                                sprintf( disconnect_mes, "%s is disconnected.\r\n", p->username );
                                remove_client( &active_clients, p->fd );

                                /* notify all active clients that one client in the new_clients list */
                                announce( active_clients, disconnect_mes );

                                /* show message in the server */
                                printf( "%s", disconnect_mes );
                            }
                            else if ( strstr( p->inbuf, FOLLOW_MSG ) == p->inbuf && p->inbuf[strlen( FOLLOW_MSG )] == ' ' && strlen( p->inbuf ) > strlen( FOLLOW_MSG ) + 1 )
                            {
                                int follow_successful = 0;
                                char    follow_fail_msg[2 * BUF_SIZE];
                                /* if the client enter command : follow + one_white_space */
                                char * to_follow = p->inbuf + strlen( FOLLOW_MSG ) + 1;

                                struct client * to_follow_ptr = is_active( to_follow, active_clients );

                                if ( to_follow_ptr == NULL )
                                {
                                    follow_successful = NOT_ACTIVE_USR_ERR;
                                    sprintf( follow_fail_msg, "%s can't follow %s since the client is not active.\r\n", p->username, p->inbuf + strlen( FOLLOW_MSG ) + 1 );
                                }
                                else if ( !not_exist( to_follow, p->following ) )
                                {
                                    follow_successful = ALREADY_FOLLOWED;
                                    sprintf( follow_fail_msg, "%s can't follow %s since the client is already followed by.\r\n", p->username, p->inbuf + strlen( FOLLOW_MSG ) + 1 );
                                }
                                else if ( !enough_space_for_clients( p->following ) || !enough_space_for_clients( to_follow_ptr->followers ) )
                                {
                                    follow_successful = NO_SPACE_ERR;
                                    sprintf( follow_fail_msg, "%s can't follow %s since either of follower/following client has no space for new following/follower.\r\r", p->username, p->inbuf + strlen( FOLLOW_MSG ) + 1 );
                                }

                                if ( follow_successful == 0 )
                                {
                                    follow_user( to_follow_ptr, p );
                                    follow_successful = 1;

                                    /* send notification in the server */
                                    printf( "%s starts to follow: %s\r\n", p->username, to_follow_ptr->username );
                                    printf( "%s has a new follower: %s\r\n", to_follow_ptr->username, p->username );

                                }

                                if ( follow_successful != 1 )
                                {
                                    /* Client cannot issue follow command */

                                    /* show in server */
                                    printf( "%s", follow_fail_msg );

                                    if ( write( p->fd, follow_fail_msg, strlen( follow_fail_msg ) ) == -1 )
                                    {
                                        remove_client( &active_clients, p->fd );
                                    }
                                }
                            }
                            else if ( strstr( p->inbuf, UNFOLLOW_MSG ) == p->inbuf &&
                                      p->inbuf[strlen( UNFOLLOW_MSG )] == ' ' &&
                                      strlen( p->inbuf ) > strlen( UNFOLLOW_MSG ) + 1 )
                            {
                                int unfollow_successful = 0;
                                char    unfollow_fail_msg[2 * BUF_SIZE];
                                /* if the client enter command : unfollow + one_white_space */
                                char * to_unfollow = p->inbuf + strlen( UNFOLLOW_MSG ) + 1;

                                struct client * to_unfollow_ptr = is_active( to_unfollow, active_clients );

                                if ( to_unfollow_ptr == NULL )
                                {
                                    unfollow_successful = NOT_ACTIVE_USR_ERR;
                                    sprintf( unfollow_fail_msg, "%s can't unfollow %s because the client is not active.\r\n", p->username, p->inbuf + strlen( UNFOLLOW_MSG ) + 1 );
                                }
                                else if ( not_exist( to_unfollow, p->following ) || not_exist( p->username, to_unfollow_ptr->followers ) )
                                {
                                    unfollow_successful = NOT_FOLLOWED;
                                    sprintf( unfollow_fail_msg, "%s can't unfollow %s because the client is not in following list.\r\n", p->username, p->inbuf + strlen( UNFOLLOW_MSG ) + 1 );
                                }

                                if ( unfollow_successful == 0 )
                                {
                                    printf( "To unfollow: %s\n", to_unfollow_ptr->username );
                                    unfollow_user( to_unfollow_ptr, p );

                                    unfollow_successful = 1;
                                    printf( "%s unfollows %s.\r\n", p->username, to_unfollow_ptr->username );
                                    printf( "%s no longer has %s as a follower.\r\n", to_unfollow_ptr->username, p->username );

                                }

                                if ( unfollow_successful != 1 )
                                {
                                    /* show in server */
                                    printf( "%s", unfollow_fail_msg );

                                    if ( write( p->fd, unfollow_fail_msg, strlen( unfollow_fail_msg ) ) == -1 )
                                    {
                                        remove_client( &active_clients, p->fd );
                                    }
                                }
                            }
                            else if ( strstr( p->inbuf, SEND_MSG ) == p->inbuf && p->inbuf[strlen( SEND_MSG )] == ' ' )
                            {
                                /* we can assume that the message must be in 1 ~ 140 characters */

                                char    * to_send_msg = p->inbuf + strlen( SEND_MSG ) + 1;
                                int msg_dest;
                                msg_dest = enough_space_for_msg( p );
                                if ( msg_dest != -1 )
                                {
                                    send_message( &(active_clients), p, to_send_msg );
                                    printf( "%s: send %s\n", p->username, to_send_msg );
                                }
                                else
                                {
                                    char send_fail_msg[2 * BUF_SIZE];
                                    sprintf( send_fail_msg, "Fail to send : %s does not have space to send message.\r\n", p->username );

                                    if ( write( p->fd, send_fail_msg, strlen( send_fail_msg ) ) == -1 )
                                    {
                                        remove_client( &active_clients, p->fd );
                                    }
                                    printf( "%s", send_fail_msg );
                                }
                            }
                            else if ( strstr( p->inbuf, SHOW_MSG ) && strlen( p->inbuf ) == strlen( SHOW_MSG ) )
                            {
                                show_prev_msg( &active_clients, p );
                            }
                            else
                            {
                                /* the command is invalid */
                                printf( "%s", INVALID_COMMAND );
                                if ( write( p->fd, INVALID_COMMAND, strlen( INVALID_COMMAND ) ) == -1 )
                                {
                                    remove_client( &active_clients, p->fd );
                                }
                            }

                            reset_client_buf( p );

                            break;
                        }
                    }
                }
            }
        }
    }
    return (0);
}




/*
* Check whether user with username to_follow_username is an active client.
* Return a struct client pointer to the client with username to_follow_username
* if such user exists. Otherwise, return NULL.
*/
struct client * is_active( char * to_follow_username, struct client * active_clients )
{
    struct client * curr_client = active_clients;
    while ( curr_client != NULL && strcmp( curr_client->username, to_follow_username ) != 0 )
    {
        curr_client = curr_client->next;
    }
    return (curr_client);
}

/*
* Check if the client with username username in the client array list_of_clients.
* Return 0 if in the array, return 1 if not.
*/
int not_exist( char * username, struct client ** list_of_clients )
{
    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        if ( list_of_clients[i] != NULL && strcmp( list_of_clients[i]->username, username ) == 0 )
        {
            return (0);
        }
    }
    return (1);
}

/*
* Return 1 if there is empty slot in the client array list_of_clients.
* Otherwise, return 0.
*/
int enough_space_for_clients( struct client ** list_of_clients )
{
    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        if ( list_of_clients[i] == NULL )
        {
            return (1);
        }
    }
    return (0);
}

/*
* client follow follows client followed.
* add follow into followed's followers list
* add followed into follow's following list
*/
void follow_user( struct client * followed, struct client * follow )
{
    /* add followed to follow's following */
    int index = 0;
    while ( index < FOLLOW_LIMIT )
    {
        if ( follow->following[index] == NULL )
        {
            follow->following[index] = followed;
            break;
        }
        index++;
    }

    /* add follow to followed's follower */
    index = 0;
    while ( index < FOLLOW_LIMIT )
    {
        if ( followed->followers[index] == NULL )
        {
            followed->followers[index] = follow;
            break;
        }
        index++;
    }
}

/*
* client follow unfollows client unfollowed
* Remove follow in unfollowed's followers list
* Remove unfollowed in follow's following list
*/
void unfollow_user( struct client * unfollowed, struct client * follow )
{
    /* delete from following */
    delete_from_list_of_clients( follow->following, unfollowed->fd );

    /* delete from followers */
    delete_from_list_of_clients( unfollowed->followers, follow->fd );
}

/*
* Return the empty slot index in client's message list
* if there is an empty slot for new message.
* Otherwise, return -1
*/
int enough_space_for_msg( struct client * client )
{
    for ( int i = 0; i < MSG_LIMIT; i++ )
    {
        if ( strlen( client->message[i] ) == 0 )
        {
            return (i);
        }
    }
    return (-1);
}

/*
* Send message msg_to_send by client sender.
*/
void send_message( struct client ** active_clients, struct client *sender, char * msg_to_send )
{
    /* save the message */
    for ( int j = 0; j < MSG_LIMIT; j++ )
    {
        if ( strlen( sender->message[j] ) == 0 )
        {
            strcpy( sender->message[j], msg_to_send );
            break;
        }
    }

    struct client   * curr_fan;
    char        msg[2 * BUF_SIZE];
    sprintf( msg, "%s sends: %s\r\n", sender->username, msg_to_send );

    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        curr_fan = sender->followers[i];
        if ( curr_fan != NULL )
        {
            if ( write( curr_fan->fd, msg, strlen( msg ) ) == -1 )
            {
                /* failed to write to the fan */
                remove_client( active_clients, curr_fan->fd );
            }
        }
    }
}

/*
* Show all messages sent by all clients in show_to's followering list
*/
void show_prev_msg( struct client **active_clients, struct client *show_to )
{
    struct client * curr_following;
    for ( int i = 0; i < FOLLOW_LIMIT; i++ )
    {
        curr_following = show_to->following[i];
        if ( curr_following != NULL )
        {
            show_sent_msg( active_clients, curr_following, show_to );
        }
    }
}

/*
* Show client curr_client 's previously sent messages to client show_to.
*/
void show_sent_msg( struct client **active_clients, struct client *curr_client, struct client *show_to )
{
    for ( int i = 0; i < MSG_LIMIT; i++ )
    {
        if ( strlen( curr_client->message[i] ) != 0 )
        {
            char to_show_msg[2 * BUF_SIZE];
            sprintf( to_show_msg, "%s wrote message : %s\r\n", curr_client->username, curr_client->message[i] );
            if ( write( show_to->fd, to_show_msg, strlen( to_show_msg ) ) == -1 )
            {
                /* fail to write */
                remove_client( active_clients, show_to->fd );
            }
        }
    }
}