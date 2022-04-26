//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#ifndef LIST_CONFIG_H
#define LIST_CONFIG_H

#include <limits.h>
#include "../IR.h"

//#define LIST_ANTI_FOOL
#define LIST_EXTENSIBLE
//#define LIST_LOGIC_INDEX

typedef Intermediate ListItem;
typedef signed long long list_index_t;

const list_index_t LIST_MAX_CAPACITY = LLONG_MAX >> 1;
const char         LIST_DUMP_PATH[]  = "list_dump.html";

#endif // LIST_CONFIG_H
