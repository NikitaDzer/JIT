cmake_minimum_required(VERSION 3.21)
project(JIT C)

set(CMAKE_C_STANDARD 11)

add_executable(
        JIT main.c tools/list/list.c tools/list/dump.c include/IR.h include/JIT.h include/compiler_bytecode_IR.h include/compiler_IR_bincode.h
        include/decompiler_IR_asm.h include/analyzer.h include/optimizer.h include/logsystem.h include/config.h include/list/list.h
        include/list/dump.h include/list/config.h compilers/compiler_bytecode_IR.c include/bytecode.h JIT/JIT.c include/utils.h tools/utils.c JIT/executer.c include/executer.h compilers/compiler_IR_bincode.c include/intermediate.h include/bincode.h optimizer/optimizer.c)
