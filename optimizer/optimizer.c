// 
// Created by MIPT student Nikita Dzer on 14.05.2022.
// 

#include <stdbool.h>
#include "../include/optimizer.h"

static inline void optimize_arithmetic(IR *const restrict IR, const list_index_t index);
static inline void optimize_move(IR *const restrict IR, const list_index_t index);

void optimize(IR *const restrict IR)
{

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

            
static void optimize_move(IR *const restrict IR, const list_index_t index)
{
    ListNode *const restrict nodes = IR->nodes;
    Intermediate *const restrict pop  = &nodes[index].item;
    Intermediate *const restrict prev = &nodes[ nodes[index].prev ].item;
    Intermediate *const restrict optimized = prev;
    
    if (pop->argument1.type == TYPE_REGISTRY)
    {
        switch (prev->opcode)
        {
            case O0_PUSH:
            {
                optimized->opcode = MOV;
                optimized->argument2 = prev->argument1;
                optimized->argument1 = pop->argument1;
                
                break;
            }
            
        }
    }
}