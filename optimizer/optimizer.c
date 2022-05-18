// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#include <stdbool.h>
#include "../include/optimizer.h"



static OptimizationResult optimize_intermediates(IR *const restrict IR);


/*!
 * @brief  Optimizes Intermediate Representation
 * @param  IR List of intermediate instructions
 * @return Result of Intermediate Representation optimizations
 */
OptimizationResult optimize(IR *const restrict IR)
{
    const OptimizationResult optimization_result = optimize_intermediates(IR);
    
    return optimization_result;
}



#define ARGUMENT(arg_type, data)                                                \
    (arg_type) == TYPE_REGISTRY ?                                               \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == TYPE_INTEGER ?                                                \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
    (arg_type) == TYPE_MEM_REGISTRY ?                                           \
            (IntermediateArgument){ .type = (arg_type), .registry  = (data) } : \
    (arg_type) == TYPE_MEM_RELATIVE ?                                           \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
    (arg_type) == TYPE_MEM_OFFSET ?                                             \
            (IntermediateArgument){ .type = (arg_type), .iconstant = (data) } : \
            (IntermediateArgument){ .type = (arg_type), .reference = NULL   }

#define OPTIMIZED(optimized_index, opcode_, argument1_, argument2_)     \
            do {                                                        \
                optimized[(optimized_index)].opcode    = (opcode_);     \
                optimized[(optimized_index)].argument1 = (argument1_);  \
                optimized[(optimized_index)].argument2 = (argument2_);  \
            } while (0)
            

static inline bool are_arguments_equal(const IntermediateArgument *const restrict argument1,
                                       const IntermediateArgument *const restrict argument2)
{
    if (argument1->type == argument2->type)
    {
        switch (argument1->type)
        {
            case TYPE_INTEGER:
            case TYPE_MEM_RELATIVE:
            case TYPE_MEM_OFFSET:
            {
                if (argument1->iconstant == argument2->iconstant)
                    return true;
        
                break;
            }
            
            case TYPE_REGISTRY:
            {
                if (argument1->registry == argument2->registry)
                    return true;
                
                break;
            }
            
            case TYPE_DOUBLE:
            {
                if (argument1->dconstant == argument2->dconstant)
                    return true;
                
                break;
            }
            
            case TYPE_MEM_REGISTRY:
            {
                if (argument1->registry == argument2->registry)
                    return true;
                
                break;
            }
            
            case TYPE_REFERENCE:
            {
                if (argument1->reference == argument2->reference)
                    return true;
                
                break;
            }
        }
    }

    return false;
}

static inline bool is_push(const IntermediateOpcode opcode)
{
    return opcode == RELATIVE_PUSH || opcode == PUSH;
}


static inline list_index_t optimize_addsd(IR *const restrict IR, const list_index_t index)
{
    ListNode *const restrict nodes = IR->nodes;
    
    const list_index_t index_prev      = nodes[index     ].prev;
    const list_index_t index_prev_prev = nodes[index_prev].prev;
          list_index_t iterator        = index;
    
    Intermediate *const restrict addsd     = &nodes[index          ].item;
    Intermediate *const restrict prev      = &nodes[index_prev     ].item;
    Intermediate *const restrict prev_prev = &nodes[index_prev_prev].item;
    
    Intermediate optimized[2] = {0};
    int          n_optimized  = 0;
    
    switch (prev->opcode)
    {
        /*
        case RELATIVE_PUSH:
        {
            if (prev_prev->opcode == PUSH)
            {
                OPTIMIZED(0, RELATIVE_PUSH, prev->argument1,                  ARGUMENT(TYPE_INTEGER, 0));
                OPTIMIZED(1, ADD,           ARGUMENT(TYPE_MEM_REGISTRY, RSP), prev_prev->argument1);
                n_optimized = 2;
            }
            
            break;
        }
        */
        
        case PUSH:
        {
            if (prev->argument1.type == TYPE_DOUBLE && prev_prev->opcode == PUSH && prev_prev->argument1.type == TYPE_DOUBLE)
            {
                OPTIMIZED(0, PUSH,
                          ARGUMENT(TYPE_DOUBLE,  prev->argument1.dconstant + prev_prev->argument1.dconstant),
                          ARGUMENT(TYPE_INTEGER, 0));
    
                n_optimized = 1;
            }
            
            /*
            if (prev_prev->opcode == RELATIVE_PUSH || prev_prev->opcode == PUSH)
            {
                OPTIMIZED(0, prev_prev->opcode, prev_prev->argument1,             ARGUMENT(TYPE_INTEGER, 0));
                OPTIMIZED(1, ADD,               ARGUMENT(TYPE_MEM_REGISTRY, RSP), prev->argument1);
                n_optimized = 2;
            }
             */
            
            break;
        }
        
        default:
        {
            break;
        }
    }
    
    if (n_optimized >= 1)
    {
        *prev_prev = optimized[0];
        list_delete(IR, index);
    
        if (n_optimized == 1)
        {
            iterator = index_prev_prev;
            list_delete(IR, index_prev);
        }
        else // if (n_optimized == 2)
        {
            *prev = optimized[1];
            iterator = index_prev;
        }
    }
    
    return iterator;
}

static inline list_index_t optimize_subsd(IR *const restrict IR, const list_index_t index)
{
    ListNode *const restrict nodes = IR->nodes;
    
    const list_index_t index_prev      = nodes[index     ].prev;
    const list_index_t index_prev_prev = nodes[index_prev].prev;
          list_index_t iterator        = index;
    
    Intermediate *const restrict sub       = &nodes[index          ].item;
    Intermediate *const restrict prev      = &nodes[index_prev     ].item;
    Intermediate *const restrict prev_prev = &nodes[index_prev_prev].item;
    
    Intermediate optimized[3] = {0};
    int          n_optimized  = 0;
    
    if (is_push(prev->opcode) && is_push(prev_prev->opcode) && are_arguments_equal(&prev->argument1, &prev_prev->argument1))
    {
        OPTIMIZED(0, PUSH, ARGUMENT(TYPE_INTEGER, 0), ARGUMENT(TYPE_INTEGER, 0));
        n_optimized = 1;
    }
    else
    {
        switch (prev->opcode)
        {
            /*
            case RELATIVE_PUSH:
            {
                if (prev_prev->opcode == PUSH)
                {
                    OPTIMIZED(0, RELATIVE_PUSH, prev->argument1,                  ARGUMENT(TYPE_INTEGER, 0));
                    OPTIMIZED(1, SUB,           ARGUMENT(TYPE_MEM_REGISTRY, RSP), prev_prev->argument1);
                    n_optimized = 2;
                }
            
                break;
            }
            */
            
            case PUSH:
            {
                if (prev->argument1.type == TYPE_DOUBLE && prev_prev->opcode == PUSH && prev_prev->argument1.type == TYPE_DOUBLE)
                {
                    OPTIMIZED(0, PUSH,
                              ARGUMENT(TYPE_DOUBLE,  prev->argument1.dconstant - prev_prev->argument1.dconstant),
                              ARGUMENT(TYPE_INTEGER, 0));
                
                    n_optimized = 1;
                }
                /*
                else if (prev_prev->opcode == RELATIVE_PUSH)
                {
                    OPTIMIZED(0, RELATIVE_PUSH, prev_prev->argument1,             ARGUMENT(TYPE_INTEGER, 0));
                    OPTIMIZED(1, SUB,           ARGUMENT(TYPE_MEM_REGISTRY, RSP), prev->argument1);
                    OPTIMIZED(2, NEG,           ARGUMENT(TYPE_MEM_REGISTRY, RSP), ARGUMENT(TYPE_INTEGER, 0));
                    n_optimized = 3;
                }
                else if (prev_prev->opcode == PUSH)
                {
                    OPTIMIZED(0, PUSH, prev->argument1,                  ARGUMENT(TYPE_INTEGER, 0));
                    OPTIMIZED(1, SUB,  ARGUMENT(TYPE_MEM_REGISTRY, RSP), prev_prev->argument1);
                    n_optimized = 2;
                }
                */
                
                break;
            }
        
            default:
            {
                break;
            }
        }
    }
    
    
    if (n_optimized >= 1)
    {
        *prev_prev = optimized[0];
        list_delete(IR, index);
        
        if (n_optimized == 1)
        {
            iterator = index_prev_prev;
            list_delete(IR, index_prev);
        }
        else if (n_optimized == 2)
        {
            *prev = optimized[1];
            iterator = index_prev;
        }
        else // if (n_optimized == 3)
        {
            *prev = optimized[1];
            *sub  = optimized[2];
        }
    }
    
    return iterator;
}

static inline list_index_t optimize_pop  (IR *const restrict IR, const list_index_t index)
{
    ListNode *const restrict nodes = IR->nodes;
    
    const list_index_t index_prev = nodes[index].prev;
          list_index_t iterator   = index;
    
    Intermediate *const restrict pop  = &nodes[index     ].item;
    Intermediate *const restrict prev = &nodes[index_prev].item;
    
    Intermediate optimized[3] = {0};
    int          n_optimized  = 0;
    
    if (is_push(prev->opcode) && are_arguments_equal(&pop->argument1, &prev->argument1))
    {
        iterator = nodes[index_prev].prev;

        list_delete(IR, index_prev);
        list_delete(IR, index);
        
        return iterator;
    }
    
    if (pop->opcode == RELATIVE_POP)
    {
        switch (prev->opcode)
        {
            case PUSH:
            {
                OPTIMIZED(0, RELATIVE_MOV, pop->argument1, prev->argument1);
                n_optimized = 1;
                
                break;
            }
            
            default:
            {
                break;
            }
        }
    }
    else // if (pop->opcode == POP)
    {
        switch (prev->opcode)
        {
            case RELATIVE_PUSH:
            {
                OPTIMIZED(0, RELATIVE_MOV, pop->argument1, prev->argument1);
                n_optimized = 1;
                
                break;
            }
            case PUSH:
            {
                OPTIMIZED(0, MOV, pop->argument1, prev->argument1);
                n_optimized = 1;
                
                break;
            }
    
            default:
            {
                break;
            }
        }
    }
    
    if (n_optimized >= 1)
    {
        *prev = optimized[0];
    
        if (n_optimized == 1)
        {
            iterator = index_prev;
            list_delete(IR, index);
        }
        else // if (n_optimized == 2)
            *pop = optimized[1];
    }
    
    return iterator;
}


static OptimizationResult optimize_intermediates(IR *const restrict IR)
{
    list_index_t iterator = list_reset_iterator(IR); // start optimizations from the second intermediate
    
    for (iterator = list_iterate_forward(IR); iterator != 0; iterator = list_iterate_forward(IR))
    {
        switch (IR->nodes[iterator].item.opcode)
        {
            case POP:
            case RELATIVE_POP:
            {
                IR->iterator = optimize_pop(IR, iterator);
                break;
            }
    
            case ADDSD_O0:
            {
                IR->iterator = optimize_addsd(IR, iterator);
                break;
            }
            
            case SUBSD_O0:
            {
                 IR->iterator = optimize_subsd(IR, iterator);
                break;
            }
            
            default:
            {
                break;
            }
        }
        
        if (iterator == LIST_FAULT)
            return OPTIMIZATION_FAILURE;
    }
    
    return OPTIMIZATION_SUCCESS;
}