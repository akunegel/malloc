#!/bin/sh

gcc main.c ./libft_malloc.so -o temporary
./temporary
rm temporary 
