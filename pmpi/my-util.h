#ifndef MY_UTIL_H
#define MY_UTIL_H

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


void print_args(int* argc, char*** argv, const char* func_name) {
    int i;
    
    if (func_name != NULL) 
	printf("%s:\n", func_name); 
    
    printf("    argc == %d\n", *argc);
    for (i = 0; i< *argc; i++) {
	printf("argv[%d] = %s\n", i, (*argv)[i]);
    }
}

#endif
