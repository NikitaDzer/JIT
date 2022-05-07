// 
// Created by MIPT student Nikita Dzer on 04.05.2022.
// 

#ifndef JIT_INTERMEDIATE_H
#define JIT_INTERMEDIATE_H

typedef enum IntermediateRegistry
{
    RAX = 0,  RBX = 3,
    RCX = 1,  RDX = 2,
    RSP = 4,  RBP = 5,
    RSI = 6,  RDI = 7,
    R8  = 8,  R9  = 9,
    R10 = 10, R11 = 11,
    R12 = 12, R13 = 13,
    R14 = 14, R15 = 15,
    
    UNDEFINED_REGISTRY = 16
} IntermediateRegistry;

typedef enum IntermediateArgumentType
{
    ARG_TYPE_REG       = 1,
    ARG_TYPE_INT       = 2,
    ARG_TYPE_DBL       = 3,
    ARG_TYPE_MEM_REG   = 4,
    ARG_TYPE_MEM_INT   = 5,
    ARG_TYPE_REFERENCE = 6,
} IntermediateArgumentType;

typedef struct IntermediateArgument
{
    union
    {
        unsigned char                 registry;
        long long                     iconstant;
        double                        dconstant;
        unsigned char       *restrict address;
        struct Intermediate *restrict reference;
    };
    
    IntermediateArgumentType type;
} IntermediateArgument;

// Can be optimized to 32-byte size for being more cache-friendly
typedef struct Intermediate
{
    IntermediateArgument argument1;
    IntermediateArgument argument2;
    
    unsigned char opcode;
    unsigned char is_compiled;
} Intermediate;

#endif // JIT_INTERMEDIATE_H
