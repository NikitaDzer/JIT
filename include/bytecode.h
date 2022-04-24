// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_BYTECODE_H
#define JIT_BYTECODE_H

typedef struct Bytecode
{
    const char *const restrict buffer;
    const unsigned long long buffer_size;
} Bytecode;

#endif // JIT_BYTECODE_H
