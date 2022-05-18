// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#ifndef JIT_LOADER_H
#define JIT_LOADER_H

#include "IR.h"


typedef enum LoadingResult
{
    LOADING_SUCCESS   = 1,
    LOADING_FAILURE   = 2,
    LOADING_REJECTION = 3
} LoadingResult;

LoadingResult load_IR(IR *restrict *const restrict import_IR,
                      const char *const restrict IR_file_path, const char *const restrict bytecode_file_path);

#endif // JIT_LOADER_H
