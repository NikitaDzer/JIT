//
// Created by MIPT student Nikita Dzer on 24.04.2022
//

#include <stdio.h>
#include <stdlib.h>
#include "include/JIT.h"


int main(const int argc, const char *argv[])
{
    /*
    if (argc < 2)
        return EXIT_FAILURE;
    */
    
    JIT("../binary");
    
    return EXIT_SUCCESS;
}
