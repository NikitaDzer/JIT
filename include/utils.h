// 
// Created by MIPT student Nikita Dzer on 28.04.2022.
//

#ifndef JIT_UTILS_H
#define JIT_UTILS_H

#include <stdio.h>


typedef unsigned long long hash_t;

static const unsigned long long BAD_GETTING_LAST_WRITE_TIME = 0;


long get_file_size(FILE *const restrict file);

void* get_file_binary(const char *const restrict file_path, size_t *const restrict content_size);

unsigned long long get_file_last_write_time(const char *const restrict file_path);


extern inline hash_t dedhash(const char* restrict key);

#endif // JIT_UTILS_H