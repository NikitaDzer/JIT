// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_COMPILER_IR_BINCODE_H
#define JIT_COMPILER_IR_BINCODE_H

#include "IR.h"
#include "bincode.h"

const Bincode* compile_IR_bincode(IR *const restrict IR, size_t *const restrict executable_size);

#endif // JIT_COMPILER_IR_BINCODE_H
