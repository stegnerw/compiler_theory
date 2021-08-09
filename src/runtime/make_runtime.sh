#!/bin/bash
gcc -c -Wall -Werror -fpic runtime.c
gcc -shared -o runtime.so runtime.o
