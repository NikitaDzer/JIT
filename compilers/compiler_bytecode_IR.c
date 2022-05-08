// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#include "../include/compiler_bytecode_IR.h"
#include "../include/bytecode.h"
#include <stdlib.h>
#include <math.h>



// -> In-file declarations
static ListNode *restrict list_nodes = NULL;


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
    
    list_nodes = IR->nodes;
    
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
static const unsigned char UNDEFINED_BYTECODE_OPCODE = -1;


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

static inline bool is_intermediate_jump_or_call(const unsigned char intermediate_opcode)
{
    switch (intermediate_opcode)
    {
        case 0x03:
        case 0x04: return true;
    
        default: return false;
    }
}

static inline IntermediateRegistry get_intermediate_registry(const double bytecode_registry)
{
    switch ((BytecodeRegistry)bytecode_registry)
    {
        case BYTECODE_RAX: return RAX;
        case BYTECODE_RBX: return RBX;
        case BYTECODE_RCX: return RCX;
        case BYTECODE_RDX: return RDX;
        case BYTECODE_RDI: return RDI;
        case BYTECODE_RSI: return RSI;
        case BYTECODE_RBP: return RBP;
        case BYTECODE_RSP: return RSP;
        case BYTECODE_R8: return  R8;
        case BYTECODE_R9: return  R9;
        case BYTECODE_R10: return R10;
        case BYTECODE_R11: return R11;
        case BYTECODE_R12: return R12;
        case BYTECODE_R13: return R13;
        case BYTECODE_R14: return R14;
        case BYTECODE_R15: return R15;
    
        default: return UNDEFINED_REGISTRY;
    }
}

static inline unsigned char get_intermediate_opcode(const char bytecode_instruction_opcode)
{
    switch (bytecode_instruction_opcode)
    {
        // PUSH
        case 1: return 0x01;
        
        // POP
        case 2: return 0x02;

        // SUM
        case 3: return 0x06;
        
        // SUB
        case 14: return 0x07;
        
        // MUL
        case 4: return 0x08;
        
        // OUT
        case 6: return 0x05;
        
        // JMP
        case 7: return 0x04;
        
        // CALL
        case 11: return 0x03;
        
        // RET
        case 12: return 0x00;
        
        default: return UNDEFINED_BYTECODE_OPCODE;
    }
}

__attribute__((__always_inline__))
static inline void put_intermediate_argument(Intermediate *const restrict intermediate,
                                             const BytecodeInstruction *const restrict instruction)
{
    IntermediateArgument *const restrict argument = &intermediate->argument1;

    if (is_intermediate_jump_or_call(intermediate->opcode))
    {
        argument->type      = ARG_TYPE_REFERENCE;
        argument->reference = &(list_nodes + (unsigned long long)instruction->argument)->item;
    }
    else
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
                argument->type      = ARG_TYPE_MEM_INT;
                argument->iconstant = (long long)instruction->argument;
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
}

static inline IntermediateStatus check_intermediate(const Intermediate *const restrict intermediate)
{
    // update log
    
    if (intermediate->opcode == UNDEFINED_BYTECODE_OPCODE)
        return INTERMEDIATE_INCORRECT;
    
    if (intermediate->argument1.type == ARG_TYPE_REG || intermediate->argument1.type == ARG_TYPE_MEM_REG)
        if (intermediate->argument1.registry == UNDEFINED_REGISTRY)
            return INTERMEDIATE_INCORRECT;
    
    if (is_intermediate_jump_or_call(intermediate->opcode))
        if (intermediate->argument1.reference < &list_nodes[0].item)
            return INTERMEDIATE_INCORRECT;
    
    return INTERMEDIATE_CORRECT;
}

static Intermediate* get_intermediate(const BytecodeInstruction *const restrict instruction)
{
    static Intermediate intermediate = {0};
    
    intermediate.opcode = get_intermediate_opcode(instruction->opcode);
    put_intermediate_argument(&intermediate, instruction);
    
    if (check_intermediate(&intermediate) == INTERMEDIATE_INCORRECT)
        return NULL;
    
    return &intermediate;
}

