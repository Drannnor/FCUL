/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H

#include "table.h"

struct table_t{
	struct entry_t *hash_table;
	int size_table;
	int num_entries;
	struct entry_t *next;
	int collisions;
};

void print_table(struct table_t *table);
struct entry_t *table_get_entries(struct table_t *table);

#endif