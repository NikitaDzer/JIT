// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#ifndef JIT_EXECUTER_H
#define JIT_EXECUTER_H

typedef enum ExecutionResult
{
    EXECUTION_SUCCESS = 0,
    EXECUTION_FAILURE = 1,
} ExecutionResult;

ExecutionResult execute_bincode(const char *const restrict bincode);

#endif // JIT_EXECUTER_H
