// 
// Created by MIPT student Nikita Dzer on 24.04.2022.
// 

#ifndef JIT_COMPILER_BYTECODE_IR_H
#define JIT_COMPILER_BYTECODE_IR_H

#include "IR.h"
#include "bytecode.h"

Intermediate* compile_bytecode_IR(const Bytecode *const restrict bytecode);

#endif // JIT_COMPILER_BYTECODE_IR_H
