#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int digits = 0;
    while((num/BASE)!= 0){
        num = num/BASE;
        digits += 1;
    }
    return digits + 1;
}

int get_ith_from_right(int num, int i) {
    int ithDigit = 0;
    while(i > 0){
        num = num / BASE;
        i -= 1;
    }
    ithDigit = num%BASE;
    return ithDigit;
}

int get_ith_from_left(int num, int i) {
    int totalDigits = count_digits(num);
    return get_ith_from_right(num, totalDigits - 1 - i);
    
}

void add_to_tally(int num, int i, int *tally) {
    // get the number at index i 
    int pos_in_tally = get_ith_from_left(num, i);
    // increament the number by 1 at index pos_in_tally in tally
    tally[pos_in_tally] += 1;
}
