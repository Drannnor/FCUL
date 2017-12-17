/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
/*
	Programa cliente para manipular tabela de hash remota.
	Os comandos introduzido no programa não deverão exceder
	80 carateres.

	Uso: table-client <ip servidor>:<porta servidor>
	Exemplo de uso: ./table_client 10.101.148.144:54321
*/

#include "message-private.h"
#include "client_stub-private.h"
#include "client_stub.h"
#include <ctype.h>

#define MAX_SIZE 81

void print_message(struct message_t *msg);

int isNumber(char *token){

	while(*token){
		if(!isdigit(*token)){
			return 0;
		}
		token++;
	}
	return 1;
}

void ol_switcheroo(char **primary, char **secondary){
	char* bff = *primary;
	*primary = *secondary;
	*secondary = bff;
}

int main(int argc, char **argv){
	char in[MAX_SIZE];
	char *tok, *tok_opc, *key_o;
	int count_param,i,n_tentativas,result,toquit = 0;
	struct data_t *value_o;
	char *tokens[3];

	signal(SIGPIPE,SIG_IGN);

	/* Testar os argumentos de entrada */
	if(argc < 3){
		fprintf(stderr, "Insufficient arguments\n");
		fprintf(stderr, "Uso: ./client <ip_servidor>:<porta_servidor> <ip_backup>:<porta_backup>\n");
		fprintf(stderr, "Exemplo de uso: ./client 10.101.148.144:54321 10.120.150.150:12345\n");
		return -1;
	}
	
	/* Usar network_connect para estabelcer ligação ao servidor */
	struct rtables_t *rtables = NULL;
	char* primary = strdup(argv[1]);
	char* secondary = strdup(argv[2]);

	/* Fazer ciclo até que o utilizador resolva fazer "quit" */
	while (1 && !toquit){
		printf(">>> "); // Mostrar a prompt para inserção de comando
		

		/* Receber o comando introduzido pelo utilizador
		   Sugestão: usar fgets de stdio.h
		   Quando pressionamos enter para finalizar a entrada no
		   comando fgets, o carater \n é incluido antes do \0.
		   Convém retirar o \n substituindo-o por \0.
		*/
		fgets(in,MAX_SIZE,stdin);
		in[strlen(in) - 1] = '\0';
		tok_opc = strdup(strtok(in," "));

		if(strcasecmp(tok_opc, "quit") == 0){
			free(tok_opc);
			toquit = 1;
			break;
		}

		count_param = 0;
		while((tok = strtok(NULL, " ")) != NULL){
			tokens[count_param] = strdup(tok);
			count_param++;
		}

		n_tentativas = 0;
		while(rtables == NULL && n_tentativas < 4){
			if(n_tentativas == 2){
				sleep(RETRY_TIME);
			}
			if((rtables = rtables_bind(primary)) == NULL){
				n_tentativas++;
				ol_switcheroo(&primary,&secondary);
			}
		}

		if(rtables == NULL){
			fprintf(stderr, "Unable to connect to server!\n");
			continue;
		}
		
		rtables -> server -> address_port_pri = primary;
		rtables -> server -> address_port_sec = secondary;//FIXME: nao eh aqui

		if(strcasecmp(tok_opc, "put") == 0){
			if(count_param < 3){
				printf("Input inválido: put <table_num> <key> <value>\n");
			} else {
				if(!isNumber(tokens[0])){
					printf("Input inválido: número de tabela tem de ser um inteiro\n");
					continue;
				}
				rtables->table_index = atoi(tokens[0]);
				if(rtables->table_index >= rtables->numberOfTables){
					fprintf(stderr, "Tabela nao existe!\n");
					continue;
				}
				if((key_o = strdup(tokens[1])) == NULL){
					fprintf(stderr, "put - strdup failed\n");
					continue;
				}
				if((value_o = data_create2(strlen(tokens[2]),(void*)tokens[2])) == NULL){
					fprintf(stderr, "put - data_create2 failed\n");
					continue;
				}
				if((result = rtables_put(rtables, key_o, value_o)) < 0){
					if(result == CONNECTION_ERROR){
						rtables_unbind(rtables);
						rtables = NULL;
						fprintf(stderr, "put - lost connection to server\n");
						continue;
					} else {
						fprintf(stderr, "put - rtables_put failed\n");
						continue;
					}
				}
			}
		} else if(strcasecmp(tok_opc, "get") == 0){
			if(count_param < 2){
				printf("Input inválido: get <table_num> <key>\n");
			} else {
				if(!isNumber(tokens[0])){
					printf("Input inválido: número de tabela tem de ser um inteiro\n");
					continue;
				}
				rtables->table_index = atoi(tokens[0]);
				if(rtables->table_index >= rtables->numberOfTables){
					fprintf(stderr, "Tabela nao existe!\n");
					continue;
				}
				if((strcmp(tokens[1], "*") == 0)){
					rtables_free_keys(rtables_get_keys(rtables));//FIXME: arranjar return 
				} else {
					if((key_o = strdup(tokens[1])) == NULL){
						fprintf(stderr, "get - strdup failed\n");
						continue;
					}
					data_destroy(rtables_get(rtables, key_o));//FIXME: arranjar return
				}
			}

		} else if(strcasecmp(tok_opc, "update") == 0){
			if(count_param < 3){
				printf("Input inválido: update <table_num> <key> <value>\n");
			} else {
				if(!isNumber(tokens[0])){
					printf("Input inválido: número de tabela tem de ser um inteiro\n");
					continue;
				}
				rtables->table_index = atoi(tokens[0]);
				if(rtables->table_index >= rtables->numberOfTables){
					fprintf(stderr, "Tabela nao existe!\n");
					continue;
				}
				if((key_o = strdup(tokens[1])) == NULL){
					fprintf(stderr, "strdup failed\n");
					continue;
				}
				if((value_o = data_create2(strlen(tokens[2]),(void*)tokens[2]))==NULL){
					fprintf(stderr, "update - data_create2 failed\n");
					continue;
				}
				if((result = rtables_update(rtables, key_o, value_o)) < 0){
					if(result == CONNECTION_ERROR){
						rtables_unbind(rtables);
						fprintf(stderr, "update - lost connection to server\n");
						continue;
					} else {
						fprintf(stderr, "update - rtables_update failed\n");
						continue;
					}
				}
			}
		} else if(strcasecmp(tok_opc, "size") == 0){
			if(count_param < 1){
				printf("Input inválido: size <table_num>\n");
			} else {
				if(!isNumber(tokens[0])){
					printf("Input inválido: número de tabela tem de ser um inteiro\n");
					continue;
				}
				rtables->table_index = atoi(tokens[0]);
				if(rtables->table_index >= rtables->numberOfTables){
					fprintf(stderr, "Tabela nao existe!\n");
					continue;
				}
				if((result = rtables_size(rtables)) < 0){
					if(result == CONNECTION_ERROR){
						rtables_unbind(rtables);
						fprintf(stderr, "size - lost connection to server\n");
						continue;
					} else {
						fprintf(stderr, "size - rtables_size failed\n");
						continue;
					}
				}
				
			}
		} else if(strcasecmp(tok_opc, "collisions") == 0){
			if(count_param < 1){
				printf("Input inválido: collisions <table_num>\n");
			} else {
				if(!isNumber(tokens[0])){
					printf("Input inválido: número de tabela tem de ser um inteiro\n");
				}
				rtables->table_index = atoi(tokens[0]);
				if(rtables->table_index >= rtables->numberOfTables){
					fprintf(stderr, "Tabela nao existe!\n");
					continue;
				}
				if((result = rtables_collisions(rtables)) < 0){
					if(result == CONNECTION_ERROR){
						rtables_unbind(rtables);
						fprintf(stderr, "collisions - lost connection to server\n");
						continue;
					} else {
						fprintf(stderr, "collisions - rtables_collisions failed\n");
						continue;
					}
				}
			}
		} else {
			printf("Input inválido: put, get, update, size, collisions, quit\n");
		}

		i = 0;
		while(i < count_param){
			free(tokens[i]);
			i++;
		}
		//free(tokens); //???
		free(tok_opc);
	}
	//free(primary);
	//free(secondary);
	return rtables_unbind(rtables); 
}

