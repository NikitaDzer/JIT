//
// Created by MIPT student Nikita Dzer on 24.04.2022
//

#include <stdio.h>
#include <stdlib.h>
#include "include/JIT.h"



int main(const int argc, const char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "The path to the file with Processor bytecode must be passed.\n");
        return EXIT_FAILURE;
    }
    
    JITResult jit_result = JIT(argv[1]);
    if (jit_result == JIT_FAILURE)
    {
        fprintf(stderr, "JIT translation and execution failure.\n");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
