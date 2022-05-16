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
    //  -v- bytecode-based opcodes -v-
                    RELATIVE_PUSH = 30, PUSH = 50,
                    RELATIVE_POP  = 31, POP  = 51,
    O0_ADD    = 1,  RELATIVE_ADD  = 32, ADD  = 52,
    O0_IMUL   = 2,  RELATIVE_IMUL = 33, IMUL = 53,
    O0_IN     = 3,
    O0_PRINTF = 4,
    O0_JMP    = 5,
    O0_JE     = 6,
    O0_JA     = 7,
    O0_HLT    = 8,
    O0_CALL   = 9,
    O0_RET    = 10,
    O0_SQRTSD = 11,                     SQRTSD = 54,
    O0_SUB    = 12, RELATIVE_SUB  = 34, SUB  = 55,
    O0_DIV    = 13,
    O0_PIX    = 14,
    O0_SHOW   = 15,
    O0_ADDSD  = 16,                     ADDSD = 56,
    O0_SUBSD  = 17,                     SUBSD = 57,
    O0_MULSD  = 18,                     MULSD = 58,
    O0_DIVSD  = 19,                     DIVSD = 59,
    
    //  -v- non-bytecode-based opcodes -v-
                    RELATIVE_MOV   = 35, MOV   = 60,
                    RELATIVE_MOVSD = 36, MOVSD = 61,
                    RELATIVE_XCHG  = 37, XCHG  = 62,
                    RELATIVE_NEG   = 38, NEG   = 63,
                    
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
