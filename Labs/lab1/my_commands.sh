#!/usr/bin/env bash
gcc -Wall -std=gnu99 -g -o echo_arg echo_arg.c
./echo_arg csc209 > ./echo_out.txt

gcc -Wall -std=gnu99 -g -o echo_stdin echo_stdin.c
./echo_stdin < ./echo_stdin.c 

gcc -Wall -std=gnu99 -g -o count count.c
./count 210 | wc -m

gcc -Wall -std=gnu99 -g -o echo_stdin echo_stdin.c
ls -S | ./echo_stdin 
