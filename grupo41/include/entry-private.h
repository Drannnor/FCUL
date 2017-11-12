/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#ifndef _ENTYT_PRIVATE_H
#define _ENTRY_PRIVATE_H

#include "entry.h"

/* Funcao que liberta a entry dada */
void entry_destroy(struct entry_t *entry);
struct entry_t *entry_create(char *key, struct data_t *value);
#endif
