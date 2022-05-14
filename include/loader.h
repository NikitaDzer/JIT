// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#ifndef JIT_LOADER_H
#define JIT_LOADER_H

#include "bincode.h"

const Bincode* load_bincode(const char *const restrict bytecode_file_path, size_t *const restrict executable_size);

#endif // JIT_LOADER_H
