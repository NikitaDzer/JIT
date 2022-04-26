//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../../include/list/list.h"

typedef list_index_t index_t;

#ifdef    LIST_ANTI_FOOL
static const index_t LIST_OUTSIDE   = -1;
static const index_t LIST_OK_CYCLE  =  1;
static const index_t LIST_BAD_CYCLE =  2;

static const char LIST_DIRECTION_FORWARD  =  0;
static const char LIST_DIRECTION_BACKWARD =  1;
#endif // LIST_ANTI_FOOL

static const index_t LIST_NO_FREE       = -1;
static const index_t LIST_DEFAULT_SHIFT =  1;

// ================================  static functions  ============================================
static inline bool is_in_range(const index_t index, const index_t capacity)
{
   return 0 <= index && index <= (capacity - 1);
}

static inline void fill_free(List *const p_list)
{
   assert(p_list);
   
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

static void swap_nodes(ListNode *const nodes, const index_t firstIndex, const index_t secondIndex)
{
   ListNode copyNode = {0};
   
   nodes[ nodes[secondIndex].next ].prev = firstIndex;
   nodes[ nodes[secondIndex].prev ].next = firstIndex;
   
   copyNode           = nodes[firstIndex ];
   nodes[firstIndex ] = nodes[secondIndex];
   nodes[secondIndex] = copyNode;
   
   nodes[ nodes[secondIndex].next ].prev = secondIndex;
   nodes[ nodes[secondIndex].prev ].next = secondIndex;
}

#ifdef    LIST_ANTI_FOOL
static inline index_t tortoise_move(const List *const p_list, const index_t start, const int_fast8_t DIRECTION)
{
   if (DIRECTION == LIST_DIRECTION_FORWARD)
      return p_list->nodes[start].next;
   else // (DIRECTION == LIST_DIRECTION_BACKWARD)
      return p_list->nodes[start].prev;
}

static inline index_t hare_move(const List *const p_list, const index_t start, const int_fast8_t DIRECTION)
{
   const ListNode *const nodes = p_list->nodes;
   
   if (DIRECTION == LIST_DIRECTION_FORWARD)
      return
              is_in_range(nodes[start].next, p_list->capacity)
              ? nodes[ nodes[start].next ].next
              : LIST_OUTSIDE;
   else // (DIRECTION == LIST_DIRECTION_BACKWARD)
      return
              is_in_range(nodes[start].prev, p_list->capacity)
              ? nodes[ nodes[start].prev ].prev
              : LIST_OUTSIDE;
}

static index_t check_list_cycle(const List *const p_list, const int_fast8_t DIRECTION)
{
   const ListNode *const nodes    = p_list->nodes;
   const index_t         capacity = p_list->capacity;
   
   index_t tortoise = 0;
   index_t hare     = 0;
   
   if (p_list->size == 0 && !(nodes[0].next == 0 && nodes[0].prev == 0))
      return LIST_OUTSIDE;
   
   for (index_t counter = 0; counter < p_list->size; counter++)
   {
      tortoise = tortoise_move(p_list, tortoise, DIRECTION);
      hare     = hare_move(p_list, hare, DIRECTION);

//      printf("%2lld: TORT %-3lld HARE %-3lld",
//             counter, tortoise, hare);
      
      if (!is_in_range(tortoise, capacity) || !is_in_range(hare, capacity))
         return LIST_OUTSIDE;
      
      if (tortoise == hare)
         return LIST_BAD_CYCLE;
   }
   
   return LIST_OK_CYCLE;
}

static index_t verify_list(const List *const p_list)
{
   if (p_list->nodes == nullptr)
      return LIST_FAULT;
   
   if (p_list->capacity == 0)
      return LIST_FAULT;
   
   if (p_list->nodes[0].item != 0)
      return LIST_FAULT;
   
   if ( !(check_list_cycle(p_list, LIST_DIRECTION_FORWARD)  == LIST_OK_CYCLE
       && check_list_cycle(p_list, LIST_DIRECTION_BACKWARD) == LIST_OK_CYCLE))
      return LIST_FAULT;
   
   return 0;
}
#endif // LIST_ANTI_FOOL
// ================================ /static functions  ============================================


// ================================  export functions  ============================================
index_t list_construct(List *const p_list, const index_t n_items)
{
    #ifdef    LIST_ANTI_FOOL
    if (p_list == nullptr)
      return LIST_FAULT;
    
    #ifdef   LIST_LOGIC_INDEX
    if (p_list->shift != 0)
      return LIST_FAULT; //!TODO enum error
    #endif // LIST_LOGIC_INDEX
    
    if (n_items + 1 > LIST_MAX_CAPACITY || n_items < 2)
      return LIST_FAULT;
    #endif // LIST_ANTI_FOOL
    
    const index_t        n_nodes = n_items + 1;
    ListNode      *const nodes   = (ListNode *)calloc(n_nodes, sizeof(ListNode));
    
    if (nodes == NULL)
      return LIST_FAULT;
    
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


    #ifdef    LIST_LOGIC_INDEX
    p_list->shift    = 1;
    #endif // LIST_LOGIC_INDEX
    
    return p_list->free;
}

void list_destruct(List *const p_list)
{
   #ifdef LIST_ANTI_FOOL
   if (p_list != nullptr)
   {
      free(p_list->nodes);

      p_list->free  = 0;
      p_list->capacity = 0;
      p_list->nodes = nullptr;
      p_list->size  = 0;

      #ifdef    LIST_LOGIC_INDEX
      p_list->shift = 0;
      #endif // LIST_LOGIC_INDEX
   }
   #else //  !LIST_ANTI_FOOL
   free(p_list->nodes);
   
   p_list->free  = 0;
   p_list->capacity = 0;
   p_list->nodes = NULL;
   p_list->size  = 0;
   p_list->iterator = 0;
   
   #ifdef    LIST_LOGIC_INDEX
   p_list->shift = 0;
   #endif // LIST_LOGIC_INDEX
   #endif // !LIST_ANTI_FOOL
}


index_t list_insertAfter(List *const p_list, const ListItem item, const index_t index)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   
   if (!is_in_range(index, p_list->capacity))
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL

   #ifdef    LIST_EXTENSIBLE
   const index_t free = get_free(p_list);
   
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #else //  !LIST_EXTENSIBLE
   const index_t free = p_list->free;
   
   #ifdef    LIST_ANTI_FOOL
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   #endif // !LIST_EXTENSIBLE
   
   ListNode      *const nodes    = p_list->nodes;
   const index_t        nextFree = nodes[free].next;
   
   nodes[free].item = item;
   nodes[free].next = nodes[index].next;
   nodes[free].prev = index;
   
   nodes[ nodes[index].next ].prev = free;
   nodes[index].next               = free;
   
   p_list->size += 1;
   p_list->free  = nextFree;

   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 2)
   {
       if (nodes[0].next == free)
         p_list->shift -= 1;
       else
         p_list->shift  = 0;
   }
   else if (p_list->shift == 1)
   {
      if (nodes[0].prev != free)
         p_list->shift = 0;
   }
   #endif // LIST_LOGIC_INDEX
   
   return free;
}

index_t list_insertBefore(List *const p_list, const ListItem item, const index_t index)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   
   if (!is_in_range(index, p_list->capacity))
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL

   #ifdef    LIST_EXTENSIBLE
   const index_t free = get_free(p_list);
   
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #else //  !LIST_EXTENSIBLE
   const index_t free = p_list->free;
   
   #ifdef    LIST_ANTI_FOOL
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   #endif // !LIST_EXTENSIBLE
   
   ListNode      *const nodes    = p_list->nodes;
   const index_t        nextFree = nodes[free].next;
   
   nodes[free].item = item;
   nodes[free].next = index;
   nodes[free].prev = nodes[index].prev;
   
   nodes[ nodes[index].prev ].next = free;
   nodes[index].prev               = free;
   
   p_list->size += 1;
   p_list->free  = nextFree;
   
   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 2)
   {
      if (nodes[0].next == free)
         p_list->shift -= 1;
      else
         p_list->shift  = 0;
   }
   else if (p_list->shift == 1)
   {
      if (nodes[0].prev != free)
         p_list->shift = 0;
   }
   #endif // LIST_LOGIC_INDEX
   
   return free;
}

index_t list_pushBack(List *const p_list, const ListItem item)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   
   #ifdef    LIST_EXTENSIBLE
   const index_t free = get_free(p_list);
   
   if (free == LIST_FAULT || free == LIST_NO_FREE)
      return LIST_FAULT;
   #else //  !LIST_EXTENSIBLE
   const index_t free = p_list->free;
   
   #ifdef    LIST_ANTI_FOOL
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   #endif // !LIST_EXTENSIBLE
   
   ListNode      *const nodes    = p_list->nodes;
   const index_t        nextFree = nodes[free].next;
   
   nodes[free].item = item;
   nodes[free].next = 0;
   nodes[free].prev = nodes[0].prev;

   nodes[ nodes[0].prev ].next = free;
   nodes[0].prev               = free;

   p_list->size += 1;
   p_list->free  = nextFree;

   // ???????????????????
   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 2)
      p_list->shift = 0;
   #endif // LIST_LOGIC_INDEX
   
   return free;
}

index_t list_pushFront(List *const p_list, const ListItem item)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   
   #ifdef    LIST_EXTENSIBLE
   const index_t free     = get_free(p_list);
   
   if (free == LIST_FAULT || free == LIST_NO_FREE)
      return LIST_FAULT;
   #else //  !LIST_EXTENSIBLE
   const index_t free     = p_list->free;
   
   #ifdef    LIST_ANTI_FOOL
   if (free == LIST_NO_FREE)
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   #endif // !LIST_EXTENSIBLE
   
   ListNode      *const nodes    = p_list->nodes;
   const index_t        nextFree = nodes[free].next;

   nodes[free].item = item;
   nodes[free].next = nodes[0].next;
   nodes[free].prev = 0;

   nodes[ nodes[0].next ].prev = free;
   nodes[0].next               = free;

   p_list->size += 1;
   p_list->free  = nextFree;

   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 2)
      p_list->shift -= 1;
   else
      p_list->shift  = 0;
   #endif // LIST_LOGIC_INDEX
   
   return free;
}

index_t list_popBack(List *const p_list, ListItem *const p_output)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr || p_output == nullptr)
      return LIST_FAULT;

   if (p_list->size == 0)
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL

   ListNode      *const nodes = p_list->nodes;
   const index_t        index = p_list->nodes[0].prev;

  *p_output = nodes[index].item;

   nodes[ nodes[index].prev ].next = 0;
   nodes[0].prev                   = nodes[index].prev;

   nodes[index].next =  p_list->free;
   nodes[index].prev = -1;

   p_list->size -= 1;
   p_list->free  = index;
   
   // ???????????????????
   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 2)
      p_list->shift = 0;
   #endif // LIST_LOGIC_INDEX
   
   return index;
}

index_t list_popFront(List *const p_list, ListItem *const p_output)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr || p_output == nullptr)
      return LIST_FAULT;

   if (p_list->size == 0)
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL

   ListNode      *const nodes = p_list->nodes;
   const index_t        index = p_list->nodes[0].next;

  *p_output = nodes[index].item;

   nodes[0].next                   = nodes[index].next;
   nodes[ nodes[index].next ].prev = 0;

   nodes[index].next = p_list->free;
   nodes[index].prev = -1;

   p_list->size -= 1;
   p_list->free  = index;

   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 1)
      p_list->shift += 1;
   #endif // LIST_LOGIC_INDEX
   
   return index;
}

index_t list_extract(List *const p_list, ListItem *const p_output, const index_t index)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr || p_output == nullptr)
      return LIST_FAULT;
   
   if (p_list->size == 0)
      return LIST_FAULT;
   
   if ( !(1 <= index && index <= p_list->capacity - 1) )
      return LIST_FAULT;
   
   if (p_list->nodes[index].prev == -1)
      return LIST_FAULT;
   
   if (verify_list(p_list))
      return LIST_FAULT;
   #endif
   
   ListNode *const nodes = p_list->nodes;
   
  *p_output = nodes[index].item;
   
   nodes[ nodes[index].prev ].next = nodes[index].next;
   nodes[ nodes[index].next ].prev = nodes[index].prev;
   
   nodes[index].next =  p_list->free;
   nodes[index].prev = -1;
   
   p_list->size -= 1;
   p_list->free  = index;

   #ifdef    LIST_LOGIC_INDEX
   if (p_list->shift >= 1 && nodes[0].prev != index)
   {
      if (nodes[0].next == index)
         p_list->shift += 1;
      else
         p_list->shift  = 0;
   }
   #endif // LIST_LOGIC_INDEX
   
   return index;
}


void list_bind(List *const p_list, const index_t firstIndex, const index_t secondIndex)
{
    ListNode *const nodes = p_list->nodes;
    
    const index_t secondIndex_prev = nodes[secondIndex].prev;
    const index_t firstIndex_next  = nodes[firstIndex ].next;
    
    for (index_t index = firstIndex_next; index != secondIndex; index = nodes[index].next)
    {
        nodes[index].prev = -1;
        p_list->size -= 1;
    }
    
    nodes[firstIndex ].next = secondIndex;
    nodes[secondIndex].prev = firstIndex;
    
    nodes[secondIndex_prev].next = p_list->free;
    p_list->free = firstIndex_next;
}

inline index_t list_iterate_forward (List *const restrict p_list)
{
    return p_list->iterator = p_list->nodes[p_list->iterator].next;
}

inline index_t list_iterate_backward(List *const restrict p_list)
{
    return p_list->iterator = p_list->nodes[p_list->iterator].prev;
}

inline index_t list_reset_iterator(List *const restrict p_list)
{
    return p_list->iterator = p_list->nodes[0].next;
}

#ifdef    LIST_LOGIC_INDEX
index_t list_sort_XXX_THE_FASTEST_SORT_IN_THE_WORLD(List *const p_list)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   
   
   if (p_list->shift == 0)
   {
      ListNode *const nodes = p_list->nodes;
      
      for (index_t shiftedIndex = 0 + LIST_DEFAULT_SHIFT, physIndex = nodes[0].next;
           shiftedIndex <= p_list->size;
           shiftedIndex++)
      {
         if (nodes[shiftedIndex].prev == -1)
         {
            nodes[shiftedIndex]                    = nodes[physIndex];
            nodes[ nodes[shiftedIndex].next ].prev = shiftedIndex;
            nodes[ nodes[shiftedIndex].prev ].next = shiftedIndex;
         }
         else if (shiftedIndex != physIndex)
            swap_nodes(nodes, shiftedIndex, physIndex);
         
         physIndex = nodes[shiftedIndex].next;
      }
   
      fill_free(p_list);
      
      p_list->shift = 1;
   }
   
   return p_list->shift;
}

index_t list_find_physIndex(const List *const p_list, const index_t logicIndex)
{
   #ifdef    LIST_ANTI_FOOL
   if (p_list == nullptr)
      return LIST_FAULT;
   
   if ( !(0 <= logicIndex && logicIndex <= p_list->size - 1) )
      return LIST_FAULT;
   #endif // LIST_ANTI_FOOL
   
   if (p_list->shift >= 1)
      return logicIndex + p_list->shift;
   
   const ListNode *nodes     = p_list->nodes;
   index_t         physIndex = nodes[0].next;
   
   for (index_t i = 0; i < logicIndex; i++)
      physIndex = nodes[physIndex].next;
   
   return physIndex;
}
#endif // LIST_LOGIC_INDEX
//
//void list_dump(const List *const p_list)
//{
//   printf(
//          #ifdef    LIST_LOGIC_INDEX
//          "Shift:    %lld\n"
//          #endif // LIST_LOGIC_INDEX
//
//          "Capacity: %lld\n"
//          "Size:     %lld\n"
//          "Free:     %lld\n",
//
//          #ifdef    LIST_LOGIC_INDEX
//          p_list->shift,
//          #endif // LIST_LOGIC_INDEX
//
//          p_list->capacity,
//          p_list->size,
//          p_list->free);
//
//   printf("Elements: ");
//   for (ssize_t i = 0; i < p_list->capacity; i++)
//   {
//      printf("%10" LIST_ITEM_SPECIFIER " | ", p_list->nodes[i].item);
//   }
//   printf("\n");
//
//   printf("Next:     ");
//   for (ssize_t i = 0; i < p_list->capacity; i++)
//   {
//      printf("%10lld | ", p_list->nodes[i].next);
//   }
//   printf("\n");
//
//   printf("Prev:     ");
//   for (ssize_t i = 0; i < p_list->capacity; i++)
//   {
//      printf("%10lld | ", p_list->nodes[i].prev);
//   }
//   printf("\n"
//          "-------------------------------------\n");
//}
// ================================ /export functions  ============================================