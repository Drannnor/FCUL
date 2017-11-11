/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/


#include "table_skel.h"

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
	for(i = 1; i < tablenum; i++){
		tables[i-2] = table_create(atoi(n_tables[i]));
	}
    
}

int table_skel_destroy(){
	int i;
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

