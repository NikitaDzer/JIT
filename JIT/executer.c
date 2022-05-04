//
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <sysinfoapi.h>
#include <memoryapi.h>
#include "../include/executer.h"


static inline DWORD get_page_size();

static inline DWORD calc_buffer_size(const unsigned long long bincode_size, const DWORD page_size);



ExecutionResult execute_bincode(const unsigned char *const restrict bincode, const unsigned long long bincode_size)
{
    DWORD old_protect = 0;
    VirtualProtect((void *)bincode, (DWORD)bincode_size, PAGE_EXECUTE_READ, &old_protect); // error processing
    
    ((void (*)(void))bincode)();
    
    return EXECUTION_SUCCESS;
}

inline unsigned char* allocate_bincode(const unsigned long long bincode_size)
{
    return VirtualAlloc(NULL, calc_buffer_size(bincode_size, get_page_size()),
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

inline void free_bincode(unsigned char *const restrict bincode)
{
    VirtualFree(bincode, 0, MEM_RELEASE); // error processing (optional)
}



static inline DWORD calc_buffer_size(const unsigned long long bincode_size, const DWORD page_size)
{
    return (bincode_size / (unsigned long long)page_size + 1ULL) * page_size;
}

static inline DWORD get_page_size()
{
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    
    return system_info.dwPageSize;
}