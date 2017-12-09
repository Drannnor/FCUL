/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "table-private.h"
#include "entry-private.h"

int hashcode(int size, char *key);
struct entry_t *get_entry(struct table_t *table, char *key);

struct table_t *table_create(int n){
    struct table_t *new_table = NULL;
    if(n > 0){
            new_table = (struct table_t*) malloc(sizeof(struct table_t));
            new_table->hash_table = 
            (struct entry_t*) malloc(sizeof(struct entry_t)*n);

            new_table->size_table = n;
            new_table->num_entries = 0;
            new_table->collisions = 0;  
            new_table->next = &new_table->hash_table[n-1];

            int i;
            for(i = 0; i < new_table->size_table; i++){
                entry_initialize(&new_table->hash_table[i]);
            }
    }

    return new_table;
}

void table_destroy(struct table_t *table){
    if(table != NULL){
        int i;
        for(i = 0; i < table->size_table; i++){
            struct entry_t entrada = table->hash_table[i];
            entry_destroy(&entrada);
        }
        free(table->hash_table);
        free(table);
    }
}

/*
 *função que encontra a proxima posição next
 */
void find_next(struct table_t *table){
    if(table -> num_entries == table -> size_table) return;
    while(table->next->key != NULL){
        table->next--;
    }
}

int table_put(struct table_t *table, char *key, struct data_t *value){
    
    if(table->num_entries < table->size_table && key != NULL){
        int hash = hashcode(table->size_table, key);
        struct entry_t *entrada = &table->hash_table[hash];
        int col = 0;
        while(entrada->key != NULL){
            col = 1;
            if(!strcmp(key,entrada->key)){

                return -1;
            }
            if(entrada->next == NULL){
                entrada->next = table -> next;

            }
            entrada = entrada->next;
        }
        entrada -> key = strdup(key);
        entrada -> value = data_dup(value);
        if(col) table->collisions++;
        table -> num_entries++;
        find_next(table);
        return 0;
    }
    return -1;
}

struct data_t *table_get(struct table_t *table, char *key){
    struct entry_t *entrada = get_entry(table, key);
    if(entrada == NULL) return NULL;
    return data_dup(entrada->value);
}

int table_update(struct table_t *table, char *key, struct data_t *value){

    struct entry_t *entrada = get_entry(table,key);
    if(entrada != NULL){
        data_destroy(entrada->value);
        entrada->value = data_dup(value);
        return 0;
    }
    return -1;
}

int table_size(struct table_t *table){
    return table->num_entries;
}

char **table_get_keys(struct table_t *table){
    char **list;
    if((list = (char **) malloc(sizeof(char *)*(table->num_entries+1))) == NULL){
        return NULL;
    }
    int i;
    int j = 0;
    for(i = 0; i < table->size_table; i++){
        if(table->hash_table[i].key != NULL){
            list[j++] = strdup(table->hash_table[i].key);   
        }
    }
    list[j] = NULL;
    return list;
}

void table_free_keys(char **keys){
    char **ptr= keys;
    if(keys != NULL){
        while(*keys){
            free(*keys++);
        }
        free(ptr);
    }
}

/*
 *funcão que devolde o hashcode de uma key
 */
int hashcode(int size, char *key){
    int i;
    int soma = 0;
    int length = strlen(key);
    if (length <= 4){
        for(i = 0; i < length; i++){
            soma += key[i];
        }
    }
    else
        soma = key[0]+key[1]+key[length-2]+key[length-1];

    return soma % size;
}

/*
 *função que devolve a entry com a key key, ou NULL caso esta nao exista
 */
struct entry_t *get_entry(struct table_t *table, char *key){
    int hash = hashcode(table->size_table, key);
    
    struct entry_t *entrada = &table->hash_table[hash];
    if(entrada->key == NULL)
        return NULL;

    if(strcmp(entrada->key, key)){
        while(entrada -> next != NULL){
            if(!strcmp(entrada->key, key)){
                return entrada;
            }
            entrada = entrada -> next;
        }
    }
    if(strcmp(entrada->key,key))
        return NULL;

    return entrada;
}

void print_table(struct table_t *table){
    int i;
    char* tkey; 
    struct entry_t* tkeyn;
    printf("-------TABELA-------\n");
    for(i=0; i < table->size_table; i++){
        printf("%d: ",i);
        if((tkey = table->hash_table[i].key) != NULL){
            printf("%s ", tkey);
            tkeyn = table->hash_table[i].next;
            while(tkeyn != NULL){
                printf("%s ", tkeyn->key);
                tkeyn = tkeyn->next;
            }
        }
        printf("\n");
    }
    printf("-------------------\n");
}

struct entry_t *table_get_entries(struct table_t *table){//TODO: get all entries from this table

}