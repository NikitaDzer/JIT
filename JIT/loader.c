// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 


#include <string.h>
#include "../include/executer.h"
#include "../include/loader.h"
#include "../include/utils.h"



static Bincode* find_bincode(const char *const restrict bytecode_file_path, size_t *const restrict executable_size);



const Bincode* load_bincode(const char *const restrict bytecode_file_path, size_t *const restrict executable_size)
{
    return NULL;
}



#define BINCODE_FILE_PATH_SIZE 256

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

static Bincode* find_bincode(const char *const restrict bytecode_file_path, size_t *const restrict executable_size)
{
    char bincode_file_path[BINCODE_FILE_PATH_SIZE] = "";
    
    strcat_s(bincode_file_path, sizeof(BINCODE_FOLDER_PATH), BINCODE_FOLDER_PATH);
    strcat_s(bincode_file_path,
             BINCODE_FILE_PATH_SIZE - sizeof(BINCODE_FOLDER_PATH),
             get_bytecode_file_name(bytecode_file_path));
    
    FILE *const restrict bincode_file = fopen(bincode_file_path, "rb");
    if (bincode_file == NULL)
        return NULL;
    
    const long bincode_file_size    = get_file_size(bincode_file);
    Bincode *const restrict bincode = allocate_bincode(bincode_file_size - DATA_SIZE);
    if (bincode == NULL)
    {
        fclose(bincode_file); // update log
        return NULL;
    }
    
    fread(bincode, sizeof(unsigned char), bincode_file_size, bincode_file);
    fclose(bincode_file);
    
    *executable_size = bincode_file_size - DATA_SIZE;
    
    return bincode;
}
