//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "config.h"

const list_index_t LIST_FAULT = -1;
         
typedef struct ListNode
{
   ListItem     item;
   list_index_t prev;
   list_index_t next;
} ListNode;

typedef struct List
{
   #ifdef    LIST_LOGIC_INDEX
   list_index_t  shift;
   #endif // LIST_LOGIC_INDEX
   
    ListNode *restrict nodes;
    list_index_t free;
    list_index_t iterator;
    list_index_t size;
    list_index_t capacity;
} List;

list_index_t list_construct(List *const p_list, const list_index_t n_items);

void         list_destruct(List *const p_list);


list_index_t list_insertAfter(List *const p_list, const ListItem item, const list_index_t index);

list_index_t list_insertBefore(List *const p_list, const ListItem item, const list_index_t index);

list_index_t list_pushBack(List *const p_list, const ListItem item);

list_index_t list_pushFront(List *const p_list, const ListItem item);


list_index_t list_extract(List *const p_list, ListItem *const p_output, const list_index_t index);

list_index_t list_popBack(List *const p_list, ListItem *const p_output);

list_index_t list_popFront(List *const p_list, ListItem *const p_output);

void list_bind(List *const restrict p_list, const list_index_t index_first, const list_index_t index_second);


inline list_index_t list_iterate_forward (List *const restrict p_list);

inline list_index_t list_iterate_backward(List *const restrict p_list);

inline list_index_t list_reset_iterator(List *const restrict p_list);

#ifdef    LIST_LOGIC_INDEX
list_index_t list_sort_XXX_THE_FASTEST_SORT_IN_THE_WORLD(List *const p_list);

list_index_t list_find_physIndex(const List *const p_list, const list_index_t logicIndex);
#endif // LIST_LOGIC_INDEX
#endif // LIST_H
