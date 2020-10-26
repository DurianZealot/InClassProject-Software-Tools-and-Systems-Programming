#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
    char *update_board[num_rows * num_cols];
    for (int i = 0; i < num_rows * num_cols; i++){
        update_board[i] = "s";
    }

    for (int i = 0; i < num_rows * num_cols; i++){
        int row_pos = i / num_cols;
        
        int col_pos = i % num_cols;

        if (row_pos != 0 && row_pos != num_rows - 1 && col_pos != 0 && col_pos != num_cols - 1){
            // printf("index is %d, with row postion is %d, column position is %d\n", i, row_pos, col_pos);
            // if it not at the edge
            // all neighbors (row - 1, col) (row + 1, col), (row, col + 1), (row, col - 1), (row - 1, col - 1), (row - 1, col + 1), (row + 1, col -1), (row + 1, col +1)
            int num_alive_cells = board[(row_pos - 1) * num_cols + col_pos] + 
                                  board[(row_pos + 1) * num_cols + col_pos] +
                                  board[(row_pos) * num_cols + col_pos + 1] +
                                  board[(row_pos) * num_cols + col_pos - 1] +
                                  board[(row_pos - 1) * num_cols + col_pos - 1] +
                                  board[(row_pos - 1) * num_cols + col_pos + 1] +
                                  board[(row_pos + 1) * num_cols + col_pos - 1] +
                                  board[(row_pos + 1) * num_cols + col_pos + 1];

            // printf("there are %d alive cells.\n", num_alive_cells);
            // printf("the value inside %d in board is %d\n", i, board[i]);
            if(board[i] == 1 && (num_alive_cells < 2 || num_alive_cells > 3)){
                update_board[i] = "0";
                // printf("%d should change from 1 to 0\n", board[i]);
            }
            if(board[i] == 0 && (num_alive_cells == 2 || num_alive_cells == 3)){
                update_board[i] = "1";
                // printf("%d should change from 0 to 1\n", board[i]);
            }
        }

    }

    for(int i = 0; i < num_cols * num_rows; i++){
        if(strcmp(update_board[i], "0") == 0){
            board[i] = 0;
        }
        if(strcmp(update_board[i], "1") == 0){
            board[i] = 1;
        }
    }
}
