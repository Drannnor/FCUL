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

int table_skel_send_tablenum (int socketfd){
	short tb = htons(tablenum);
	int res; 
    if((res = (write_all(socketfd, (char *) &tb, _SHORT))) < 0){
		fprintf(stderr, "Write failed - size write_all\n");
		return -1;
	}
	return res;
}

struct entry_t *table_skel_get_entries(int numero_da_tabela){//TODO: get all entries from this table
	
	if(numero_da_tabela > tablenum){
		fprintf(stderr,"Tabela não existe\n");
		return NULL;
	}
	
	//verificar se o numero eh valido
	//fazer a lista
	//devolver 


}

int table_skel_size(int numero_da_tabela){
	//verificar se eh valido
	//ir buscar
	//devolver
}




