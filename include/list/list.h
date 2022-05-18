//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "config.h"

extern const list_index_t LIST_FAULT;

typedef struct ListNode
{
   ListItem     item;
   list_index_t prev;
   list_index_t next;
} ListNode;

typedef struct List
{
    ListNode *restrict nodes;
    list_index_t free;
    list_index_t iterator;
    list_index_t size;
    list_index_t capacity;
} List;


List* construct_list(const list_index_t n_items);

void destruct_list(List *const p_list);


list_index_t list_pushBack(List *const p_list, const ListItem *const restrict item);

void list_delete(List *const p_list, const list_index_t index);


extern inline list_index_t list_iterate_forward (List *const restrict p_list);

extern inline list_index_t list_reset_iterator(List *const restrict p_list);

#endif // LIST_H
