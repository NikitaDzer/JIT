// 
// Created by MIPT student Nikita Dzer on 04.05.2022.
// 

#ifndef JIT_INTERMEDIATE_H
#define JIT_INTERMEDIATE_H

typedef enum IntermediateRegistry
{
    RAX = 0, R8  = 8,
    RBX = 3, R9  = 9,
    RCX = 1, R10 = 10,
    RDX = 2, R11 = 11,
    RSP = 4, R12 = 12,
    RBP = 5, R13 = 13,
    RSI = 6, R14 = 14,
    RDI = 7, R15 = 15,
    
    XMM0 = 0, XMM8  = 8,
    XMM1 = 1, XMM9  = 9,
    XMM2 = 2, XMM10 = 10,
    XMM3 = 3, XMM11 = 11,
    XMM4 = 4, XMM12 = 12,
    XMM5 = 5, XMM13 = 13,
    XMM6 = 6, XMM14 = 14,
    XMM7 = 7, XMM15 = 15,
    
    NO_REGISTRY        = 16,
    UNDEFINED_REGISTRY = 17,
} IntermediateRegistry;

typedef enum IntermediateOpcode
{
    RELATIVE_PUSH = 1, PUSH = 4,
    RELATIVE_POP  = 2, POP  = 5,
    RELATIVE_MOV  = 3, MOV  = 6,
    
    IN   = 10,
    OUT  = 11,
    JMP  = 12,
    HLT  = 13,
    CALL = 14,
    RET  = 15,
    PIX  = 16,
    SHOW = 17,
    
    JE_O0     = 20, JE     = 30,
    JA_O0     = 21, JA     = 31,
    SQRTSD_O0 = 22, SQRTSD = 32,
    ADDSD_O0  = 23, ADDSD  = 33,
    SUBSD_O0  = 24, SUBSD  = 34,
    MULSD_O0  = 25, MULSD  = 35,
    DIVSD_O0  = 26, DIVSD  = 36,
                    
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
