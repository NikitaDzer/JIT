// 
// Created by MIPT student Nikita Dzer on 17.05.2022.
// 

#include <stdlib.h>
#include <stdbool.h>
#include "../include/IR.h"
#include "../include/saver.h"


typedef enum WritingResult
{
    WRITING_SUCCESS = 1,
    WRITING_FAILURE = 2,
} WritingResult;


static const ListNode* linearize_IR (IR *const restrict IR);
static WritingResult   write_IR_file(const char *const restrict IR_file_path,
                                     const IRHeader *const restrict header, const ListNode *const restrict nodes);


/*!
 * @brief  Saves Intermediate Representation to the file
 * @param  IR            List of intermediate instructions
 * @param  IR_file_path  Path to the file where Intermediate Representation will be written
 * @return Result of saving Intermediate Representation
 */
SavingResult save_IR(IR *const restrict IR, const char *const restrict IR_file_path)
{
    const IRHeader header = { .n_intermediates = IR->size };
    const ListNode *const restrict nodes = linearize_IR(IR);
    if (nodes == NULL)
        return SAVING_FAILURE;
    
    const WritingResult writing_result = write_IR_file(IR_file_path, &header, nodes); free(nodes);
    if (writing_result == WRITING_FAILURE)
        return SAVING_FAILURE;
    
    return SAVING_SUCCESS;
}



static inline bool is_jump_or_call(const IntermediateOpcode opcode)
{
    switch (opcode)
    {
        case CALL:
        case JMP:
        case JE:
        case JA:
        case JE_O0:
        case JA_O0: return true;
        
        default: return false;
    }
}

static const ListNode* linearize_IR(IR *const restrict IR)
{
    ListNode *const restrict nodes      = calloc(IR->size, sizeof(ListNode));
    ListNode *restrict       nodes_free = nodes;
    if (nodes == NULL)
        return NULL;
    
    list_index_t *const restrict legacy_indexes      = calloc(IR->size, sizeof(list_index_t));
    list_index_t *restrict       legacy_indexes_free = legacy_indexes;
    if (legacy_indexes == NULL)
    {
        free(nodes);
        return NULL;
    }
    
    struct UnresolvedReferences
    {
        unsigned long long reference;
        list_index_t index;
    }
    *const restrict references = calloc(IR->size, sizeof(struct UnresolvedReferences)),
    *restrict references_free  = references;
    
    if (references == NULL)
    {
        free(nodes);
        free(legacy_indexes);
        return NULL;
    }
    
    // -v- copy used nodes from IR -v-
    for (list_index_t iterator = list_reset_iterator(IR), index = 0;
         iterator != 0;
         iterator = list_iterate_forward(IR), index++)
    {
        const Intermediate *const restrict intermediate = &IR->nodes[iterator].item;
        
        *legacy_indexes_free++ = iterator;
        *nodes_free++          = (ListNode){ .prev = index,
                                             .next = index + 2,
                                             .item = *intermediate };
        
        if (is_jump_or_call(intermediate->opcode))
            *references_free++ = (struct UnresolvedReferences){ .reference = intermediate->argument1.reference,
                                                                .index     = index };
    }
    
    // -v- resolve references -v-
    const list_index_t n_references = references_free - references;
    const list_index_t IR_size      = IR->size;
    
    for (list_index_t i = 0; i < n_references; i++)
    {
        const unsigned long long reference = references[i].reference;
        const list_index_t       index     = references[i].index;
        
        for (list_index_t j = 0; j < IR_size; j++)
            if (legacy_indexes[j] == reference)
            {
                nodes[index].item.argument1.reference = j + 1;
                break;
            }
    }
    
    nodes[IR_size - 1].next = 0; // make list cyclical
    
    free(legacy_indexes);
    free(references);
    
    return nodes;
}

static WritingResult write_IR_file(const char *const restrict IR_file_path,
                                   const IRHeader *const restrict header, const ListNode *const restrict nodes)
{
    FILE *const restrict IR_file = fopen(IR_file_path, "wb");
    if (IR_file == NULL)
        return WRITING_FAILURE;
    
    fwrite(header, sizeof(IRHeader), 1,                       IR_file);
    fwrite(nodes,  sizeof(ListNode), header->n_intermediates, IR_file);
    
    fclose(IR_file);
    
    return WRITING_SUCCESS;
}


