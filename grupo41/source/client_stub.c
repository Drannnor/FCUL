#include "client_stub-private.h"

//static struct server_t server;

/* Fun��o para estabelecer uma associa��o entre o cliente e um conjunto de
 * tabelas remotas num servidor.
 * Os alunos dever�o implementar uma forma de descobrir quantas tabelas
 * existem no servidor.
 * address_port � uma string no formato <hostname>:<port>.
 * retorna NULL em caso de erro .
 */
struct rtables_t *rtables_bind(const char *address_port){
    struct rtables_t rtables;
    if((rtables = (struct rtables_t) malloc(sizeof(struct rtables_t))) == NULL){
        fprintf(stderr, "Failed malloc!\n");
        return NULL;
    }

    rtables.server = network_connect(address_port);
    rtables.t_num = 0;

    return rtables;
}

/* Termina a associa��o entre o cliente e um conjunto de tabelas remotas, e
 * liberta toda a mem�ria local. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtables_unbind(struct rtables_t *rtables){
    int n_co = network_close(rtables->server);
    free(rtables);
    return n_co;
}

/* Fun��o para adicionar um par chave valor numa tabela remota.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int rtables_put(struct rtables_t *rtables, char *key, struct data_t *value){

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return -1;
    }

    msg_out->opcode = OC_PUT;
    msg_out->c_type = CT_ENTRY;
	msg_out->table_num = rtables->t_num;

    if((msg_out->content.entry = entry_create(key, value)) == NULL){
        fprintf(stderr, "Failed to create entry!\n");
        free_message(msg_out);
    }
    
    print_message(msg_out);
    msg_res = network_send_receive(rtables->server,msg_out);
    print_message(msg_res);
    return msg_res->content.result;
    //???

}

/* Fun��o para substituir na tabela remota, o valor associado � chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
int rtables_update(struct rtables_t *rtables, char *key, struct data_t *value){
    /*//copiado do table.c, com tables e nao rtables
    struct entry_t *entrada = get_entry(table,key);
    if(entrada != NULL){
        data_destroy(entrada->value);
        entrada->value = data_dup(value);
        return 0;
    }
    return -1;*/

    if((msg_out = malloc(sizeof(struct message_t))) == NULL){
        perror("Erro ao alocar memoria para mensagem");
        return -1;
    }

    msg_out->opcode = OC_UPDATE;
    msg_out->c_type = CT_ENTRY;
	msg_out->table_num = rtables->t_num;
    
    //porque nao usamos o table_update? Pelo que parece, nao percebi os enunciados passados.
    //??? e por ai fora
}

/* Fun��o para obter da tabela remota o valor associado � chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *rtables_get(struct rtables_t *rtables, char *key){
    /*//copiado do table.c, com tables e nao rtables
    struct entry_t *entrada = get_entry(table, key);
    if(entrada == NULL) return NULL;
    return data_dup(entrada->value);*/

    if((msg_out = malloc(sizeof(struct message_t))) == NULL){
        perror("Erro ao alocar memoria para mensagem");
        return -1;
    }

    msg_out->opcode = OC_GET;
    msg_out->c_type = CT_KEY;
	msg_out->table_num = rtables->t_num;
    msg_out->content = strdup(key);
    //???
}

/* Devolve n�mero de pares chave/valor na tabela remota.
 */
int rtables_size(struct rtables_t *rtables){
    //copiado do table.c, com tables e nao rtables
    //return table->num_entries;
    if((msg_out = malloc(sizeof(struct message_t))) == NULL){
        perror("Erro ao alocar memoria para mensagem");
        return -1;
    }

    msg_out->opcode = OC_SIZE;
    msg_out->c_type = 0;
	msg_out->table_num = rtables->t_num;

    //???
}

/* Devolve o número de colisões existentes na tabela remota.
 */
int rtables_collisions(struct rtables_t *rtables){
    if((msg_out = malloc(sizeof(struct message_t))) == NULL){
        perror("Erro ao alocar memoria para mensagem");
        return -1;
    }

    msg_out->opcode = OC_COLLS;
    msg_out->c_type = 0;
	msg_out->table_num = rtables->t_num;

    //???
}

/* Devolve um array de char * com a c�pia de todas as keys da
 * tabela remota, e um �ltimo elemento a NULL.
 */
char **rtables_get_keys(struct rtables_t *rtables){
    /*//copiado do table.c, com tables e nao rtables
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
    */
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