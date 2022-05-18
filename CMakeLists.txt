cmake_minimum_required(VERSION 3.21)
project(JIT C)

set(CMAKE_C_STANDARD 11)

add_executable(JIT main.c JIT/JIT.c JIT/executer.c JIT/saver.c JIT/loader.c
               compilers/compiler_bytecode_IR.c compilers/compiler_IR_bincode.c
               optimizer/optimizer.c tools/utils.c tools/list/list.c)
