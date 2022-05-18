// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#ifndef JIT_OPTIMIZER_H
#define JIT_OPTIMIZER_H

#include "IR.h"

typedef enum OptimizationResult
{
    OPTIMIZATION_SUCCESS = 1,
    OPTIMIZATION_FAILURE = 2,
} OptimizationResult;

OptimizationResult optimize(IR *const restrict IR);

#endif // JIT_OPTIMIZER_H
