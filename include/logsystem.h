// 
// Created by MIPT student Nikita Dzer on 25.04.2022.
// 

#ifndef JIT_LOGSYSTEM_H
#define JIT_LOGSYSTEM_H

#include "bytecode.h"
#include "IR.h"


#define $(fn) \
    process_##fn

typedef enum LogResult
{
    LOG_SUCCESS = 0,
} LogResult;



LogResult construct_logsystem(void);

LogResult destruct_logsystem(void);

LogResult logsystem_append(const char *const restrict message,
                           const char *const restrict func, const long line, const char *const restrict file);

Intermediate* process_compiler_bytecode_IR(const Bytecode *const restrict bytecode);

#endif // JIT_LOGSYSTEM_H
