// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#include <string.h>
#include "../include/utils.h"
#include "../include/IR.h"
#include "../include/loader.h"


static IR* read_IR_file(const char *const restrict IR_file_path);


/*!
 * @brief  Loads Intermediate Representation from the file
 * @param  import_IR          Pointer that will contain Intermediate Representation
 * @param  IR_file_path       Path to the file with Intermediate Representation
 * @param  bytecode_file_path Path to the file with Processor bytecode
 * @return Result of loading Intermediate Representation
 */
LoadingResult load_IR(IR *restrict *const restrict import_IR,
                      const char *const restrict IR_file_path, const char *const restrict bytecode_file_path)
{
    const unsigned long long IR_last_write_time       = get_file_last_write_time(IR_file_path);
    const unsigned long long bytecode_last_write_time = get_file_last_write_time(bytecode_file_path);
    
    if (IR_last_write_time == BAD_GETTING_LAST_WRITE_TIME
        || bytecode_last_write_time == BAD_GETTING_LAST_WRITE_TIME
        || IR_last_write_time < bytecode_last_write_time)
        return LOADING_REJECTION;
    
    IR *const restrict IR = read_IR_file(IR_file_path);
    if (IR == NULL)
        return LOADING_FAILURE;
    
    *import_IR = IR;
    return LOADING_SUCCESS;
}


static inline void read_IR_header(const char *const restrict IR_buffer, IRHeader *const restrict header)
{
    *header = *(IRHeader *)IR_buffer;
}

static IR* read_IR_buffer(const char *const restrict IR_buffer, const size_t n_intermediates)
{
    IR *const restrict IR = construct_list(n_intermediates);
    if (IR == NULL)
        return NULL;
    
    IR->nodes[0] = (ListNode){ .prev = n_intermediates, .next = 1 };
    
    memcpy(IR->nodes + 1, IR_buffer, n_intermediates * sizeof(ListNode));
    IR->size = n_intermediates;
    IR->free = -1;
    
    return IR;
}

static IR* read_IR_file(const char *const restrict IR_file_path)
{
    const char *const restrict IR_file_content = get_file_binary(IR_file_path, NULL);
    if (IR_file_content == NULL)
        return NULL;
    
    IRHeader header = {0};
    read_IR_header(IR_file_content, &header);
    
    return read_IR_buffer(IR_file_content + sizeof(IRHeader), header.n_intermediates);
}
