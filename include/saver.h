// 
// Created by MIPT student Nikita Dzer on 17.05.2022.
// 

#ifndef JIT_SAVER_H
#define JIT_SAVER_H

#include "IR.h"


typedef enum SavingResult
{
    SAVING_SUCCESS = 1,
    SAVING_FAILURE = 2
} SavingResult;

SavingResult save_IR(IR *const restrict IR, const char *const restrict IR_file_path);


#endif // JIT_SAVER_H
