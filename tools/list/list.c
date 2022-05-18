//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#include <string.h>
#include <stdlib.h>
#include "../../include/list/list.h"

typedef list_index_t index_t;

static const index_t LIST_NO_FREE       = -1;
static const index_t LIST_DEFAULT_SHIFT =  1;

// ================================  static functions  ============================================
static inline void fill_free(List *const p_list)
{
   ListNode     *const nodes    = p_list->nodes;
   const size_t        capacity = p_list->capacity;
   
   for (index_t i = p_list->size + LIST_DEFAULT_SHIFT; i < capacity - 1; i++)
   {
       nodes[i].next =  i + 1;
       nodes[i].prev = -1;
   }
   
    nodes[capacity - 1].next =  LIST_NO_FREE;
    nodes[capacity - 1].prev = -1;
}

static index_t get_free(List *const p_list)
{
   if (p_list->free >= 1)
      return p_list->free;
      
   if (p_list->capacity == LIST_MAX_CAPACITY)
      return LIST_NO_FREE;
      
   if (p_list->capacity * 2 > LIST_MAX_CAPACITY)
      p_list->capacity  = LIST_MAX_CAPACITY;
   else
      p_list->capacity *= 2;
   
   p_list->nodes = (ListNode *)realloc(p_list->nodes, p_list->capacity * sizeof(ListNode));
   
   fill_free(p_list);
   
   if (p_list->nodes == NULL)
      return LIST_FAULT;
   
   return p_list->size + 1;
}
// ================================ /static functions  ============================================



const list_index_t LIST_FAULT = -1;

// ================================  export functions  ============================================
List* construct_list(const index_t n_items)
{
    const index_t n_nodes = n_items + 1;
    
    List *const p_list  = (List *)calloc(1, sizeof(List));
    if (p_list == NULL)
        return NULL;
    
    ListNode *const nodes   = (ListNode *)calloc(n_nodes, sizeof(ListNode));
    if (nodes == NULL)
    {
        free(p_list);
        return NULL;
    }
    
    for (index_t i = 1; i < n_nodes; i++)
    {
      nodes[i].next =  i + 1;
      nodes[i].prev = -1;
    }
    
    nodes[n_nodes - 1].next = LIST_NO_FREE;
    
    nodes[0].next = 0;
    nodes[0].prev = 0;
    
    p_list->nodes    = nodes;
    p_list->free     = 1;
    p_list->iterator = 0;
    p_list->size     = 0;
    p_list->capacity = n_nodes;
    
    return p_list;
}

void destruct_list(List *const p_list)
{
    free(p_list->nodes);
    free(p_list);
}


index_t list_pushBack(List *const p_list, const ListItem *const restrict item)
{
   const index_t free = get_free(p_list);
   
   if (free == LIST_FAULT || free == LIST_NO_FREE)
      return LIST_FAULT;
   
   ListNode      *const nodes    = p_list->nodes;
   const index_t        nextFree = nodes[free].next;
   
   nodes[free].item = *item;
   nodes[free].next = 0;
   nodes[free].prev = nodes[0].prev;

   nodes[ nodes[0].prev ].next = free;
   nodes[0].prev               = free;

   p_list->size += 1;
   p_list->free  = nextFree;
   
   return free;
}


void list_delete(List *const p_list, const index_t index)
{
    ListNode *const nodes = p_list->nodes;
    
    nodes[ nodes[index].prev ].next = nodes[index].next;
    nodes[ nodes[index].next ].prev = nodes[index].prev;
    
    nodes[index].next =  p_list->free;
    nodes[index].prev = -1;
    
    p_list->size -= 1;
    p_list->free  = index;
}

inline index_t list_iterate_forward (List *const restrict p_list)
{
    return p_list->iterator = p_list->nodes[p_list->iterator].next;
}

inline index_t list_reset_iterator(List *const restrict p_list)
{
    return p_list->iterator = p_list->nodes[0].next;
}
// ================================ /export functions  ============================================