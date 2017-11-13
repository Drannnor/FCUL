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
			fprintf(stderr, "Numero de tabela dado inválido.\n");
			msg_resposta = message_error();
		}
		else{
			msg_resposta = process_message(msg_in, tables[tb_num]);
		}
	}
    return msg_resposta;
}




