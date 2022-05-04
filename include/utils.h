// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
//

#ifndef JIT_UTILS_H
#define JIT_UTILS_H

#include <stdio.h>

long get_file_size(FILE *const restrict file);

char* get_file_text(const char *const restrict file_path, size_t *const restrict content_size);

void* get_file_binary(const char *const restrict file_path, size_t *const restrict content_size);

#endif // JIT_UTILS_H
