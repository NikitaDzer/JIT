// 
// Created by MIPT student Nikita Dzer on 26.04.2022.
// 

#include "../include/compiler_bytecode_IR.h"
#include "../include/bytecode.h"
#include <stdlib.h>
#include <math.h>



static ListNode *restrict IR_nodes = NULL;


static inline const BytecodeHeader*      read_bytecode_header(const char *const restrict bytecode);
static inline const BytecodeInstruction* get_bytecode_instructions(const char *const restrict bytecode);
static IR* get_IR(const BytecodeInstruction *const restrict instructions, const size_t n_instructions);


/*!
 * @brief  Compiles Processor bytecode to Intermediate Representation
 * @param  bytecode Buffer with bytecode
 * @return Pointer to list of intermediate instructions
 */
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



static const double APPROXIMATION_FACTOR = 1.5; // empirical coefficient for calculating IR size


static inline list_index_t calc_IR_size_approximately(const size_t n_instructions)
{
    return n_instructions * APPROXIMATION_FACTOR;
}

static inline const BytecodeHeader* read_bytecode_header(const char *const restrict bytecode)
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


static inline bool is_jump_or_call(const BytecodeOpcode opcode)
{
    switch (opcode)
    {
        case BYTECODE_JE:
        case BYTECODE_JA:
        case BYTECODE_JMP:
        case BYTECODE_CALL: return true;
    
        default: return false;
    }
}

static inline bool is_intermediate_incorrect(const Intermediate *const restrict intermediate)
{
    if (intermediate->opcode == UNDEFINED_OPCODE)
        return true;
    
    if (intermediate->argument1.type == TYPE_REGISTRY || intermediate->argument1.type == TYPE_MEM_REGISTRY)
        if (intermediate->argument1.registry == UNDEFINED_REGISTRY)
            return true;
    
    if (intermediate->argument1.type == TYPE_REFERENCE)
        if (intermediate->argument1.reference < &IR_nodes[0].item)
            return true;
    
    return false;
}


static inline IntermediateOpcode get_intermediate_opcode(const BytecodeInstruction *const restrict instruction)
{
    switch (instruction->opcode)
    {
        case BYTECODE_PUSH: return instruction->is_RAM ? RELATIVE_PUSH : PUSH;
        case BYTECODE_POP:  return instruction->is_RAM ? RELATIVE_POP  : POP;
        case BYTECODE_SUM:  return ADDSD_O0;
        case BYTECODE_SUB:  return SUBSD_O0;
        case BYTECODE_MUL:  return MULSD_O0;
        case BYTECODE_IN:   return IN;
        case BYTECODE_OUT:  return OUT;
        case BYTECODE_JMP:  return JMP;
        case BYTECODE_JE:   return JE_O0;
        case BYTECODE_JA:   return JA_O0;
        case BYTECODE_HLT:  return HLT;
        case BYTECODE_CALL: return CALL;
        case BYTECODE_RET:  return RET;
        case BYTECODE_DIV:  return DIVSD_O0;
        case BYTECODE_SQRT: return SQRTSD_O0;
        
        default: return UNDEFINED_OPCODE;
    }
}

static inline IntermediateRegistry get_intermediate_gpr(const BytecodeRegistry registry)
{
    switch (registry)
    {
        case BYTECODE_RAX: return RAX;
        case BYTECODE_RBX: return RBX;
        case BYTECODE_RCX: return RCX;
        case BYTECODE_RDX: return RDX;
        case BYTECODE_RDI: return RDI;
        case BYTECODE_RSI: return RSI;
        case BYTECODE_RBP: return RBP;
        case BYTECODE_RSP: return RSP;
        case BYTECODE_R8:  return R8;
        case BYTECODE_R9:  return R9;
        case BYTECODE_R10: return R10;
        case BYTECODE_R11: return R11;
        case BYTECODE_R12: return R12;
        case BYTECODE_R13: return R13;
        case BYTECODE_R14: return R14;
        case BYTECODE_R15: return R15;
        
        default: return UNDEFINED_REGISTRY;
    }
}

static inline IntermediateRegistry get_intermediate_fpr(const BytecodeRegistry registry)
{
    switch (registry)
    {
        case BYTECODE_RAX: return XMM0;
        case BYTECODE_RBX: return XMM1;
        case BYTECODE_RCX: return XMM2;
        case BYTECODE_RDX: return XMM3;
        case BYTECODE_RDI: return XMM4;
        case BYTECODE_RSI: return XMM5;
        case BYTECODE_RBP: return XMM6;
        case BYTECODE_RSP: return XMM7;
        case BYTECODE_R8:  return XMM8;
        case BYTECODE_R9:  return XMM9;
        case BYTECODE_R10: return XMM10;
        case BYTECODE_R11: return XMM11;
        case BYTECODE_R12: return XMM12;
        case BYTECODE_R13: return XMM13;
        case BYTECODE_R14: return XMM14;
        case BYTECODE_R15: return XMM15;
        
        default: return UNDEFINED_REGISTRY;
    }
}


static inline IntermediateArgument get_intermediate_argument(const BytecodeInstruction *const restrict instruction)
{
    IntermediateArgument argument = {0};
    
    if (is_jump_or_call(instruction->opcode))
    {
        argument.type      = TYPE_REFERENCE;
        argument.reference = &(IR_nodes + 1 + (unsigned long long)instruction->argument)->item;
        // ------------------------------ ^ ---------------------------------------------------
        // ---- skip first node which points to head and tail of list -------------------------
    }
    else
    {
        if (instruction->is_RAM)
        {
            if (instruction->is_registry)
            {
                argument.type     = TYPE_MEM_REGISTRY;
                argument.registry = get_intermediate_gpr(instruction->argument);
            }
            else
            {
                argument.type      = TYPE_MEM_RELATIVE;
                argument.iconstant = (unsigned long long)instruction->argument * sizeof(double);
            }
        }
        else
        {
            if (instruction->is_registry)
            {
                argument.type     = TYPE_REGISTRY;
                argument.registry = get_intermediate_fpr(instruction->argument);
            }
            else
            {
                argument.type      = TYPE_DOUBLE;
                argument.dconstant = instruction->argument;
            }
        }
    }
    
    return argument;
}

static Intermediate* get_intermediate(const BytecodeInstruction *const restrict instruction)
{
    static Intermediate intermediate = {0};
    
    intermediate.opcode    = get_intermediate_opcode(instruction);
    intermediate.argument1 = get_intermediate_argument(instruction);
    
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
