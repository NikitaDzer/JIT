cmake_minimum_required(VERSION 3.21)
project(JIT C)

set(CMAKE_C_STANDARD 11)

add_executable(
        JIT main.c tools/list/list.c tools/list/dump.c include/IR.h include/JIT.h include/bytecode.h include/compiler_bytecode_IR.h include/compiler_IR_bincode.h
        include/decompiler_IR_asm.h include/analyzer.h include/optimizer.h include/logsystem.h include/config.h include/list/list.h
        include/list/dump.h include/list/config.h )
