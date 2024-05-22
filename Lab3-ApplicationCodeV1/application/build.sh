#!/usr/bin/env bash
gcc -c ../protocol/linklayer.c -o linklayer.o 
gcc -w main.c linklayer.o -o main
