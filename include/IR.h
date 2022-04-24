// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_IR_H
#define JIT_IR_H

typedef enum IntermediateArgType
{
    ARG_TYPE_REG     = 1,
    ARG_TYPE_IMM_INT = 2,
    ARG_TYPE_IMM_DBL = 3,
    ARG_TYPE_MEM_REG = 4,
    ARG_TYPE_MEM_IMM = 5,
} IntermediateArgType;

typedef struct IntermediateArg
{
    union
    {
        unsigned char        reg;
        long long            constant_i;
        double               constant_d;
        
        unsigned long long   address;
        struct Intermediate *reference;
    };
    
    IntermediateArgType type;
} IntermediateArg;

// Can be optimized to 32-byte size for being more cache-friendly
typedef struct Intermediate
{
    IntermediateArg arg1;
    IntermediateArg arg2;
    
    unsigned char opcode;
} Intermediate;

typedef struct IR
{
    Intermediate *restrict intermediates;
    unsigned long long     n_intermediates;
    const Intermediate *restrict *restrict order;
} IR;

#endif // JIT_IR_H
