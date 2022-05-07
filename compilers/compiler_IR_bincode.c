// 
// Created by MIPT student Nikita Dzer on 04.05.2022.
// 

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/compiler_IR_bincode.h"
#include "../include/executer.h"


typedef enum CompilationResult
{
    COMPILATION_PLUG    = 0,
    COMPILATION_SUCCESS = 1,
    COMPILATION_FAILURE = 2
} CompilationResult;


static unsigned char *restrict executable_free = NULL;

static const Intermediate *restrict *restrict unresolved_intermediates_free = NULL;


static inline CompilationResult compile_intermediates(IR *const restrict IR);

static inline void resolve_intermediates(const Intermediate *const restrict *const restrict unresolved_intermediates);

const Bincode* compile_IR_bincode(IR *const restrict IR, size_t *const restrict executable_size, const size_t data_size)
{
    Bincode *const restrict bincode = allocate_bincode(IR->size, data_size);
    if (bincode == NULL)
        return NULL;
    
    executable_free = bincode->executable;
    
    const Intermediate *restrict *const restrict unresolved_intermediates = calloc(IR->size, sizeof(Intermediate *)); // can be optimized
    unresolved_intermediates_free = unresolved_intermediates;
    if (unresolved_intermediates == NULL)
    {
        free_bincode(bincode);
        return NULL;
    }
    
    CompilationResult compilation_result = compile_intermediates(IR);
    if (compilation_result == COMPILATION_FAILURE)
    {
        free((void *)unresolved_intermediates);
        free_bincode(bincode);
        
        return NULL;
    }
    
    resolve_intermediates(unresolved_intermediates);
    
    free((void *)unresolved_intermediates);
    
    *executable_size = executable_free - bincode->executable;
    
    return bincode;
}


#define EXECUTABLE_PUSH(type, data)                 \
    do {                                            \
        *(type *)(executable_free) = (type)(data);  \
        executable_free += sizeof(type);            \
    } while (0)

#define ADD_UNRESOLVED_INTERMEDIATE()                    \
    do {                                                 \
        executable_free += sizeof(int32_t);              \
        *unresolved_intermediates_free++ = intermediate; \
    } while (0)

#define CREATE_ARGUMENT(arg_type, data)                                         \
    (arg_type) == ARG_TYPE_REG ?                                                \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == ARG_TYPE_INT ?                                                \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
    (arg_type) == ARG_TYPE_MEM_REG ?                                            \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == ARG_TYPE_MEM_INT ?                                            \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
            (IntermediateArgument){ .type = (arg_type), .reference = NULL   }
            

typedef enum BincodeRegistry
{
    BINCODE_RAX = 0, BINCODE_RBX = 3,
    BINCODE_RCX = 1, BINCODE_RDX = 2,
    BINCODE_RDI = 7, BINCODE_RSI = 6,
    BINCODE_RBP = 5, BINCODE_RSP = 4,
    
    BINCODE_R8  = 0, BINCODE_R9  = 1,
    BINCODE_R10 = 2, BINCODE_R11 = 3,
    BINCODE_R12 = 4, BINCODE_R13 = 5,
    BINCODE_R14 = 6, BINCODE_R15 = 7
} BincodeRegistry;


static const unsigned char REGISTER_DIRECT    = 0b11000000;
static const unsigned char REGISTER_INDIRECT  = 0b00000000;
static const unsigned char SIB                = 0b00000100;

static const unsigned char REX_B              = 0b00000001; // extension of the ModRM r/m, SIB base or opcode reg
static const unsigned char REX_X              = 0b00000010; // extension of the SIB index
static const unsigned char REX_R              = 0b00000100; // extension of the ModRM reg
static const unsigned char REX_W              = 0b00001000; // 64-bit operand size
static const unsigned char REX_IDENTIFIER     = 0b01000000;

static const size_t JUMP_OR_CALL_INSTRUCTION_SIZE = sizeof(int32_t) + 1;


static inline unsigned char get_modrm(const unsigned char is_register_direct,
                                      const unsigned char registry,
                                      const unsigned char registry_memory)
{
    return is_register_direct | (registry << 3) | registry_memory;
}

static inline BincodeRegistry get_rq(const IntermediateRegistry registry)
{
    switch (registry)
    {
        case RAX:
        case RBX:
        case RCX:
        case RDX:
        case RDI:
        case RSI:
        case RBP:
        case RSP: return (BincodeRegistry)registry;
        
        case R8:
        case R9:
        case R10:
        case R11:
        case R12:
        case R13:
        case R14:
        case R15: return (BincodeRegistry)(registry - R8);
    }
}

static inline bool is_new_rq(const IntermediateRegistry registry)
{
    return registry >= R8;
}

static inline bool is_dword(const long long iconstant)
{
    return INT32_MIN <= iconstant && iconstant <= INT32_MAX;
}

static inline void write_push(const IntermediateArgument argument)
{
    if (argument.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x50 + get_rq(registry));
    }
    else if (argument.type == ARG_TYPE_INT)
    {
        const long long iconstant = argument.iconstant;
        
        EXECUTABLE_PUSH(int8_t, 0x68);
        EXECUTABLE_PUSH(int32_t, iconstant);
    }
    else if (argument.type == ARG_TYPE_MEM_REG)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
    
        EXECUTABLE_PUSH(int8_t, 0xFF);
        EXECUTABLE_PUSH(int8_t, (6 << 3) | get_rq(registry));
    }
    else
    {
    }
}

static inline void write_pop(const IntermediateArgument argument)
{
    if (argument.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x58 + get_rq(registry));
    }
    else if (argument.type == ARG_TYPE_MEM_REG)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
        
        EXECUTABLE_PUSH(int8_t, 0x8F);
        EXECUTABLE_PUSH(int8_t, get_rq(registry));
    }
}

static inline void write_mov(const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument1.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry = argument1.registry;
        
        if (argument2.type == ARG_TYPE_INT)
        {
            if (is_new_rq(registry))
                EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | REX_R);
            else
                EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W);

            EXECUTABLE_PUSH(int8_t, 0xB8 + get_rq(registry));
            EXECUTABLE_PUSH(int64_t, argument2.iconstant);
        }
        else // if (argument2.type == ARG_TYPE_MEM_REG || argument2.type == ARG_TYPE_MEM_INT)
        {
            /*
            if (registry < N_RQ_REGISTERS)
            {
                EXECUTABLE_PUSH(unsigned char, REX_IDENTIFIER | REX_W);
                EXECUTABLE_PUSH(unsigned char, 0x8B);
                EXECUTABLE_PUSH(unsigned char, 10);
            }
            */
        }
    }
}

static inline void write_call(const IntermediateArgument argument)
{
    if (argument.type == ARG_TYPE_INT)
    {
        EXECUTABLE_PUSH(int8_t, 0xE8);
        EXECUTABLE_PUSH(int32_t, argument.iconstant);
    }
    else // if (argument.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry = argument.registry;
        
        if (is_new_rq(registry))
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_B);
    
        EXECUTABLE_PUSH(int8_t, 0xFF);
        EXECUTABLE_PUSH(int8_t, (2 << 3) | get_rq(registry));
    }
}

static void write_JIT_jumptable()
{
}

__attribute__((__always_inline__))
static inline CompilationResult compile_intermediate(Intermediate *const restrict intermediate)
{
    unsigned char *const restrict address = executable_free;
    
    switch (intermediate->opcode)
    {
        // ADD
        case 0x10:
        {
            if (intermediate->argument1.type == ARG_TYPE_REG)
            {
                // ADD_R_I
                if (intermediate->argument2.type == ARG_TYPE_INT)
                {
                    EXECUTABLE_PUSH(unsigned char, 0x81);
                    EXECUTABLE_PUSH(unsigned char, 0b11000000 | (intermediate->argument1.registry << 3) | 0b00000000);
                    EXECUTABLE_PUSH(long long, intermediate->argument2.iconstant);
                }
                else // if (type == ARG_TYPE_REG)
                {
                
                }
            }
            
            break;
        }
            
        // RET
        case 0x00:
        {
            EXECUTABLE_PUSH(unsigned char, 0xC3);
            
            break;
        }
            
        // PUSH
        case 0x01:
        {
            write_push(intermediate->argument1);
            break;
        }
            
        // POP
        case 0x02:
        {
            write_pop(intermediate->argument1);
            break;
        }
            
        // CALL
        case 0x03:
        {
            const Intermediate *const restrict reference = intermediate->argument1.reference;
            
            EXECUTABLE_PUSH(unsigned char, 0xE8);
            
            if (reference->is_compiled)
                EXECUTABLE_PUSH(int32_t, reference->argument1.address - executable_free - 1);
            else
                ADD_UNRESOLVED_INTERMEDIATE();
            
            break;
        }
        
        // JMP
        case 0x04:
        {
            const Intermediate *const restrict reference = intermediate->argument1.reference;
    
            EXECUTABLE_PUSH(unsigned char, 0xE9);
            
            if (reference->is_compiled)
                EXECUTABLE_PUSH(int32_t, reference->argument1.address - executable_free - 1);
            else
                ADD_UNRESOLVED_INTERMEDIATE();
            
            break;
        }
        
        // OUT
        case 0x05:
        {
            write_pop((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 5 }); // get argument from stack
            
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, 5));
            
            write_push((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 0 });
            write_push((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 5 });
            write_push((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 6 });
            write_push((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 1 });
            
            write_mov((IntermediateArgument){ .type = ARG_TYPE_REG, .registry  = 0 },
                      (IntermediateArgument){ .type = ARG_TYPE_INT, .iconstant = 0 });
            
            write_mov((IntermediateArgument){ .type = ARG_TYPE_REG, .registry  = 6 },
                      (IntermediateArgument){ .type = ARG_TYPE_INT, .iconstant = 0x402004 });
    
            write_mov((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 1 },
                      (IntermediateArgument){ .type = ARG_TYPE_INT, .iconstant = printf });
    
            EXECUTABLE_PUSH(unsigned char, 0x48);
            EXECUTABLE_PUSH(unsigned char, 0x83);
            EXECUTABLE_PUSH(unsigned char, 0xEC);
            EXECUTABLE_PUSH(unsigned char, 0x08);
            
            EXECUTABLE_PUSH(unsigned char, 0xFF);
            EXECUTABLE_PUSH(unsigned char, 0xD3);
    
            EXECUTABLE_PUSH(unsigned char, 0x48);
            EXECUTABLE_PUSH(unsigned char, 0x83);
            EXECUTABLE_PUSH(unsigned char, 0xC4);
            EXECUTABLE_PUSH(unsigned char, 0x08);
            
            write_pop((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 1 });
            write_pop((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 6 });
            write_pop((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 5 });
            write_pop((IntermediateArgument){ .type = ARG_TYPE_REG, .registry = 0 });
            
            break;
        }
        
        default:
        {
            return COMPILATION_FAILURE;
        }
    }
    
    intermediate->argument2.address = address;
    intermediate->is_compiled = 1;
    
    return COMPILATION_SUCCESS;
}

__attribute__((__always_inline__))
static inline CompilationResult compile_intermediates(IR *const restrict IR)
{
    CompilationResult compilation_result = COMPILATION_PLUG;
    
    for (list_index_t i = list_reset_iterator(IR); i != 0; i = list_iterate_forward(IR))
    {
        compilation_result = compile_intermediate(&IR->nodes[i].item);
        
        if (compilation_result == COMPILATION_FAILURE)
            return COMPILATION_FAILURE;
    }
    
    return COMPILATION_SUCCESS;
}

static inline void resolve_intermediates(const Intermediate *const restrict *const restrict unresolved_intermediates)
{
          unsigned char *restrict operand_address     = NULL;
          unsigned char *restrict instruction_address = NULL;
    const unsigned char *restrict destination_address = NULL;
    
    for (const Intermediate *const restrict *restrict iterator = unresolved_intermediates;
         iterator != unresolved_intermediates_free;
         iterator += 1)
    {
        instruction_address = (*iterator)->argument2.address;
        operand_address     = instruction_address + 1;
        destination_address = (*iterator)->argument1.reference->argument2.address;
        
        *(int32_t *)operand_address = (int32_t)(destination_address - instruction_address - JUMP_OR_CALL_INSTRUCTION_SIZE);
    }
}


/*
 * +---------------------------------------------------------------------+
 * |                       x86-64 instructions                           |
 * +---------------------------------------------------------------------+
 *
 * legacy prefixes | opcode with prefixes | mod R/M     |  SIB        | displacement | immediate
 * ----------------|----------------------|-------------|-------------|--------------|------------
 * optional        | required             | if required | if required | if required  | if required
 * 1-4             | 1-4                  | 1           | 1           | 1, 2, 4, 8   | 1, 2, 4, 8
 *
 *
 *
 * --------------------- Legacy opcodes ---------------------
 * Mandatory prefix:
 * - probably for SIMD instructions
 *
 * REX prefix:
 * - instructions, that default to 64-bit operand size in long mode:
 *   CALL (near), ENTER, Jcc, JMP (near), LEAVE, LOOPcc, MOV CR, MOV DR, POP, PUSH, RET (near)
 *
 *
 * Opcode:
 * - can be 1, 2, 3 length
 *
 * --------------------- VEX/XOP opcodes ---------------------
 *
 * --------------------- 3DNow! opcodes ---------------------
 *
 *
 * --------------------- ModRM ---------------------
 * It's used to specify:
 * - 2 regs
 * - 1 reg, 1 mem-based operand, addressing mode
 *
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * |  mod  |    reg    |     rm    |
 * +---+---+---+---+---+---+---+---+
 * \-- 2 --\---- 3 ----\---- 3 ----\
 *
 * mod:
 * - mod  = 0b11 - reg-direct addressing mode
 * - mod != 0b11 - reg-indirect addressing mode
 *
 * reg:
 * - reg = rax, rcx, rdx, rbx, others...
 *
 * rm:
 * - mod  = 0b11 => rm = reg
 * - mod != 0b11 => rm = [reg]
 *
 *
 * --------------------- SIB ---------------------
 * Formula:
 * - effective_address = scale * index + base + offset
 *
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | scale |   index   |   base    |
 * +---+---+---+---+---+---+---+---+
 * \-- 2 --\---- 3 ----\---- 3 ----\
 *
 * Table:
 * - scale = 1, 2, 4, 8
 * - index = rax, rcx, rdx, rbx, others...
 * - base  = rax, rcx, rdx, rbx, others...
 *
 * Jokes:
 * - if base = 0b101, offset can be added
 *
 * --------------------- REX ------------------------
 * Format:
 * 8   7   6   5   4   3   2   1   0
 * +---+---+---+---+---+---+---+---+
 * | 0   1   0   0 | W | R | X | B |
 * +---+---+---+---+---+---+---+---+
 * \------ 4 ------\ 1 \ 1 \ 1 \ 1 \
 *
 * Table:
 * - W = 0 - default operand size, 1 - 64-bit operand size
 * - R - extension of the ModRM.reg
 * - X - extension of the SIB.index
 * - B - extension of the ModRM.rm, SIB.index, or opcode.reg
 *
 * --------------------- Displacement ---------------------
 * Description:
 * - signed value that is added to the base of CS or to RIP
 *
 * Size:
 * - 1, 2, 4 bytes
 *
 * --------------------- Immediate ---------------------
 * Size:
 * - 1, 2, 4, 8 bytes
 * */

// 0x8d | 00 000 100 | 00 110 111
/* ---^   -^ --^ --^   -^ --^ --^
 *    |    |   |   |    |   |   |
 *    /    |   |   |    |   |   \
 * lea     /   |   |    |   \    rdi
 *    reg-ind  /   |    \    rsi
 *          rax   SIB    scale = 1
 *
 */

// lea eax, [rdi + rsi * 1]