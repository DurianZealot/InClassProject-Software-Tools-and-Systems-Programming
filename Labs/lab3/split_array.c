#include <stdio.h>
#include <stdlib.h>

/* Return a pointer to an array of two dynamically allocated arrays of ints.
   The first array contains the elements of the input array s that are
   at even indices.  The second array contains the elements of the input
   array s that are at odd indices.

   Do not allocate any more memory than necessary.
*/
int **split_array(const int *s, int length) {
    // decide how many numbers are at even indices and odd indices
    int even_num, odd_num;
    if (length % 2 == 0){
        even_num = length / 2;
        odd_num = length / 2;
    }
    else{
        even_num = (length + 1) / 2;
        odd_num = (length - 1) / 2;
    }
    int *even_indices = malloc(sizeof(int) * even_num);
    int *odd_indices = malloc(sizeof(int) * odd_num);
    int even = 0;
    int odd = 0;
    for(int index = 0; index < length; index++){
        if (index %2 == 0){
            even_indices[even] = s[index];
            even += 1;
        }
        else{
            odd_indices[odd] = s[index];
            odd += 1;
        }
    }
    int **split_array = malloc(sizeof(int*)*2);
    split_array[0] = even_indices;
    split_array[1] = odd_indices;

    return split_array;
}

/* Return a pointer to an array of ints with size elements.
   - strs is an array of strings where each element is the string
     representation of an integer.
   - size is the size of the array
 */

int *build_array(char **strs, int size) {
    int *arr = malloc(sizeof(int)*size);
    for(int i = 0; i < size; i++){
        // strtol takes pointer as an input 
        arr[i] = strtol(strs[i+1], NULL, 10);
    }
    return arr;
}


int main(int argc, char **argv) {
    /* Replace the comments in the next two lines with the appropriate
       arguments.  Do not add any additional lines of code to the main
       function or make other changes.
     */
    // argv[0] is not out user input
    int *full_array = build_array(argv, argc - 1);
    int **result = split_array(full_array, argc - 1);
    printf("Original array:\n");
    for (int i = 0; i < argc - 1; i++) {
        printf("%d ", full_array[i]);
    }
    printf("\n");

    printf("result[0]:\n");
    for (int i = 0; i < argc / 2; i++) {
        printf("%d ", result[0][i]);
    }
    printf("\n");

    printf("result[1]:\n");
    for (int i = 0; i < (argc - 1) / 2; i++) {
        printf("%d ", result[1][i]);
    }
    printf("\n");
    free(full_array);
    free(result[0]);
    free(result[1]);
    free(result);

    // check memory leak by valgrind ./split_array 1 2 3 4
    return 0;
}
