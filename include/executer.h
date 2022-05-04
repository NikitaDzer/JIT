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


ExecutionResult execute_bincode(const unsigned char *const restrict bincode, const unsigned long long bincode_size);

extern inline unsigned char* allocate_bincode(const unsigned long long bincode_size);

extern inline void free_bincode(unsigned char *const restrict bincode);

#endif // JIT_EXECUTER_H
