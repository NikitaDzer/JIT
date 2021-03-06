// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#ifndef JIT_BYTECODE_H
#define JIT_BYTECODE_H

#include <stddef.h>
#include <stdbool.h>

typedef enum BytecodeRegistry
{
    BYTECODE_RAX = 0, BYTECODE_R8  = 8,
    BYTECODE_RBX = 1, BYTECODE_R9  = 9,
    BYTECODE_RCX = 2, BYTECODE_R10 = 10,
    BYTECODE_RDX = 3, BYTECODE_R11 = 11,
    BYTECODE_RDI = 6, BYTECODE_R12 = 12,
    BYTECODE_RSI = 5, BYTECODE_R13 = 13,
    BYTECODE_RBP = 4, BYTECODE_R14 = 14,
    BYTECODE_RSP = 7, BYTECODE_R15 = 15,
} BytecodeRegistry;

typedef enum BytecodeOpcode
{
    BYTECODE_PUSH = 1,
    BYTECODE_POP  = 2,
    BYTECODE_SUM  = 3,
    BYTECODE_MUL  = 4,
    BYTECODE_IN   = 5,
    BYTECODE_OUT  = 6,
    BYTECODE_JMP  = 7,
    BYTECODE_JE   = 8,
    BYTECODE_JA   = 9,
    BYTECODE_HLT  = 10,
    BYTECODE_CALL = 11,
    BYTECODE_RET  = 12,
    BYTECODE_SQRT = 13,
    BYTECODE_SUB  = 14,
    BYTECODE_DIV  = 15,
    BYTECODE_PIX  = 16,
    BYTECODE_SHOW = 17,
} BytecodeOpcode;

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
