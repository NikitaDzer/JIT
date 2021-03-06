//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

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


static char* get_IR_file_path(const char *const restrict bytecode_file_path);


/*!
 * @brief  Executes Processor bytecode on x86_64 architecture
 * @param  bytecode_file_path Path to file with bytecode
 * @return Result of bytecode translation and execution
 */
JITResult JIT(const char *const restrict bytecode_file_path)
{
    size_t executable_size = 0;
    IR *restrict IR = NULL;
    
    char *const IR_file_path = get_IR_file_path(bytecode_file_path);
    if (IR_file_path == NULL)
        return JIT_FAILURE;
    
    const LoadingResult loading_result = load_IR(&IR, IR_file_path, bytecode_file_path);
    if (loading_result == LOADING_FAILURE)
    {
        free(IR_file_path);
        return JIT_FAILURE;
    }
    
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
    
    free(IR_file_path);
    
    const Bincode *const restrict bincode = compile_IR_bincode(IR, &executable_size); destruct_list(IR);
    if (bincode == NULL)
        return JIT_FAILURE;
    
    const ExecutionResult execution_result = execute_bincode(bincode->executable, executable_size); free_bincode((void *)bincode);
    if (execution_result == EXECUTION_FAILURE)
        return JIT_FAILURE;
    
    return JIT_SUCCESS;
}


#define HASH_BUFFER_SIZE (sizeof(hash_t) * CHAR_BIT)

static const char               IR_FOLDER_SUBPATH[] = "\\JIT_IR";
static const unsigned long long IR_FILE_PATH_SIZE = 128;

static char* get_IR_file_path(const char *const restrict bytecode_file_path)
{
    const char *const restrict temp_folder = getenv("temp");
    if (temp_folder == NULL)
        return NULL;
    
    char hash_buffer[HASH_BUFFER_SIZE] = "";
    char *const restrict IR_file_path  = calloc(IR_FILE_PATH_SIZE, sizeof(char));
    if (IR_file_path == NULL)
        return NULL;
    
    strcat(IR_file_path, temp_folder);
    strcat(IR_file_path, IR_FOLDER_SUBPATH);
    strcat(IR_file_path, _ui64toa( dedhash(bytecode_file_path), hash_buffer, 16 ));
    
    return IR_file_path;
}
