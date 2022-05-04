// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#include "../include/compiler_bytecode_IR.h"
#include "../include/bytecode.h"
#include <stdlib.h>
#include <math.h>



// -> In-file declarations
static inline const BytecodeHeader* read_bytecode_header(const char *const restrict bytecode);

static inline const BytecodeInstruction* get_bytecode_instructions(const char *const restrict bytecode);

static inline list_index_t calc_IR_size_approximately(const size_t n_instructions);

static Intermediate* get_intermediate(const BytecodeInstruction *const restrict instruction);
// <- In-file declarations



// -> Export functions
IR* compile_bytecode_IR(const char *const restrict bytecode)
{
    const BytecodeHeader *const restrict header = read_bytecode_header(bytecode);
    if (header == NULL)
    {
        return NULL;
    }
    
    if (header->n_instructions == 0)
    {
        // update log
        return NULL;
    }
    
    IR *const restrict IR = construct_list( calc_IR_size_approximately(header->n_instructions) );
    if (IR == NULL)
    {
        free((void *)header);
        return NULL;
    }
    
    Intermediate *restrict intermediate = NULL;
    const BytecodeInstruction *restrict instructions_iterator = get_bytecode_instructions(bytecode);
    const BytecodeInstruction *const restrict stop_pointer = instructions_iterator + header->n_instructions;
    
    for (; instructions_iterator != stop_pointer; instructions_iterator++)
    {
        intermediate = get_intermediate(instructions_iterator);
        
        // Reorganize memory cleaning
        if (intermediate == NULL)
        {
            free((void *)header);
            destruct_list(IR);
            
            return NULL;
        }
        
        if (list_pushBack(IR, intermediate) == LIST_FAULT)
        {
            free((void *)header);
            destruct_list(IR);
    
            return NULL;
        }
    }
    
    free((void *)header);
    
    return IR;
}
// <- Export functions



typedef enum IntermediateStatus
{
    INTERMEDIATE_CORRECT   = 1,
    INTERMEDIATE_INCORRECT = 2
} IntermediateStatus;


static const double APPROXIMATION_FACTOR = 1.2; // empirical coefficient
static unsigned char UNDEFINED_BYTECODE_INSTRUCTION_OPCODE = 255;
static unsigned char UNDEFINED_BYTECODE_INSTRUCTION_REGISTRY = 255;
static unsigned long long INCORRECT_BYTECODE_INSTRUCTION_ARGUMENT = -1ULL; // rename constant


static inline list_index_t calc_IR_size_approximately(const size_t n_instructions)
{
    return n_instructions * APPROXIMATION_FACTOR;
}

static const BytecodeHeader* read_bytecode_header(const char *const restrict bytecode)
{
    BytecodeHeader *const restrict header = calloc(1, sizeof(BytecodeHeader));
    if (header == NULL)
        return NULL;
    
    *header = *(BytecodeHeader *)bytecode;
    
    return header;
}

static inline const BytecodeInstruction* get_bytecode_instructions(const char *const restrict bytecode)
{
    return (BytecodeInstruction *)(bytecode + sizeof(BytecodeHeader));
}

static inline unsigned char get_intermediate_registry(const double bytecode_instruction_argument)
{
    switch ((unsigned char)bytecode_instruction_argument)
    {
        default:
            return UNDEFINED_BYTECODE_INSTRUCTION_REGISTRY;
    }
}

static inline unsigned char get_intermediate_opcode(const char bytecode_instruction_opcode)
{
    switch (bytecode_instruction_opcode)
    {
        case 12:
        {
            return 0x00;
        }
        
        default:
            return UNDEFINED_BYTECODE_INSTRUCTION_OPCODE;
    }
}

static inline unsigned long long get_intermediate_address(const double bytecode_instruction_argument)
{
    if (bytecode_instruction_argument < 0.0)
        return INCORRECT_BYTECODE_INSTRUCTION_ARGUMENT;
    
    if (floor(bytecode_instruction_argument) != bytecode_instruction_argument)
        return INCORRECT_BYTECODE_INSTRUCTION_ARGUMENT;
    
    return (unsigned long long)bytecode_instruction_argument;
}

__attribute__((__always_inline__))
static inline void put_intermediate_argument(IntermediateArgument *const restrict argument,
                                             const BytecodeInstruction *const restrict instruction)
{
    if (instruction->is_RAM)
    {
        if (instruction->is_registry)
        {
            argument->type     = ARG_TYPE_MEM_REG;
            argument->registry = get_intermediate_registry(instruction->argument);
        }
        else
        {
            argument->type    = ARG_TYPE_MEM_INT;
            argument->address = get_intermediate_address(instruction->argument);
        }
    }
    else
    {
        if (instruction->is_registry)
        {
            argument->type     = ARG_TYPE_REG;
            argument->registry = get_intermediate_registry(instruction->argument);
        }
        else
        {
            if (floor(instruction->argument) == instruction->argument)
            {
                argument->type      = ARG_TYPE_INT;
                argument->iconstant = (long long)instruction->argument;
            }
            else
            {
                argument->type      = ARG_TYPE_DBL;
                argument->dconstant = instruction->argument;
            }
        }
    }
}

static inline IntermediateStatus check_intermediate(const Intermediate *const restrict intermediate)
{
    // update log
    
    if (intermediate->opcode == UNDEFINED_BYTECODE_INSTRUCTION_OPCODE)
        return INTERMEDIATE_INCORRECT;
    
    if (intermediate->argument1.type == ARG_TYPE_REG || intermediate->argument1.type == ARG_TYPE_MEM_REG)
        if (intermediate->argument1.registry == UNDEFINED_BYTECODE_INSTRUCTION_REGISTRY)
            return INTERMEDIATE_INCORRECT;
    
    if (intermediate->argument1.type == ARG_TYPE_MEM_INT)
        if (intermediate->argument1.address == INCORRECT_BYTECODE_INSTRUCTION_ARGUMENT)
            return INTERMEDIATE_INCORRECT;
    
    return INTERMEDIATE_CORRECT;
}

static Intermediate* get_intermediate(const BytecodeInstruction *const restrict instruction)
{
    static Intermediate intermediate = {0};
    
    intermediate.opcode = get_intermediate_opcode(instruction->opcode);
    put_intermediate_argument(&intermediate.argument1, instruction);
    
    if (check_intermediate(&intermediate) == INTERMEDIATE_INCORRECT)
        return NULL;
    
    return &intermediate;
}
