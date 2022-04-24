// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_H
#define JIT_H

typedef enum JITResult
{
    JIT_SUCCESS = 0,
    JIT_FAILURE = 1,
} JITResult;

JITResult JIT(const char *const restrict bytecode_path, const );

#endif // JIT_H
