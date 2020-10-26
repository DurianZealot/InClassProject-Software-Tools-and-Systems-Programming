#include <stdio.h>
#include <string.h>
int phone(char *string, int digit){
    if(digit < -1 || digit > 9){
        printf("ERROR\n");
        return 1;
    }
    if (digit == -1){
        // string is already a pointer to the given string
        printf("%s\n", string);
    }
    else{
        printf("%c\n", string[digit]);
    }
    return 0;
}
int main(){
    char string[11];
    int digit;

    // string is already a pointer that points to the first element in string
    // so we don't need to use &string
    // but since digit is not a pointer
    // we need to use &digit to assign value into it
    scanf("%10s", string);
    scanf("%d", &digit);

    return phone(string, digit);
}