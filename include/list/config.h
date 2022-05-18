//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#ifndef LIST_CONFIG_H
#define LIST_CONFIG_H

#include <limits.h>
#include "../intermediate.h"

typedef Intermediate ListItem;
typedef signed long long list_index_t;

static const list_index_t LIST_MAX_CAPACITY = LLONG_MAX >> 1;

#endif // LIST_CONFIG_H
