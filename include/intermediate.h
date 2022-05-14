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
    
    NO_REGISTRY        = 16,
    UNDEFINED_REGISTRY = 17,
} IntermediateRegistry;

typedef enum IntermediateOpcode
{
    O0_PUSH   = 1,
    O0_POP    = 2,
    O0_ADD    = 3,
    O0_MUL    = 4,
    O0_IN     = 5,
    O0_PRINTF = 6,
    O0_JMP    = 7,
    O0_JE     = 8,
    O0_JA     = 9,
    O0_HLT    = 10,
    O0_CALL   = 11,
    O0_RET    = 12,
    O0_SQRT   = 13,
    O0_SUB    = 14,
    O0_DIV    = 15,
    O0_PIX    = 16,
    O0_SHOW   = 17,
    
    UNDEFINED_OPCODE = 0,
} IntermediateOpcode;

typedef enum IntermediateArgumentType
{
    TYPE_REGISTRY     = 1,
    TYPE_INTEGER      = 2,
    TYPE_DOUBLE       = 3,
    TYPE_MEM_REGISTRY = 4,
    TYPE_MEM_RELATIVE = 5,
    TYPE_MEM_OFFSET   = 6,
    TYPE_REFERENCE    = 7,
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
    
    IntermediateOpcode opcode;
    unsigned char      is_compiled;
} Intermediate;

#endif // JIT_INTERMEDIATE_H
