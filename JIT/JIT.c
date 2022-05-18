//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/JIT.h"
#include "../include/utils.h"
#include "../include/executer.h"
#include "../include/compiler_IR_bincode.h"
#include "../include/compiler_bytecode_IR.h"
#include "../include/optimizer.h"
#include "../include/loader.h"
#include "../include/saver.h"


static void        dump_executable (const unsigned char *restrict executable, const size_t executable_size);
static const char* get_IR_file_path(const char *const restrict bytecode_file_path);


/*!
 * @brief  Executes Processor bytecode on x86_64 architecture
 * @param  bytecode_file_path Path to file with bytecode
 * @return Result of bytecode translation and execution
 */
JITResult JIT(const char *const restrict bytecode_file_path)
{
    size_t executable_size = 0;
    IR *restrict IR = NULL;
    
    const char *const IR_file_path = get_IR_file_path(bytecode_file_path);
    if (IR_file_path == NULL)
        return JIT_FAILURE;
    
    const LoadingResult loading_result = load_IR(&IR, IR_file_path, bytecode_file_path);
    if (loading_result == LOADING_FAILURE)
        return JIT_FAILURE;
    
    if (loading_result == LOADING_REJECTION)
    {
        const char *const restrict bytecode = get_file_binary(bytecode_file_path, NULL);
        if (bytecode == NULL)
            return JIT_FAILURE;
    
        IR = compile_bytecode_IR(bytecode); free((void *)bytecode);
        if (IR == NULL)
            return JIT_FAILURE;
    
        const OptimizationResult optimization_result = optimize(IR);
        if (optimization_result == OPTIMIZATION_FAILURE)
        {
            destruct_list(IR);
            return JIT_FAILURE;
        }
        
        const SavingResult saving_result = save_IR(IR, IR_file_path);
        if (saving_result == SAVING_FAILURE)
        {
            destruct_list(IR);
            return JIT_FAILURE;
        }
    }
    
    const Bincode *const restrict bincode = compile_IR_bincode(IR, &executable_size); destruct_list(IR);
    if (bincode == NULL)
        return JIT_FAILURE;
    
    dump_executable(bincode->executable, executable_size);
    
    const ExecutionResult execution_result = execute_bincode(bincode->executable, executable_size); free_bincode((void *)bincode);
    if (execution_result == EXECUTION_FAILURE)
        return JIT_FAILURE;
    
    return JIT_SUCCESS;
}



#define HASH_BUFFER_SIZE (sizeof(hash_t) * CHAR_BIT)

static const char               IR_FOLDER_PATH[]  = "C:\\Windows\\Temp\\";
static const char               IR_FILE_PREFIX[]  = "JIT_IR_";
static const unsigned long long IR_FILE_PATH_SIZE = 128;


static const char* get_IR_file_path(const char *const restrict bytecode_file_path)
{
    char hash_buffer[HASH_BUFFER_SIZE] = "";
    char *const restrict IR_file_path  = calloc(IR_FILE_PATH_SIZE, sizeof(char));
    if (IR_file_path == NULL)
        return NULL;
    
    strcat(IR_file_path, IR_FOLDER_PATH);
    strcat(IR_file_path, IR_FILE_PREFIX);
    strcat(IR_file_path, _ui64toa( dedhash(bytecode_file_path), hash_buffer, 16 ));
    
    return IR_file_path;
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


