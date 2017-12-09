/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "message-private.h"
#include "table-private.h"
#include "entry-private.h"

#define _OPCODE 2
#define _C_TYPE 2
#define _TABLE_NUM 2
#define _OCT 6

//tamanhos de content

#define _MIN_ENTRY 14
#define _MIN_KEY 9
#define _MIN_KEYS 13
#define _MIN_VALUE 11
#define _MIN_RESULT 10

#define _MIN_SIZE 9

int n_keys(char **keys);
int sum_length_keys(char **keys);
void value_marshalling(struct data_t *value_data, char **ptr);
void value_unmarshalling(struct message_t *m, char **ptr);
void key_marshalling(char *key, char **ptr);

void free_message(struct message_t *msg)
{
    //Verificar se msg é NULL

    if (msg != NULL){
        // Se msg->c_type for:
        switch (msg->c_type){
        // VALOR, libertar msg->content.data
            case CT_VALUE:
                data_destroy(msg->content.data);
                break;
            // CHAVE, libertar msg->content.key
            case CT_KEY:
                free(msg->content.key);
                break;
            // CHAVES, libertar msg->content.keys
            case CT_KEYS:
                table_free_keys(msg->content.keys);
                break;
            // ENTRY, libertar msg->content.entry
            case CT_ENTRY:
                entry_destroy(msg->content.entry);
                free(msg->content.entry);
                break;
        }
        // libertar msg
        free(msg);
    }
}

int message_to_buffer(struct message_t *msg, char **msg_buf){
    char *ptr;
    int buffer_size = _OCT;
    uint16_t short_value;
    uint32_t long_value;

    //Verificar se msg é NULL
    if (msg == NULL){
        return -1;
    }

    switch (msg->c_type){
        case CT_ENTRY:
            buffer_size += _SHORT + (strlen(msg->content.entry->key)) +
                        _INT + (msg->content.entry->value->datasize);
            break;
        case CT_KEY:
            buffer_size += _SHORT + (strlen(msg->content.key));
            break;
        case CT_KEYS:

            buffer_size += _INT + (sum_length_keys(msg->content.keys));
            break;
        case CT_VALUE:
            buffer_size += _INT + (msg->content.data->datasize);
            break;
        case CT_RESULT:
            buffer_size += _INT;
            break;
    }

    if ((*msg_buf = (char *)malloc(buffer_size)) == NULL){
        fprintf(stderr, "Failed malloc msg_buf\n");
        return -1;
    }

    ptr = *msg_buf;

    short_value = htons(msg->opcode);
    memcpy(ptr, &short_value, _SHORT);
    ptr += _SHORT;

    short_value = htons(msg->c_type);
    memcpy(ptr, &short_value, _SHORT);
    ptr += _SHORT;

    // Serializar número da tabela
    short_value = htons(msg->table_num);
    memcpy(ptr, &short_value, _SHORT);
    ptr += _SHORT;

    int i;
    int total;
    switch (msg->c_type)
    {
    case CT_ENTRY:

        key_marshalling(msg->content.entry->key, &ptr);
        value_marshalling(msg->content.entry->value, &ptr);
        break;

    case CT_KEY:

        key_marshalling(msg->content.key, &ptr);
        break;

    case CT_KEYS:

        total = n_keys(msg->content.keys);

        long_value = htonl(total);
        memcpy(ptr, &long_value, _INT);
        ptr += _INT;

        for (i = 0; i < total; i++){
            key_marshalling(msg->content.keys[i], &ptr);
        }
        break;

    case CT_VALUE:

        value_marshalling(msg->content.data, &ptr);
        break;

    case CT_RESULT:

        long_value = htonl(msg->content.result);
        memcpy(ptr, &long_value, _INT);
        break;

        // Consoante o conteúdo da mensagem, continuar a serialização da mesma
    }
    return buffer_size;
}

struct message_t *buffer_to_message(char *msg_buf, int msg_size)
{
    struct message_t *msg;
    // Verificar se msg_buf é NULL
    // msg_size tem tamanho mínimo ?

    if (msg_buf == NULL || msg_size < _MIN_SIZE){
        printf("Buffer to msg - invalido input");
        return NULL;
    }

    // Alocar memória para uma struct message_t

    if ((msg = (struct message_t *)malloc(sizeof(struct message_t))) == NULL){
        return NULL;
    }

    // Recuperar o opcode e c_type
    // O opcode e c_type são válidos?

    uint16_t short_aux;

    memcpy(&short_aux, msg_buf, _SHORT);
    msg->opcode = ntohs(short_aux);
    msg_buf += _SHORT;


    if(msg->opcode < 10 || msg->opcode > 100){
        free(msg);
        printf("%d\n", msg->opcode);
        return NULL;
    }

    memcpy(&short_aux, msg_buf, _SHORT);
    msg->c_type = ntohs(short_aux);
    msg_buf += _SHORT;

    // Recuperar table_num

    memcpy(&short_aux, msg_buf, _SHORT);
    msg->table_num = ntohs(short_aux);
    msg_buf += _SHORT;

    struct data_t *d;
    int str_len, nkeys, i, j, ds;
    short ks;
    char* key;

    switch (msg->c_type){
    case CT_ENTRY:
        ks = ntohs(*(short *) msg_buf);
        msg_buf += _SHORT;
        if ((key = malloc((ks * sizeof(char)) + 1)) == NULL){
            free(msg);
            return NULL;
        }

        memcpy(key, msg_buf, ks);
        key[ks] = '\0';
        msg_buf += ks;
        ds = ntohl(*(int *) msg_buf);
        msg_buf += _INT;
        if ((d = data_create(ds)) == NULL){
            free(key);
            free(msg);
            return NULL;
        }
        memcpy(d->data, msg_buf, ds);
        msg->content.entry = (struct entry_t *)malloc(sizeof(struct entry_t));
        if (msg->content.entry == NULL){
            data_destroy(d);
            free(key);
            free(msg);
            return NULL;
        }
        msg->content.entry->key = strdup(key);
        if (msg->content.entry->key == NULL){
            data_destroy(d);
            free(key);
            free(msg->content.entry);
            free(msg);
            return NULL;
        }
        msg->content.entry->value = data_dup(d);
        if (msg->content.entry->value == NULL){
            data_destroy(d);
            free(key);
            free(msg->content.entry->key);
            free(msg->content.entry);
            free(msg);
            return NULL;
        }
        data_destroy(d);
        free(key);

        break;
    case CT_KEY:

        str_len = (int) ntohs(*(short *) msg_buf);
        msg_buf += _SHORT;
        if ((msg->content.key = (char *) malloc((str_len * sizeof(char)) + 1)) == NULL){
            free(msg);
            return NULL;
        }
        memcpy(msg->content.key, msg_buf, str_len);
        msg->content.key[str_len] = '\0';

        break;
    case CT_KEYS:
        nkeys = ntohl(*(int *) msg_buf);
        msg_buf += _INT;
        if ((msg->content.keys = (char **) malloc((nkeys + 1) * sizeof(char *))) == NULL){
            free(msg);
            return NULL;
        }
        for (i = 0; i < nkeys; i++){
            ks = ntohs(*(short *) msg_buf);
            msg_buf += _SHORT;
            if ((msg->content.keys[i] = malloc((ks * sizeof(char)) + 1)) == NULL){
                for (j = 0; j < i; j++)
                    free(msg->content.keys[j]);
                free(msg);
                return NULL;
            }
            memcpy(msg->content.keys[i], msg_buf, ks);
            msg->content.keys[i][ks] = '\0';
            msg_buf += ks;
        }
        msg->content.keys[i] = NULL;

        break;
    case CT_VALUE:
        value_unmarshalling(msg, &msg_buf);

        break;
    case CT_RESULT:
        msg->content.result = ntohl(*(int *) msg_buf);
        break;
    }

    // Exemplo da mesma coisa que em cima mas de forma compacta, ao estilo C!

    //msg->opcode = ntohs(*(short *) msg_buf++);
    //msg->c_type = ntohs(*(short *) ++msg_buf);
    //msg_buf += _SHORT;

    // Consoante o c_type, continuar a recuperação da mensagem original
    return msg;
}

int n_keys(char **keys)
{
    int result = 0;
    while (*keys++)
    {
        result++;
    }
    return result;
}

int sum_length_keys(char **keys)
{
    int result = 0;
    while (*keys)
    {
        result += strlen(*keys++) + _SHORT;
    }
    return result;
}

void value_marshalling(struct data_t *value_data, char **ptr)
{

    uint32_t long_value;

    int data_size = value_data->datasize;

    long_value = htonl(value_data->datasize);
    memcpy(*ptr, &long_value, _INT);
    *ptr += _INT;

    memcpy(*ptr, value_data->data, data_size);
    *ptr += data_size;
}

void value_unmarshalling(struct message_t *m, char **ptr)
{

    int datasize_conv;

    //memcpy(&data_size,*ptr,_INT);
    datasize_conv = ntohl(*(int*) *ptr);
    *ptr += _INT;
    if ((m->content.data = data_create(datasize_conv)) == NULL)
    {
        free(m);
        return;
    }

    memcpy(m->content.data->data, *ptr, datasize_conv);
    *ptr += datasize_conv;

}

void key_marshalling(char *key, char **ptr)
{

    uint16_t short_value;

    int str_len = strlen(key);

    short_value = htons(str_len);
    memcpy(*ptr, &short_value, _SHORT);
    *ptr += _SHORT;

    memcpy(*ptr, key, str_len);
    *ptr += str_len;
}

int write_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len>0) {
		int res = write(sock, buf, len);
		if(res<0) {
			if(errno==EINTR) continue;
			fprintf(stderr, "Write failed\n");
			return res;
		}
		buf += res;
		len -= res;
	}
	return bufsize;
}

int read_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len>0) {
		int res = read(sock, buf, len);
		if(res <= 0) {
			if(errno==EINTR) continue;
			if(res < 0) fprintf(stderr, "Read failed\n");
			return res;
		}
		buf += res;
		len -= res;
	}
	return bufsize;
}

struct message_t* message_error(int errcode){
    struct message_t *msg;
    if((msg = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
		fprintf(stderr, "Failed malloc message_pedido\n");
		return NULL;
	}
	msg->opcode = OC_RT_ERROR;
    msg->c_type = CT_RESULT;
    msg->table_num = 0;
	msg->content.result = errcode;
	return msg;
}

// struct message_t *copy_message(struct message_t *msg){//TODO: funcao que copia uma messagem
//     struct message_t *new_msg;

//     if ((new_msg = (struct message_t *)malloc(sizeof(struct message_t))) == NULL){
//         return NULL;
//     }

// }

void print_message(struct message_t *msg) {
    int i;
    
    printf("\n----- MESSAGE -----\n");
    printf("Tabela número: %d\n", msg->table_num);
    printf("opcode: %d, c_type: %d\n", msg->opcode, msg->c_type);
    switch(msg->c_type) {
        case CT_ENTRY:{
            printf("key: %s\n", msg->content.entry->key);
            printf("datasize: %d\n", msg->content.entry->value->datasize);
        }break;
        case CT_KEY:{
            printf("key: %s\n", msg->content.key);
        }break;
        case CT_KEYS:{
            for(i = 0; msg->content.keys[i] != NULL; i++) {
                printf("key[%d]: %s\n", i, msg->content.keys[i]);
            }
        }break;
        case CT_VALUE:{
            printf("datasize: %d\n", msg->content.data->datasize);
        }break;
        case CT_RESULT:{
			if(msg->opcode == OC_RT_ERROR){
				printf("Ocorreu um erro! Tente novamente!\n");
				break;
			}
			printf("result: %d\n", msg->content.result);
        }break;
    }
    printf("-------------------\n");
}


/* Função que recebe uma tabela e uma mensagem de pedido e:
	- aplica a operação na mensagem de pedido na tabela;
	- devolve uma mensagem de resposta com oresultado.
*/
struct message_t *process_message(struct message_t *msg_pedido, struct table_t *tabela){
	struct message_t *msg_resposta;
	
	/* Verificar parâmetros de entrada - verificar se os parametros sao null*/
	if(msg_pedido == NULL){
		fprintf(stderr, "msg_pedido dada igual a NULL.\n");
		return message_error(SERVER_ERROR);
	}

	if(tabela == NULL){
		fprintf(stderr, "Tabela dada igual a NULL\n");
		return message_error(SERVER_ERROR);
	}

	if((msg_resposta = (struct message_t*) malloc(sizeof(struct message_t)))==NULL){
		fprintf(stderr, "Failed malloc\n");
		return NULL;
	}
	
	/* Verificar opcode e c_type na mensagem de pedido */
	short opc_p = msg_pedido->opcode;

	/* Aplicar operação na tabela */
	char *key_p;
	struct data_t *value_p;
	int result_r;

	switch(opc_p){
		case OC_SIZE:
            if(msg_pedido->c_type != CT_RESULT){
                fprintf(stderr, "size - c_type errado!\n");
                return message_error(SERVER_ERROR);
            }
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_size(tabela);
			break;
		case OC_UPDATE:
            if(msg_pedido->c_type != CT_ENTRY){
                fprintf(stderr, "update - c_type errado!\n");
                return message_error(SERVER_ERROR);
            }
			key_p = msg_pedido->content.entry->key;
			value_p = msg_pedido->content.entry->value;
			result_r = table_update(tabela, key_p, value_p);
			if(result_r < 0){
				free(msg_resposta);
				return message_error(SERVER_ERROR);
			}
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = result_r;
			break;
		case OC_GET:
            if(msg_pedido->c_type != CT_KEY){
                fprintf(stderr, "get - c_type errado!\n");
                return message_error(SERVER_ERROR);
            }
			if(strcmp(msg_pedido->content.key,"*") == 0){
				msg_resposta->content.keys = table_get_keys(tabela);
				msg_resposta->c_type = CT_KEYS;
			}
			else{
				key_p = msg_pedido->content.key;
				if((msg_resposta->content.data = table_get(tabela, key_p)) == NULL){
					msg_resposta->content.data = data_create_empty();
				}
				msg_resposta->c_type = CT_VALUE;
			}
			break;
		case OC_PUT:
            if(msg_pedido->c_type != CT_ENTRY){
                fprintf(stderr, "put - c_type errado!\n");
                return message_error(SERVER_ERROR);
            }
			key_p = msg_pedido->content.entry->key;
			value_p = msg_pedido->content.entry->value;
			result_r = table_put(tabela, key_p, value_p);
			if(result_r < 0){
				free(msg_resposta);
				return message_error(SERVER_ERROR);
			}
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = result_r;
			break;
		case OC_COLLS:
            if(msg_pedido->c_type != CT_RESULT){
                fprintf(stderr, "collisions - c_type errado!\n");
                return message_error(SERVER_ERROR);
            }
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = tabela->collisions;
			break;
		default:
			free(msg_resposta);
			return message_error(SERVER_ERROR);
	}

	/* Preparar mensagem de resposta */
	msg_resposta->opcode = opc_p + 1;
	msg_resposta->table_num = msg_pedido->table_num;

	return msg_resposta;
}
