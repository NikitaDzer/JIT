// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#ifndef JIT_EXECUTER_H
#define JIT_EXECUTER_H

#include "bincode.h"


typedef enum ExecutionResult
{
    EXECUTION_SUCCESS = 0,
    EXECUTION_FAILURE = 1,
} ExecutionResult;


static const unsigned long long PROCESSOR_RAM_SIZE = 2048 * sizeof(double);
static const unsigned long long EXECUTER_DATA_SIZE = 4096;
static const unsigned long long DATA_SIZE          = PROCESSOR_RAM_SIZE + EXECUTER_DATA_SIZE;


ExecutionResult execute_bincode(const unsigned char *const restrict executable, const unsigned long long executable_size);

Bincode* allocate_bincode(const unsigned long long executable_size);

void free_bincode(Bincode *const restrict bincode);

#endif // JIT_EXECUTER_H
