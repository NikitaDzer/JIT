//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <sysinfoapi.h>
#include <memoryapi.h>
#include <stdio.h>
#include "../include/executer.h"



static inline unsigned char* find_data_buffer(unsigned char *const restrict buffer, const unsigned long long executable_size);


/*!
 * @brief  Executes buffer of binary instructions
 * @param  executable       Buffer of binary instructions
 * @param  executable_size  Executable buffer size
 * @return Result of buffer execution
 */
ExecutionResult execute_bincode(const unsigned char *const restrict executable, const unsigned long long executable_size)
{
    DWORD old_protect = 0;
    VirtualProtect((void *)executable, executable_size, PAGE_EXECUTE_READ, &old_protect);
    
    ((void (*)(void))executable)();
    
    return EXECUTION_SUCCESS;
}

/*!
 * @brief  Allocates buffers for data and binary instructions
 * @param  executable_size  Executable buffer size
 * @return Pointer to struct with data and executable buffers
 */
Bincode*       allocate_bincode(const unsigned long long executable_size)
{
    Bincode *const restrict bincode = calloc(1, sizeof(Bincode));
    if (bincode == NULL)
        return NULL;
    
    unsigned char *const restrict buffer = VirtualAlloc(NULL, executable_size + DATA_SIZE,
                                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (buffer == NULL)
    {
        free(bincode);
        return NULL;
    }
    
    bincode->executable = buffer;
    bincode->data       = find_data_buffer(buffer, executable_size);
    
    return bincode;
}

/*!
 * @brief  Free buffers for data and binary instructions
 * @param  bincode Pointer to struct with data and executable buffers
 * @return Nothing
 */
void free_bincode(Bincode *const restrict bincode)
{
    VirtualFree(bincode->executable, 0, MEM_RELEASE); // error processing (optional)
    free(bincode);
}



static inline DWORD get_page_size(void)
{
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    
    return system_info.dwPageSize;
}

static inline unsigned char* find_data_buffer(unsigned char *const restrict buffer, const unsigned long long executable_size)
{
    const DWORD page_size = get_page_size();
    
    return buffer + (executable_size / (unsigned long long)page_size + 1ULL) * page_size;
}