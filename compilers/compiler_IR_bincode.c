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


static unsigned char *restrict bincode      = NULL;
static unsigned char *restrict bincode_free = NULL;

static const Intermediate *restrict *restrict unresolved_intermediates_free = NULL;


static inline CompilationResult compile_intermediates(IR *const restrict IR);

static inline void resolve_intermediates(const Intermediate *const restrict *const restrict unresolved_intermediates);

const unsigned char* compile_IR_bincode(IR *const restrict IR, size_t *const restrict bincode_size)
{
    bincode = allocate_bincode(IR->size);
    bincode_free = bincode;
    if (bincode == NULL)
        return NULL;
    
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
    
    *bincode_size = bincode_free - bincode;
    
    return bincode;
}


#define BINCODE_PUSH(type, data)                 \
    do {                                         \
        *(type *)(bincode_free) = (type)(data);  \
        bincode_free += sizeof(type);            \
    } while (0)

#define ADD_UNRESOLVED_INTERMEDIATE()                    \
    do {                                                 \
        bincode_free += sizeof(int32_t);                 \
        *unresolved_intermediates_free++ = intermediate; \
    } while (0)
    

static const unsigned char REGISTER_DIRECT    = 0b11000000;
static const unsigned char REGISTER_INDIRECT  = 0b00000000;
static const unsigned char SIB                = 0b00000100;

static const unsigned char REX_B              = 0b00000001;
static const unsigned char REX_IDENTIFIER     = 0b01000000;

static const unsigned char N_RQ_REGISTERS     = 8;
static const unsigned char N_NEW_RQ_REGISTERS = 8;

static const size_t JUMP_OR_CALL_INSTRUCTION_SIZE = sizeof(int32_t) + 1;


static inline unsigned char get_modrm(const unsigned char is_register_direct,
                                      const unsigned char registry,
                                      const unsigned char registry_memory)
{
    return is_register_direct | (registry << 3) | registry_memory;
}

static inline unsigned char get_rq(const unsigned char registry)
{
    switch (registry)
    {
        case 0: return 0;
        case 1: return 3;
        case 2: return 1;
        case 3: return 2;
        case 4: return 5;
        case 5: return 6;
        case 6: return 7;
        case 7: return 4;
    }
}

static inline unsigned char get_new_rq(const unsigned char registry)
{
    return registry - N_RQ_REGISTERS;
}

static inline bool is_dword(const long long iconstant)
{
    return INT32_MIN <= iconstant && iconstant <= INT32_MAX;
}

__attribute__((__always_inline__))
static inline CompilationResult compile_intermediate(Intermediate *const restrict intermediate)
{
    unsigned char *const restrict address = bincode_free;
    
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
                    BINCODE_PUSH(unsigned char, 0x81);
                    BINCODE_PUSH(unsigned char, 0b11000000 | (intermediate->argument1.registry << 3) | 0b00000000);
                    BINCODE_PUSH(long long,     intermediate->argument2.iconstant);
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
            BINCODE_PUSH(unsigned char, 0xC3);
            
            break;
        }
            
            // PUSH
        case 0x01:
        {
            if (intermediate->argument1.type == ARG_TYPE_REG)
            {
                const unsigned char registry = intermediate->argument1.registry;
                
                if (registry < N_RQ_REGISTERS)
                    BINCODE_PUSH(unsigned char, 0x50 + get_rq(registry));
                else
                {
                    BINCODE_PUSH(unsigned char, REX_IDENTIFIER | REX_B);
                    BINCODE_PUSH(unsigned char, 0x50 + get_new_rq(registry));
                }
            }
            else if (intermediate->argument1.type == ARG_TYPE_INT)
            {
                const long long iconstant = intermediate->argument1.iconstant;
                
                /*
                if (is_dword(iconstant))
                {
                    BINCODE_PUSH(unsigned char, 0x68);
                    BINCODE_PUSH(int32_t, (int32_t)iconstant);
                }
                */
                
                BINCODE_PUSH(unsigned char, 0x68);
                BINCODE_PUSH(int32_t, iconstant);
            }
            else if (intermediate->argument1.type == ARG_TYPE_MEM_REG)
            {
                const unsigned char registry = intermediate->argument1.registry;
                
                if (registry < N_RQ_REGISTERS)
                {
                    BINCODE_PUSH(unsigned char, 0xFF);
                    BINCODE_PUSH(unsigned char, (6 << 3) | get_rq(registry));
                }
                else
                {
                    BINCODE_PUSH(unsigned char, REX_IDENTIFIER | REX_B);
                    BINCODE_PUSH(unsigned char, 0xFF);
                    BINCODE_PUSH(unsigned char, (6 << 3) | get_new_rq(registry));
                }
            }
            else
            {
            }
            
            break;
        }
            
            // POP
        case 0x02:
        {
            if (intermediate->argument1.type == ARG_TYPE_REG)
            {
                const unsigned char registry = intermediate->argument1.registry;
                
                if (registry < N_RQ_REGISTERS)
                    BINCODE_PUSH(unsigned char, 0x58 + get_rq(registry));
                else
                {
                    BINCODE_PUSH(unsigned char, REX_IDENTIFIER | REX_B);
                    BINCODE_PUSH(unsigned char, 0x58 + get_new_rq(registry));
                }
            }
            else if (intermediate->argument1.type == ARG_TYPE_MEM_REG)
            {
                const unsigned char registry = intermediate->argument1.registry;
                
                if (registry < N_RQ_REGISTERS)
                {
                    BINCODE_PUSH(unsigned char, 0x8F);
                    BINCODE_PUSH(unsigned char, get_rq(registry));
                }
                else
                {
                    BINCODE_PUSH(unsigned char, REX_IDENTIFIER | REX_B);
                    BINCODE_PUSH(unsigned char, 0x8F);
                    BINCODE_PUSH(unsigned char, get_new_rq(registry));
                }
            }
            
            break;
        }
            
            // CALL
        case 0x03:
        {
            const Intermediate *const restrict reference = intermediate->argument1.reference;
            
            BINCODE_PUSH(unsigned char, 0xE8);
            
            if (reference->is_compiled)
                BINCODE_PUSH(int32_t, reference->argument1.address - bincode_free - 1);
            else
                ADD_UNRESOLVED_INTERMEDIATE();
            
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