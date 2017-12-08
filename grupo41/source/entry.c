/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "entry-private.h"

void entry_initialize(struct entry_t *entry){
    if(entry != NULL){
        entry->key = NULL;
        entry->value = NULL;
        entry->next = NULL;
    }
}

struct entry_t *entry_dup(struct entry_t *entry){
    struct entry_t *new_entry = NULL;
    if(entry != NULL){
        new_entry = (struct entry_t*) malloc(sizeof(struct entry_t));
        if(new_entry != NULL){
            new_entry -> key = strdup(entry->key);
            new_entry -> value = data_dup(entry -> value);
            new_entry -> next = entry-> next;
        }
    }
    return new_entry;
}

void entry_destroy(struct entry_t *entry){
    data_destroy(entry->value);
    free(entry->key);
}

struct entry_t *entry_create(char *key, struct data_t *value){
    struct entry_t *ent = NULL;

    if(key != NULL && value != NULL){
        if((ent = (struct entry_t*)malloc(sizeof(struct entry_t))) == NULL){
            fprintf(stderr, "Failed malloc!\n");
            return NULL;
        }
        ent->key = key;
        ent->value = value;
        ent->next = NULL;
    }
    
    return ent;
}