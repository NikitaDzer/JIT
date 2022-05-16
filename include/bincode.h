// 
// Created by MIPT student Nikita Dzer on 08.05.2022.
// 

#ifndef JIT_BINCODE_H
#define JIT_BINCODE_H

typedef enum BincodeRegistry
{
    BINCODE_RAX = 0, BINCODE_R8  = 0,
    BINCODE_RBX = 3, BINCODE_R9  = 1,
    BINCODE_RCX = 1, BINCODE_R10 = 2,
    BINCODE_RDX = 2, BINCODE_R11 = 3,
    BINCODE_RDI = 7, BINCODE_R12 = 4,
    BINCODE_RSI = 6, BINCODE_R13 = 5,
    BINCODE_RBP = 5, BINCODE_R14 = 6,
    BINCODE_RSP = 4, BINCODE_R15 = 7,
    
    BINCODE_XMM0 = 0, BINCODE_XMM8  = 0,
    BINCODE_XMM1 = 1, BINCODE_XMM9  = 1,
    BINCODE_XMM2 = 2, BINCODE_XMM10 = 2,
    BINCODE_XMM3 = 3, BINCODE_XMM11 = 3,
    BINCODE_XMM4 = 4, BINCODE_XMM12 = 4,
    BINCODE_XMM5 = 5, BINCODE_XMM13 = 5,
    BINCODE_XMM6 = 6, BINCODE_XMM14 = 6,
    BINCODE_XMM7 = 7, BINCODE_XMM15 = 7,
    
    BINCODE_NO_REGISTRY = 16
} BincodeRegistry;

typedef struct Bincode
{
    unsigned char *restrict executable;
    unsigned char *restrict data;
} Bincode;

#endif // JIT_BINCODE_H
