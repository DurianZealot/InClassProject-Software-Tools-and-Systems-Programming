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
    int error = 0;
    scanf("%10s", string);
    while((scanf("%d", &digit) == 1)){
        if (phone(string, digit) == 1){
            error = 1;
        }
    }
    return error;
}