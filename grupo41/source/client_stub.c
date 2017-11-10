#include "client_stub-private.h"

/* Fun��o para estabelecer uma associa��o entre o cliente e um conjunto de
 * tabelas remotas num servidor.
 * Os alunos dever�o implementar uma forma de descobrir quantas tabelas
 * existem no servidor.
 * address_port � uma string no formato <hostname>:<port>.
 * retorna NULL em caso de erro .
 */
struct rtables_t *rtables_bind(const char *address_port){

}

/* Termina a associa��o entre o cliente e um conjunto de tabelas remotas, e
 * liberta toda a mem�ria local. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtables_unbind(struct rtables_t *rtables){

}

/* Fun��o para adicionar um par chave valor numa tabela remota.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int rtables_put(struct rtables_t *rtables, char *key, struct data_t *value){
    /*//copiado do table.c, com tables e nao rtables
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
    return -1;*/

    if((msg_out = malloc(sizeof(struct message_t))) == NULL){
        perror("Erro ao alocar memoria");
        return -1;
    }

    msg_out->opcode = OC_PUT;
    msg_out->c_type = CT_ENTRY;
	msg_out->table_num = rtables->t_num;

    msg_res = network_send_receive(rtables->socket,msg_out);
    

}

/* Fun��o para substituir na tabela remota, o valor associado � chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
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

/* Fun��o para obter da tabela remota o valor associado � chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *rtables_get(struct rtables_t *tables, char *key){
    //copiado do table.c, com tables e nao rtables
    struct entry_t *entrada = get_entry(table, key);
    if(entrada == NULL) return NULL;
    return data_dup(entrada->value);
}

/* Devolve n�mero de pares chave/valor na tabela remota.
 */
int rtables_size(struct rtables_t *rtables){
    //copiado do table.c, com tables e nao rtables
    return table->num_entries;
}

/* Devolve um array de char * com a c�pia de todas as keys da
 * tabela remota, e um �ltimo elemento a NULL.
 */
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

/* Liberta a mem�ria alocada por rtables_get_keys().
 */
void rtables_free_keys(char **keys){
    char **ptr= keys;
    while(*keys){
        free(*keys++);
    }
    free(ptr);
}