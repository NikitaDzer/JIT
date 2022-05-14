//
// Created by MIPT student Nikita Dzer on 12.11.2021.
//

#ifndef LIST_DUMP_H
#define LIST_DUMP_H

#include "list.h"

static const char LIST_DOTFILE_PATH[] = "dotfile.txt";

void list_dump_init(void);

void list_dump(const List *const p_list);

#endif // LIST_DUMP_H
