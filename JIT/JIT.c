//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/JIT.h"
#include "../include/utils.h"
#include "../include/executer.h"
#include "../include/compiler_IR_bincode.h"
#include "../include/compiler_bytecode_IR.h"
#include "../include/optimizer.h"


static void dump_executable(const unsigned char *restrict executable, const size_t executable_size);

/*!
 * @brief  Executes Processor bytecode on x86_64 architecture
 * @param  bytecode_file_path Path to file with bytecode
 * @return Result of bytecode translation and execution
 */
JITResult JIT(const char *const restrict bytecode_file_path)
{
    size_t executable_size = 0;
    
    const char *const restrict bytecode = get_file_binary(bytecode_file_path, NULL);
    if (bytecode == NULL)
    {
        return JIT_FAILURE;
    }
    
    IR *const restrict IR = compile_bytecode_IR(bytecode); free((void *)bytecode);
    if (IR == NULL)
    {
        return JIT_FAILURE;
    }
    
    const OptimizationResult optimization_result = optimize(IR);
    if (optimization_result == OPTIMIZATION_FAILURE)
    {
        destruct_list(IR);
        return JIT_FAILURE;
    }
    
    const Bincode *const restrict bincode = compile_IR_bincode(IR, &executable_size); destruct_list(IR);
    if (bincode == NULL)
    {
        return JIT_FAILURE;
    }
    
    dump_executable(bincode->executable, executable_size);
    
    const ExecutionResult execution_result = execute_bincode(bincode->executable, executable_size); free_bincode((void *)bincode);
    if (execution_result == EXECUTION_FAILURE)
    {
        return JIT_FAILURE;
    }
    
    return JIT_SUCCESS;
}

static void dump_executable(const unsigned char *restrict executable, const size_t executable_size)
{
    FILE *restrict dump = fopen("../bincode.bin", "wb");
    if (dump == NULL)
        return;
    
    unsigned char hex[2] = {0};
    
    for (size_t i = 0; i < executable_size; i++)
    {
        unsigned char ascii  = executable[i];
    
        hex[0] = ascii / 16 >= 10 ? ascii / 16 - 10 + 'A' : ascii / 16 + '0';
        hex[1] = ascii % 16 >= 10 ? ascii % 16 - 10 + 'A' : ascii % 16 + '0';
        
        fwrite(hex, sizeof(unsigned char), 2, dump);
    }
    
    fclose(dump);
}
