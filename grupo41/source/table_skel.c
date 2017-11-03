/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/


#include "table_skel.h"

static struct table_t **tables;
static struct tablenum;
int table_skel_init(char **n_tables){


    if((tables = (struct table_t**)malloc(sizeof(struct table_t*)*(tablenum))) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		return -1;
    }
    

}

int table_skel_destroy(){
    for(i = 0; i < tablenum; i++){
		table_destroy(tables[i]);
	}

	free(tables);

}

struct message_t *invoke(struct message_t *msg_in){
    struct message_t *msg_resposta;

    if(msg_in->table_num >= tablenum){
		fprintf(stderr, "Numero de tabela dado inválido.\n");
		msg_resposta = message_error();
	}
	else{
		msg_resposta = process_message(msg_in, tables[msg_in->table_num]);
    }
    return msg_resposta;
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
		return message_error();
	}

	if(tabela == NULL){
		fprintf(stderr, "Tabela dada igual a NULL\n");
		return message_error();
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
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_size(tabela);
			break;
		case OC_UPDATE:
			key_p = msg_pedido->content.entry->key;
			value_p = msg_pedido->content.entry->value;
			result_r = table_update(tabela, key_p, value_p);
			if(result_r < 0){
				free(msg_resposta);
				return message_error();
			}
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = result_r;
			break;
		case OC_GET: 
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
			key_p = msg_pedido->content.entry->key;
			value_p = msg_pedido->content.entry->value;
			result_r = table_put(tabela, key_p, value_p);
			if(result_r < 0){
				free(msg_resposta);
				return message_error();
			}
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = result_r;
			break;
		case OC_COLLS: 
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = tabela->collisions;
			break;
		default:
			free(msg_resposta);
			return message_error();
	}

	/* Preparar mensagem de resposta */
	msg_resposta->opcode = opc_p + 1;
	msg_resposta->table_num = msg_pedido->table_num;

	return msg_resposta;
}
