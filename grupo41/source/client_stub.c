#include "client_stub-private.h"


struct rtables_t *rtables_bind(const char *address_port){

}

int rtables_unbind(struct rtables_t *rtables){

}

int rtables_put(struct rtables_t *rtables, char *key, struct data_t *value){

}

int rtables_update(struct rtables_t *rtables, char *key, struct data_t *value){

}

struct data_t *rtables_get(struct rtables_t *tables, char *key){

}

int rtables_size(struct rtables_t *rtables){

}

char **rtables_get_keys(struct rtables_t *rtables){

}

void rtables_free_keys(char **keys){
    
}