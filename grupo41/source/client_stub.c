#include "client_stub-private.h"


struct rtables_t *rtables_bind(const char *address_port){

}

int rtables_unbind(struct rtables_t *rtables){

}

int rtables_put(struct rtables_t *rtables, char *key, struct data_t *value){
    //copiado do table.c, com tables e nao rtables
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

int rtables_update(struct rtables_t *rtables, char *key, struct data_t *value){
    //copiado do table.c, com tables e nao rtables
    struct entry_t *entrada = get_entry(table,key);
    if(entrada != NULL){
        data_destroy(entrada->value);
        entrada->value = data_dup(value);
        return 0;
    }
    return -1;

}

struct data_t *rtables_get(struct rtables_t *tables, char *key){
    //copiado do table.c, com tables e nao rtables
    struct entry_t *entrada = get_entry(table, key);
    if(entrada == NULL) return NULL;
    return data_dup(entrada->value);

}

int rtables_size(struct rtables_t *rtables){
    //copiado do table.c, com tables e nao rtables
    return table->num_entries;
}

char **rtables_get_keys(struct rtables_t *rtables){
    //copiado do table.c, com tables e nao rtables
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

void rtables_free_keys(char **keys){
    char **ptr= keys;
    while(*keys){
        free(*keys++);
    }
    free(ptr);
}