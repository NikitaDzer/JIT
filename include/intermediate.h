// 
// Created by MIPT student Nikita Dzer on 04.05.2022.
// 

#ifndef JIT_INTERMEDIATE_H
#define JIT_INTERMEDIATE_H

typedef enum IntermediateArgumentType
{
    ARG_TYPE_REG     = 1,
    ARG_TYPE_INT     = 2,
    ARG_TYPE_DBL     = 3,
    ARG_TYPE_MEM_REG = 4,
    ARG_TYPE_MEM_INT = 5,
} IntermediateArgumentType;

typedef struct IntermediateArgument
{
    union
    {
        unsigned char        registry;
        long long            iconstant;
        double               dconstant;
        unsigned long long   address;
        struct Intermediate *reference;
    };
    
    IntermediateArgumentType type;
} IntermediateArgument;

// Can be optimized to 32-byte size for being more cache-friendly
typedef struct Intermediate
{
    IntermediateArgument argument1;
    IntermediateArgument argument2;
    
    unsigned char opcode;
} Intermediate;

#endif // JIT_INTERMEDIATE_H