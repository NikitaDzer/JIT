// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
// 

#include "../include/utils.h"

long get_file_size(FILE *const restrict file)
{
    const long current_position = ftell(file);
    
    fseek(file, 0L, SEEK_END); // error processing
    
    const long file_size = ftell(file);
    
    fseek(file, current_position, SEEK_SET); // error processing
    
    return file_size;
}
