#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Write the copy function to perform exactly as strncpy does, with one
   exception: your copy function will guarantee that dest is always
   null-terminated.
   You shoud read the man page to learn how strncpy works.

  NOTE: You must write this function without using any string functions.
  The only function that should depend on string.h is memset.
 */

char *copy(char *dest, const char *src, int capacity) {
    // capacity is the maximum character to copy into the char dest
    // if capacity > strlen(src), just copy the entire src
    // if capacity <= strlen(src), copyt the src for the first (capcity-1) characters in src 
    // terminate with a null terminator
    
    int index = 0;
    while(index < capacity - 1 && src[index] != '\0'){
        dest[index] = src[index];
        index += 1;
    }

    while(index < capacity){
	dest[index] = '\0';
	index ++;
    }
    return dest;
}


int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: copy size src\n");
        exit(1);
    }
    int size = strtol(argv[1], NULL, 10);
    char *src = argv[2];

    char dest[size];
    memset(dest, 'x', size);

    copy(dest, src, size);

    printf("%s\n", dest);
    return 0;
}
