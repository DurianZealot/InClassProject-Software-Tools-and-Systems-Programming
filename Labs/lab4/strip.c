#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    Complete this program by writing the function strip_q_marks that takes
    a single string and returns an integer.

    The function should modify the string to remove any trailing question marks
    and return the number of question marks that were removed.

    Examples
    original sentence       modified sentence       return value
    =============================================================
    "Hello? World???"       "Hello? World"          3
    "What..?"               "What.."                1
    "Apples?..?"            "Apples?.."             1
    "Coffee"                "Coffee"                0
    "Who?What?Where?"       "Who?What?Where"        1
*/

// Write the function strip_q_marks here
int strip_q_marks(char *origin_sentence){
    int mark_count = 0;
    int i = strlen(origin_sentence)-1;
    while(i >= 0){
        char s[2];
        strncpy(s, &origin_sentence[i], 1);
        s[1] = '\0';

        if(strcmp(s,"?") == 0){
            mark_count += 1;
        }
        else{
            break; 
        }
        i = i - 1;
    }
    for (int j = i+1; j < strlen(origin_sentence); j++){
        origin_sentence[j] = '\0';
    }
    return mark_count;
}

int main(int argc, char **argv) {
    // Do not change this main function.
    if(argc != 2) {
        fprintf(stderr, "Usage: strip message\n");
        exit(1);
    }
    int result = strip_q_marks(argv[1]);
    printf("%s %d", argv[1], result);
    return 0;
}
