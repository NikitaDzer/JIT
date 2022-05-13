// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#include "../include/compiler_bytecode_IR.h"
#include "../include/bytecode.h"
#include <stdlib.h>
#include <math.h>



// -> In-file declarations
static ListNode *restrict IR_nodes = NULL;


static inline const BytecodeHeader* read_bytecode_header(const char *const restrict bytecode);

static inline const BytecodeInstruction* get_bytecode_instructions(const char *const restrict bytecode);

static IR* get_IR(const BytecodeInstruction *const restrict instructions, const size_t n_instructions);
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
    
    IR *const restrict IR = get_IR(get_bytecode_instructions(bytecode), header->n_instructions);
    
    free((void *)header);
    
    return IR;
}
// <- Export functions



static const double APPROXIMATION_FACTOR = 1.2; // empirical coefficient


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


static inline bool is_jump_or_call(const IntermediateOpcode opcode)
{
    switch (opcode)
    {
        case O0_JE:
        case O0_JA:
        case O0_JMP:
        case O0_CALL: return true;
    
        default: return false;
    }
}

static inline bool is_intermediate_incorrect(const Intermediate *const restrict intermediate)
{
    if (intermediate->opcode == UNDEFINED_OPCODE)
        return true;
    
    if (intermediate->argument1.type == ARG_TYPE_REG || intermediate->argument1.type == ARG_TYPE_MEM_REG)
        if (intermediate->argument1.registry == UNDEFINED_REGISTRY)
            return true;
    
    if (is_jump_or_call(intermediate->opcode))
        if (intermediate->argument1.reference < &IR_nodes[0].item)
            return true;
    
    return false;
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

static inline IntermediateOpcode get_intermediate_opcode(const BytecodeOpcode opcode)
{
    switch (opcode)
    {
        case BYTECODE_PUSH: return O0_PUSH;
        case BYTECODE_POP:  return O0_POP;
        case BYTECODE_SUM:  return O0_ADD;
        case BYTECODE_SUB:  return O0_SUB;
        case BYTECODE_MUL:  return O0_MUL;
        case BYTECODE_OUT:  return O0_PRINTF;
        case BYTECODE_JMP:  return O0_JMP;
        case BYTECODE_CALL: return O0_CALL;
        case BYTECODE_RET:  return O0_RET;
        
        default: return UNDEFINED_OPCODE;
    }
}

static inline IntermediateArgument get_intermediate_argument(const BytecodeInstruction *const restrict instruction,
                                                             const bool is_jump_or_call)
{
    IntermediateArgument argument = {0};
    
    if (is_jump_or_call)
    {
        argument.type      = ARG_TYPE_REFERENCE;
        argument.reference = &(IR_nodes + (unsigned long long)instruction->argument)->item;
    }
    else
    {
        if (instruction->is_RAM)
        {
            if (instruction->is_registry)
            {
                argument.type     = ARG_TYPE_MEM_REG;
                argument.registry = get_intermediate_registry(instruction->argument);
            }
            else
            {
                argument.type      = ARG_TYPE_MEM_INT;
                argument.iconstant = (long long)instruction->argument;
            }
        }
        else
        {
            if (instruction->is_registry)
            {
                argument.type     = ARG_TYPE_REG;
                argument.registry = get_intermediate_registry(instruction->argument);
            }
            else
            {
                if (floor(instruction->argument) == instruction->argument)
                {
                    argument.type      = ARG_TYPE_INT;
                    argument.iconstant = (long long)instruction->argument;
                }
                else
                {
                    argument.type      = ARG_TYPE_DBL;
                    argument.dconstant = instruction->argument;
                }
            }
        }
    }
    
    return argument;
}

static Intermediate* get_intermediate(const BytecodeInstruction *const restrict instruction)
{
    static Intermediate intermediate = {0};
    
    intermediate.opcode    = get_intermediate_opcode(instruction->opcode);
    intermediate.argument1 = get_intermediate_argument(instruction, is_jump_or_call(intermediate.opcode));
    
    if (is_intermediate_incorrect(&intermediate))
        return NULL;
    
    return &intermediate;
}

static IR* get_IR(const BytecodeInstruction *const restrict instructions, const size_t n_instructions)
{
    IR *const restrict IR = construct_list(calc_IR_size_approximately(n_instructions));
    if (IR == NULL)
        return NULL;
    
    IR_nodes = IR->nodes;
    
    Intermediate *restrict intermediate = NULL;
    
    for (list_index_t i = 0; i < n_instructions; i++)
    {
        intermediate = get_intermediate(instructions + i);
        
        if (intermediate == NULL)
        {
            destruct_list(IR);
            return NULL;
        }
        
        if (list_pushBack(IR, intermediate) == LIST_FAULT)
        {
            destruct_list(IR);
            return NULL;
        }
    }
    
    return IR;
}
