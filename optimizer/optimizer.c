// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#include <stdbool.h>
#include "../include/optimizer.h"



static OptimizationResult optimize_intermediates(IR *const restrict IR);



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


static inline list_index_t optimize_pop(IR *const restrict IR, const list_index_t index)
{
    ListNode *const restrict nodes = IR->nodes;
    
    const list_index_t index_prev = nodes[index].prev;
          list_index_t iterator   = index;
    
    Intermediate *const restrict pop  = &nodes[index     ].item;
    Intermediate *const restrict prev = &nodes[index_prev].item;
    
    Intermediate optimized[3] = {0};
    int          n_optimized  = 0;
    
    
    if (are_arguments_equal(&pop->argument1, &prev->argument1))
    {
        iterator = nodes[index_prev].prev;

        list_delete(IR, index_prev);
        list_delete(IR, index);
        
        return iterator;
    }
    
    if (pop->argument1.type == TYPE_REGISTRY)
    {
        switch (prev->opcode)
        {
            case O0_PUSH:
            {
                OPTIMIZED(0, MOV, pop->argument1, prev->argument1);
                n_optimized = 1;
                
                break;
            }
    
            default:
            {
                return iterator;
            }
        }
    }
    else
    {
        switch (prev->opcode)
        {
            case O0_PUSH:
            {
                if (prev->argument1.type == TYPE_MEM_REGISTRY || prev->argument1.type == TYPE_MEM_RELATIVE)
                {
                    IntermediateRegistry registry = R13;
                    
                    if (pop->argument1.type == TYPE_MEM_REGISTRY && pop->argument1.registry == R13)
                    {
                        if (pop->argument1.registry == R13)
                            registry = R14;
                        
                        if ((prev->argument1.type == TYPE_REGISTRY || prev->argument1.type == TYPE_MEM_REGISTRY)
                             && prev->argument1.registry == R14)
                            registry = R15;
                    }
                    else if (prev->argument1.type == R13)
                        registry = R15;
    
                    OPTIMIZED(0, XCHG, ARGUMENT(TYPE_REGISTRY, registry), pop->argument1);
                    OPTIMIZED(1, MOV,  ARGUMENT(TYPE_REGISTRY, registry), prev->argument1);
                    OPTIMIZED(2, XCHG, ARGUMENT(TYPE_REGISTRY, registry), pop->argument1);
                    
                    n_optimized = 3;
                }
                else // if (prev->argument1.type → {TYPE_REGISTRY, TYPE_INTEGER, TYPE_DOUBLE}
                {
                    OPTIMIZED(0, MOV, pop->argument1, prev->argument2);
                    n_optimized = 1;
                }
    
                break;
            }
    
            default:
            {
                return iterator;
            }
        }
    }
    
    
    *prev = optimized[0];
    
    if (n_optimized == 1)
    {
        iterator = index_prev;
        list_delete(IR, index);
    }
    else // if (n_optimized >= 2)
    {
        *pop = optimized[1];
    
        if (n_optimized == 3)
            iterator = list_insertAfter(IR, &optimized[2], index);
    }
    
    return iterator;
}

static OptimizationResult optimize_intermediates(IR *const restrict IR)
{
    for (list_index_t iterator = list_reset_iterator(IR); iterator != 0; iterator = list_iterate_forward(IR))
    {
        switch (IR->nodes[iterator].item.opcode)
        {
            case O0_POP:
            {
                IR->iterator = optimize_pop(IR, iterator);
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