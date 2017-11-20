#include "client_stub-private.h"
#include "client_stub.h"
#include "entry-private.h"

/* Fun��o para estabelecer uma associa��o entre o cliente e um conjunto de
 * tabelas remotas num servidor.
 * Os alunos dever�o implementar uma forma de descobrir quantas tabelas
 * existem no servidor. //???TODO + verificar out of bounds + reconnect
 * address_port � uma string no formato <hostname>:<port>.
 * retorna NULL em caso de erro .
 */
struct rtables_t *rtables_bind(const char *address_port){
    if(address_port == NULL){
        return NULL;
    }

    struct rtables_t *rtables;
    if((rtables = (struct rtables_t*) malloc(sizeof(struct rtables_t))) == NULL){
        fprintf(stderr, "Failed malloc!\n");
        return NULL;
    }

    rtables->server = network_connect(address_port);
    rtables->t_num = 0;

    return rtables;
}

/* Termina a associa��o entre o cliente e um conjunto de tabelas remotas, e
 * liberta toda a mem�ria local. 
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtables_unbind(struct rtables_t *rtables){
    if(rtables == NULL){
        return -1;
    }

    int n_co = network_close(rtables->server);
    free(rtables);
    return n_co;
}

/* Fun��o para adicionar um par chave valor numa tabela remota.
 * Devolve 0 (ok) ou -1 (problemas).
 */
int rtables_put(struct rtables_t *rtables, char *key, struct data_t *value){
    if(rtables == NULL || key == NULL || value == NULL){
        return -1;
    }

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

    struct message_t *msg_res;
    msg_res = network_send_receive(rtables->server,msg_out);
    print_message(msg_res);

    int res = msg_res->content.result;
    free(key);
    data_destroy(value);
    free_message(msg_res);
    free_message(msg_out);
    return res;
}

/* Fun��o para substituir na tabela remota, o valor associado � chave key.
 * Devolve 0 (OK) ou -1 em caso de erros.
 */
int rtables_update(struct rtables_t *rtables, char *key, struct data_t *value){
    if(rtables == NULL || key == NULL || value == NULL){
        return -1;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return -1;
    }

    msg_out->opcode = OC_UPDATE;
    msg_out->c_type = CT_ENTRY;
	msg_out->table_num = rtables->t_num;

    if((msg_out->content.entry = entry_create(key, value)) == NULL){
        fprintf(stderr, "Failed to create entry!\n");
        free_message(msg_out);
    }

    print_message(msg_out);
    struct message_t *msg_res;
    msg_res = network_send_receive(rtables->server,msg_out);
    print_message(msg_res);

    int res = msg_res->content.result;
    free(key);
    data_destroy(value);
    free_message(msg_res);
    free_message(msg_out);
    return res;
}

/* Fun��o para obter da tabela remota o valor associado � chave key.
 * Devolve NULL em caso de erro.
 */
struct data_t *rtables_get(struct rtables_t *rtables, char *key){
    if(rtables == NULL || key == NULL){
        return NULL;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return NULL;
    }

    msg_out->opcode = OC_GET;
    msg_out->c_type = CT_KEY;
	msg_out->table_num = rtables->t_num;
    if((msg_out->content.key = strdup(key)) == NULL){
        free_message(msg_out);
        return NULL;
    }

    print_message(msg_out);
    struct message_t *msg_res;
    if((msg_res = network_send_receive(rtables->server,msg_out)==NULL || msg_res->opcode == OC_RT_ERROR)){
        return NULL;
    }
    print_message(msg_res);

    struct data_t *data_res;
    if((data_res = data_dup(msg_res->content.data)) == NULL){
        free(data_res);
        free_message(msg_out);
        free_message(msg_res);
        return NULL;
    }

    free(key);
    free_message(msg_res);
    free_message(msg_out);
    return data_res;
}

/* Devolve n�mero de pares chave/valor na tabela remota.
 */
int rtables_size(struct rtables_t *rtables){
    if(rtables == NULL){
        return -1;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return -1;
    }

    msg_out->opcode = OC_SIZE;
    msg_out->c_type = 0;
	msg_out->table_num = rtables->t_num;

    print_message(msg_out);
    struct message_t *msg_res;
    msg_res = network_send_receive(rtables->server, msg_out);
    print_message(msg_res); 
    
    int res = msg_res->content.result;
    free_message(msg_res);
    free_message(msg_out);
    return res;
}

/* Devolve o número de colisões existentes na tabela remota.
 */
int rtables_collisions(struct rtables_t *rtables){
    if(rtables == NULL){
        return -1;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return -1;
    }

    msg_out->opcode = OC_COLLS;
    msg_out->c_type = 0;
	msg_out->table_num = rtables->t_num;

    print_message(msg_out);
    struct message_t *msg_res;
    msg_res = network_send_receive(rtables->server, msg_out);
    print_message(msg_res); 
    
    int res = msg_res->content.result;
    free_message(msg_res);
    free_message(msg_out);
    return res;
}

/* Devolve um array de char * com a c�pia de todas as keys da
 * tabela remota, e um �ltimo elemento a NULL.
 */
char **rtables_get_keys(struct rtables_t *rtables){
    if(rtables == NULL){
        return NULL;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return NULL;
    }

    char* asterisco = "*";

    msg_out->opcode = OC_GET;
    msg_out->c_type = CT_KEY;
	msg_out->table_num = rtables->t_num;
    msg_out->content.key = asterisco;

    print_message(msg_out);
    struct message_t *msg_res;
    msg_res = network_send_receive(rtables->server,msg_out);
    print_message(msg_res);

    int i = 0;
    while(msg_res->content.keys[i] != NULL){
        i++;
    }

    char** res;
    if((res = (char**) malloc(sizeof(char*) * (i+1))) == NULL){ //precisamos de 4 sockets!!!
        fprintf(stderr, "Failed to malloc!\n");
        return NULL;
    }

    i = 0;
    while(msg_res->content.keys[i] != NULL){
        res[i] = strdup(msg_res->content.keys[i]);
        i++;
    }
    res[i] = NULL;

    free_message(msg_res);
    free(msg_out);
    return res;
}

/* Liberta a mem�ria alocada por rtables_get_keys().
 */
void rtables_free_keys(char **keys){
    if(keys == NULL){
        return;
    }

    char **ptr = keys;
    while(*keys){
        free(*keys++);
    }
    free(ptr);
}