// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include <stdlib.h>
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

char* get_file_text(const char *const restrict file_path, size_t *const restrict content_size)
{
    FILE *const restrict file = fopen(file_path, "r");
    if (file == NULL)
        return NULL;
    
    return (char *)get_file_content(file, content_size);
}

void* get_file_binary(const char *const restrict file_path, size_t *const restrict content_size)
{
    FILE *const restrict file = fopen(file_path, "rb");
    if (file == NULL)
        return NULL;
    
    return get_file_content(file, content_size);
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