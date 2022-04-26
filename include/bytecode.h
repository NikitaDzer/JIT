// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#ifndef JIT_BYTECODE_H
#define JIT_BYTECODE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct BytecodeHeader
{
    size_t n_instructions;
} BytecodeHeader;

typedef struct BytecodeInstruction
{
    char   opcode;
    bool   is_registry;
    bool   is_RAM;
    double argument;
} BytecodeInstruction;

#endif // JIT_BYTECODE_H
