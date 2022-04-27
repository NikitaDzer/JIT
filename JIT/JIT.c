//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/JIT.h"
#include "../include/utils.h"
#include "../include/executer.h"


#define BINCODE_FILE_PATH_SIZE 256

static const char* find_bincode(const char *const restrict bytecode_file_path);

JITResult JIT(const char *const restrict bytecode_file_path)
{
    const char *restrict bincode = find_bincode(bytecode_file_path);
    if (bincode == NULL)
    {
    
    }
    
    return JIT_SUCCESS;
}

static const char BINCODE_FOLDER_PATH[] = "C:\\Windows\\Temp";

static inline const char* get_bytecode_file_name(const char *const restrict bytecode_file_path)
{
    const size_t bytecode_file_path_len = strlen(bytecode_file_path);
    const char *restrict latest_subpath = bytecode_file_path;
    
    for (size_t i = 0; i < bytecode_file_path_len; i++)
        if (bytecode_file_path[i] == '\\')
            latest_subpath = bytecode_file_path + i + 1;
    
    return latest_subpath;
}

static const char* find_bincode(const char *const restrict bytecode_file_path)
{
    char bincode_file_path[BINCODE_FILE_PATH_SIZE] = "";
    
    strcat(bincode_file_path, BINCODE_FOLDER_PATH);
    strcat_s(bincode_file_path,
             BINCODE_FILE_PATH_SIZE - sizeof(BINCODE_FOLDER_PATH),
             get_bytecode_file_name(bytecode_file_path));
    
    FILE *const restrict bincode_file = fopen(bincode_file_path, "rb"); // make fopen_s with errno_t
    if (bincode_file == NULL)
        return NULL;
    
    const long bincode_file_size = get_file_size(bincode_file);
    char *const restrict bincode = calloc(bincode_file_size, sizeof(char));
    if (bincode == NULL)
    {
        fclose(bincode_file); // update log
        return NULL;
    }
    
    fread(bincode, sizeof(char), bincode_file_size + 1, bincode_file); // error processing
    fclose(bincode_file); // error processing
    
    return bincode;
}
