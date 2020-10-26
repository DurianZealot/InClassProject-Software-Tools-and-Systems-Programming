#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    int num_rows = strtol(argv[1], NULL, 10);
    int num_cols = strtol(argv[2], NULL, 10);
    int num_states = strtol(argv[3], NULL, 10);
    int *board = malloc(sizeof(int)*num_cols*num_rows);
    int count = 0;
    // Read the board inital state from stdin
    for(int i = 0; i < num_rows; i++){
        for (int j = 0; j < num_cols; j++)
        {
            scanf("%d", &board[count]);
            count += 1;
        }
    
    }

    // for(int i = 0; i < num_rows * num_cols; i++){
    //     printf("%d ", board[i]);
    // }
    print_state(board, num_rows, num_cols);

    for(int i = 0; i < num_states - 1; i++){
        update_state(board, num_rows, num_cols);
        print_state(board, num_rows, num_cols);
    }

    return 0;
}
