#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }
    
    // Read input of position
    char *posChar = argv[1];
    int pos = strtol(posChar, NULL, 10);
    
    // Set up array to store the result
    int curr_num;
    int *summmary_array = malloc(sizeof(int)*10);
    for(int i = 0; i < 10; i++){
        summmary_array[i] = 0;
    }
    if(argc == 2){
        // there is no input file, so we read from standard input
        
        while (scanf("%d", &curr_num) == 1){
            add_to_tally(curr_num, pos, summmary_array);
        }

    }
    else{
        // there is an input file
        FILE *inputFile;
        char *fileName = argv[2];
        inputFile = fopen(fileName, "r");

        // if input file does not exist
        if (inputFile == NULL){
            return 1;
        }

        // Read input from files
        char currIntChar[255];
        while (fscanf(inputFile, "%s\n", currIntChar)!=EOF){
            curr_num = strtol(currIntChar, NULL, 10);
            add_to_tally(curr_num, pos, summmary_array);
        }
    }

    for(int i = 0; i < 10; i++){
            printf("%ds: %d\n", i, summmary_array[i]);
        }
    return 0;
}
