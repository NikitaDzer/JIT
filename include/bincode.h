// 
// Created by MIPT student Nikita Dzer on 08.05.2022.
// 

#ifndef JIT_BINCODE_H
#define JIT_BINCODE_H

typedef enum BincodeRegistry
{
    BINCODE_RAX = 0, BINCODE_RBX = 3,
    BINCODE_RCX = 1, BINCODE_RDX = 2,
    BINCODE_RDI = 7, BINCODE_RSI = 6,
    BINCODE_RBP = 5, BINCODE_RSP = 4,
    
    BINCODE_R8  = 0, BINCODE_R9  = 1,
    BINCODE_R10 = 2, BINCODE_R11 = 3,
    BINCODE_R12 = 4, BINCODE_R13 = 5,
    BINCODE_R14 = 6, BINCODE_R15 = 7
} BincodeRegistry;

typedef struct Bincode
{
    unsigned char *restrict executable;
    unsigned char *restrict data;
} Bincode;

#endif // JIT_BINCODE_H
