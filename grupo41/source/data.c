/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

struct data_t *data_create(int size){
    struct data_t *a = NULL;

    if(size > 0){
        a = (struct data_t*) malloc(sizeof(struct data_t));
        if (a != NULL){
            a->datasize = size;
            a->data = malloc(size);
        }
    }

    return a;
}

struct data_t *data_create2(int size, void * data){
    struct data_t *b = NULL;
    if(data != NULL){
        b = data_create(size);
        if(b != NULL){
            memcpy(b->data,data,size);
        }
    }
    return b;
}

void data_destroy(struct data_t *data){
    if(data != NULL){
        free(data->data);
        free(data);
    }
}

struct data_t *data_dup(struct data_t *data){
    struct data_t *result= NULL;
    if(data != NULL){
        result = data_create2(data->datasize, data->data);
    }
    return result;
}

struct data_t *data_create_empty(){
    struct data_t *a = NULL;
    if((a = (struct data_t*) malloc(sizeof(struct data_t))) == NULL){
        fprintf(stderr, "data_create_empty - failed malloc\n");
    }
    if (a != NULL){
        a->datasize = 0;
        a->data = NULL;
    }
    return a;
}