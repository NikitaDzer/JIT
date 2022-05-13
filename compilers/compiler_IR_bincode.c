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


static Bincode *restrict bincode = NULL;
static unsigned char *restrict executable_free = NULL;
static const Intermediate *restrict *restrict unresolved_intermediates_free = NULL;


static inline void write_executer_data(void);

static CompilationResult compile_intermediates(IR *const restrict IR);

static void resolve_intermediates(const Intermediate *const restrict *const restrict unresolved_intermediates);


const Bincode* compile_IR_bincode(IR *const restrict IR, size_t *const restrict executable_size)
{
    bincode = allocate_bincode(IR->size);
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
    
    write_executer_data();
    
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

static inline unsigned char set_ext(const IntermediateRegistry registry, const unsigned char value)
{
    return is_new_rq(registry) ? value : 0;
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
                EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | REX_B);
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

static inline void write_add(const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument1.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == ARG_TYPE_REG)
        {
            const IntermediateRegistry registry2 = argument2.registry;
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x03);
            EXECUTABLE_PUSH(int8_t, REGISTER_DIRECT | get_rq(registry1) << 3 | get_rq(argument2.registry));
        }
    }
}

static inline void write_sub(const IntermediateArgument argument1, const IntermediateArgument argument2)
{
    if (argument1.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry1 = argument1.registry;
        
        if (argument2.type == ARG_TYPE_REG)
        {
            const IntermediateRegistry registry2 = argument2.registry;
            
            EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry1, REX_R) | set_ext(registry2, REX_B));
            EXECUTABLE_PUSH(int8_t, 0x2B);
            EXECUTABLE_PUSH(int8_t, REGISTER_DIRECT | get_rq(registry1) << 3 | get_rq(argument2.registry));
        }
    }
}

static inline void write_mul(const IntermediateArgument argument)
{
    if (argument.type == ARG_TYPE_REG)
    {
        const IntermediateRegistry registry = argument.registry;

        EXECUTABLE_PUSH(int8_t, REX_IDENTIFIER | REX_W | set_ext(registry, REX_R));
        EXECUTABLE_PUSH(int8_t, 0xF7);
        EXECUTABLE_PUSH(int8_t, REGISTER_DIRECT | 4 << 3 | get_rq(registry));
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
        EXECUTABLE_PUSH(int8_t, REGISTER_DIRECT | (2 << 3) | get_rq(registry));
    }
}

static inline void write_executer_data()
{
    *(int64_t *)(bincode->data + LLD_SPECIFIER_SHIFT) = 0x000A646C6C25; // "%lld\n"
    *(int64_t *)(bincode->data + LG_SPECIFIER_SHIFT)  = 0x000A676C25;   // "%lg\n"
}


static inline CompilationResult compile_intermediate(Intermediate *const restrict intermediate)
{
    unsigned char *const restrict address = executable_free;
    
    switch (intermediate->opcode)
    {
        case O0_PUSH:
        {
            write_push(intermediate->argument1);
            break;
        }
    
        case O0_POP:
        {
            write_pop(intermediate->argument1);
            break;
        }
        
        case O0_ADD:
        {
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_add(CREATE_ARGUMENT(ARG_TYPE_REG, RAX), CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_push(CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
        
            break;
        }
        
        case O0_MUL:
        {
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_mul(CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_push(CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
        
            break;
        }
        
        case O0_IN:
        {
            break;
        }
        
        case O0_PRINTF:
        {
            write_pop(CREATE_ARGUMENT(ARG_TYPE_REG, RDX));
    
            write_mov(CREATE_ARGUMENT(ARG_TYPE_REG, RBX), CREATE_ARGUMENT(ARG_TYPE_INT, 32));
            write_sub(CREATE_ARGUMENT(ARG_TYPE_REG, RSP), CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_mov(CREATE_ARGUMENT(ARG_TYPE_REG, RCX), CREATE_ARGUMENT(ARG_TYPE_INT, bincode->data + LLD_SPECIFIER_SHIFT));
            write_mov(CREATE_ARGUMENT(ARG_TYPE_REG, RBX), CREATE_ARGUMENT(ARG_TYPE_INT, printf));
    
            write_call(CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
    
            write_mov(CREATE_ARGUMENT(ARG_TYPE_REG, RBX), CREATE_ARGUMENT(ARG_TYPE_INT, 32));
            write_add(CREATE_ARGUMENT(ARG_TYPE_REG, RSP), CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
    
            break;
        }
        
        case O0_JMP:
        {
            const Intermediate *const restrict reference = intermediate->argument1.reference;
        
            EXECUTABLE_PUSH(int8_t, 0xE9);
        
            if (reference->is_compiled)
                EXECUTABLE_PUSH(int32_t, reference->argument1.address - executable_free - 1);
            else
                ADD_UNRESOLVED_INTERMEDIATE();
        
            break;
        }
        
        case O0_JE:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_JA:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_HLT:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_CALL:
        {
            const Intermediate *const restrict reference = intermediate->argument1.reference;
        
            EXECUTABLE_PUSH(int8_t , 0xE8);
        
            if (reference->is_compiled)
                EXECUTABLE_PUSH(int32_t, reference->argument1.address - executable_free - 1);
            else
                ADD_UNRESOLVED_INTERMEDIATE();
        
            break;
        }
        
        case O0_RET:
        {
            EXECUTABLE_PUSH(int8_t, 0xC3);
            break;
        }
        
        case O0_SQRT:
        {
            break;
        }
    
        case O0_SUB:
        {
            write_pop (CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
            write_pop (CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_sub (CREATE_ARGUMENT(ARG_TYPE_REG, RAX), CREATE_ARGUMENT(ARG_TYPE_REG, RBX));
            write_push(CREATE_ARGUMENT(ARG_TYPE_REG, RAX));
        
            break;
        }
        
        case O0_DIV:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_PIX:
        {
            return COMPILATION_FAILURE;
        }
        
        case O0_SHOW:
        {
            return COMPILATION_FAILURE;
        }
        
        /*
            case ADD:
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
             */
        
        default:
        {
            return COMPILATION_FAILURE;
        }
    }
    
    intermediate->argument2.address = address;
    intermediate->is_compiled = 1;
    
    return COMPILATION_SUCCESS;
}

static CompilationResult compile_intermediates(IR *const restrict IR)
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


static void resolve_intermediates(const Intermediate *const restrict *const restrict unresolved_intermediates)
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