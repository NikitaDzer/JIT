// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_IR_H
#define JIT_IR_H

#include "list/list.h"

typedef struct IRHeader
{
    size_t n_intermediates;
} IRHeader;

typedef List IR;

#endif // JIT_IR_H
