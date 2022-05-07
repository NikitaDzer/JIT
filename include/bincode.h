// 
// Created by MIPT student Nikita Dzer on 08.05.2022.
// 

#ifndef JIT_BINCODE_H
#define JIT_BINCODE_H

typedef struct Bincode
{
    unsigned char *restrict executable;
    unsigned char *restrict data;
} Bincode;

#endif // JIT_BINCODE_H
