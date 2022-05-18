// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
//

#include <stdlib.h>
#include <fileapi.h>
#include <handleapi.h>
#include "../include/utils.h"

static void* get_file_content(FILE *const restrict file, size_t *const restrict content_size);


long get_file_size(FILE *const restrict file)
{
    const long current_position = ftell(file);
    
    fseek(file, 0L, SEEK_END); // error processing
    
    const long file_size = ftell(file);
    
    fseek(file, current_position, SEEK_SET); // error processing
    
    return file_size;
}

void* get_file_binary(const char *const restrict file_path, size_t *const restrict content_size)
{
    FILE *const restrict file = fopen(file_path, "rb");
    if (file == NULL)
        return NULL;
    
    return get_file_content(file, content_size);
}

unsigned long long get_file_last_write_time(const char *const restrict file_path)
{
    HANDLE file = CreateFileA(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    // ------------------------------------------------^---------------------------------------------------
    // Prevents other processes from opening a file or device if they request delete, read, or write access
    if (file == INVALID_HANDLE_VALUE)
        return BAD_GETTING_LAST_WRITE_TIME;
    
    FILETIME file_time = {0};
    const WINBOOL result = GetFileTime(file, NULL, NULL, &file_time); CloseHandle(file);
    
    if (result == 0)
        return BAD_GETTING_LAST_WRITE_TIME;
    
    return ((unsigned long long)file_time.dwHighDateTime << (sizeof(DWORD) * CHAR_BIT)) | file_time.dwLowDateTime;
}

extern inline hash_t dedhash(const char* restrict key)
{
    hash_t hash = key[0];
    size_t len = strlen(key);
    
    for (size_t i = 1; i < len; i++)
        hash = ((hash >> 1) | (hash << (sizeof(hash_t) * CHAR_BIT - 1))) ^ key[i];
    
    return hash;
}


static void* get_file_content(FILE *const restrict file, size_t *const restrict content_size)
{
    const long file_size = get_file_size(file);
    void *const restrict content = calloc(file_size + 1, sizeof(char));
    if (content == NULL)
    {
        fclose(file);
        return NULL;
    }
    
    const size_t content_size_local = fread(content, sizeof(char), file_size, file);
    
    if (content_size != NULL)
        *content_size = content_size_local;
    
    fclose(file);
    
    return content;
}