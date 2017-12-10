/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/

#include "table_skel-private.h"

static struct table_t **tables;
static int tablenum;

int table_skel_init(char **n_tables){

	if(n_tables == NULL){
		fprintf(stderr, "NULL n_tables");
		return -1;
	}
	
	tablenum = atoi(n_tables[0]);

    if((tables = (struct table_t**)malloc(sizeof(struct table_t*)*(tablenum))) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		return -1;
    }

	int i;
	for(i = 0; i < tablenum; i++){
		if((tables[i] = table_create(atoi(n_tables[i+1]))) == NULL){
			fprintf(stderr, "Failed to create tables\n");
			free(tables);
			return -1;
		}
	}
	
	for(i = 0; i < tablenum + 1; i++){
		free(n_tables[i]);
	}
	free(n_tables);
    return 0;
}

int table_skel_destroy(){
	int i;
    for(i = 0; i < tablenum; i++){
		table_destroy(tables[i]);
	}
	free(tables);
	return 0;
}

struct message_t *invoke(struct message_t *msg_in){
    struct message_t *msg_resposta = NULL;

	int tb_num = msg_in->table_num;
	if(msg_in != NULL){	
		if(tb_num >= tablenum){
			fprintf(stderr, "Tabela nao existe.\n");
			msg_resposta = message_error(SERVER_ERROR);
		}
		else{
			msg_resposta = process_message(msg_in, tables[tb_num]);
		}
	}
    return msg_resposta;
}

void table_skel_print(int n){
	if(n < tablenum){
		print_table(tables[n]);
	} else {
		fprintf(stderr, "Tabela nao existe.\n");
	}
}

struct message_t* table_skel_get_tablenum(struct message_t *msg_in){
	struct message_t *msg_out;

	if(msg_in -> opcode != OC_TABLE_NUM){
		return message_error(CLIENT_ERROR);
	}
	
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
		fprintf(stderr, "table_skel_get_tablenum - Failed malloc\n");
		return NULL;
	}

	msg_out->opcode =  msg_in->opcode + 1;
    msg_out->c_type = CT_RESULT;
    msg_out->table_num = 0;
	msg_out->content.result = tablenum;

	return msg_out;
}

struct entry_t **table_skel_get_entries(int numero_da_tabela){

	if(numero_da_tabela > tablenum){
		fprintf(stderr,"Tabela não existe\n");
		return NULL;
	}
	return table_get_entries(tables[numero_da_tabela]);
}

int table_skel_size(int numero_da_tabela){
	//verificar se eh valido
	//ir buscar
	//devolver
	if(numero_da_tabela > tablenum){
		fprintf(stderr,"Tabela não existe\n");
		return -1;
	}

	return tables[numero_da_tabela]->size_table;
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