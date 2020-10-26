#include <stdio.h>

#include <stdlib.h>

#include <assert.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/wait.h>

#include "point.h"

#include "serial_closest.h"

#include "utilities_closest.h"


// define wrapper for fork
int fork_wrapper() {
    int res;
    if ((res = fork()) == -1) {
        perror("fork");
        exit(1);
    }
    return res;
}

// define wrapper for pipe
void pipe_wrapper(int * fd) {
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }
}

// a helper function to get the minimum distance for points in the middle
double get_mid_min_dis(struct Point * p, int size, double prev_min_dis) {
    int mid = size / 2;
    struct Point mid_point = p[mid];

    // Build an array  strip[] that contains points close (closer than d) 
    // the line passing through the middle point

    struct Point * strip = malloc(sizeof(struct Point) * size);
    if (strip == NULL) {
        perror("malloc");
        exit(1);
    }

    int j = 0;
    for (int i = 0; i < size; i++) {
        if (abs(p[i].x - mid_point.x) < prev_min_dis) {
            strip[j] = p[i], j++;
        }
    }

    // Find the closest points in strip. Return the minimum of prev_min_dis
    // and closest distance in strip[].
    double new_min = min(prev_min_dis, strip_closest(strip, j, prev_min_dis));
    free(strip);

    return new_min;
}

/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point * p, int n, int pdmax, int * pcount) {
    // base case
    if (n < 2) {
        return 0.0;
    }
    if (n < 4 || pdmax == 0) {
        return closest_serial(p, n);
    }

    // recursive
    else {
        int left_size;
        int right_size;

        if (n % 2 == 0) {
            // when n is even
            left_size = n / 2;
            right_size = n / 2;
        } else {
            // when n is odd
            left_size = (n - 1) / 2;
            right_size = n - left_size;
        }

        // make two pipe
        int all_pipes[2][2];
        // declare a distance
        double min_distance;

        // pipe for left child
        pipe_wrapper(all_pipes[0]);

        // fork left child
        int lchild = fork_wrapper();

        if (lchild == 0) {
            // left child block

            // close read end
            if (close(all_pipes[0][0]) == -1) {
                perror("read at left child");
                exit(1);
            }

            min_distance = closest_parallel(p, left_size, pdmax - 1, pcount);
            if (write(all_pipes[0][1], & min_distance, sizeof(double)) != sizeof(double)) {
                perror("fail to write min_distance at left child");
                exit(1);
            }
            // close write end
            if (close(all_pipes[0][1]) == -1) {
                perror("write at left child");
                exit(1);
            }

            // children do not share memory with  parent so pcount is 0 and left child will get its own pcount
            exit( * pcount);

        } else {
            // parent block
            // close pipes (write)
            if (close(all_pipes[0][1]) == -1) {
                perror("write at parent for left child");
                exit(1);
            }

            // pipe for right child
            pipe_wrapper(all_pipes[1]);

            // fork right child
            int rchild = fork_wrapper();

            if (rchild == 0) {
                // right child block

                // close pipes that are forked by the parent for the left child
                // write end is already closed before forking right child
                // so no need to close the write end here

                if (close(all_pipes[0][0]) == -1) {
                    perror("right child close left child's read");
                    exit(1);
                }

                // close read end
                if (close(all_pipes[1][0]) == -1) {
                    perror("read at right child");
                    exit(1);
                }

                min_distance = closest_parallel(p + left_size, right_size, pdmax - 1, pcount);
                if (write(all_pipes[1][1], & min_distance, sizeof(double)) != sizeof(double)) {
                    perror("fail to write min_distance at right child");
                    exit(1);
                }
                // close write end
                if (close(all_pipes[1][1]) == -1) {
                    perror("write at right child");
                }

                exit( * pcount);
            } else {
                // parent block
                // close write end
                if (close(all_pipes[1][1]) == -1) {
                    perror("write at parent for right child");
                    exit(1);
                }

                // wait two sub children to finish here

                // first read the input data first
                double distances[2];
                pid_t pid;
                int status;
                for (int i = 0; i < 2; i++) {
                    if ((pid = wait( & status)) == -1) {
                        perror("wait");
                        exit(1);
                    }

                    if (WIFEXITED(status)) {
                        * pcount += WEXITSTATUS(status) + 1;
                    }
                }

                for (int j = 0; j < 2; j++) {
                    // dup2 from read to stdin
                    dup2(all_pipes[j][0], fileno(stdin));

                    if (read(fileno(stdin), & (distances[j]), sizeof(double)) < 0) {
                        perror("fail to collect distance");
                        exit(1);
                    }

                    if (close(all_pipes[j][0]) == -1) {
                        perror("parent close read for child");
                        exit(1);
                    }
                }

                // finally  compare the result 
                if (distances[1] > distances[0]) {
                    return min(distances[0], get_mid_min_dis(p, n, distances[0]));
                } else {
                    return min(distances[1], get_mid_min_dis(p, n, distances[1]));
                }
            }
        }
    }
    return 0.0;
}