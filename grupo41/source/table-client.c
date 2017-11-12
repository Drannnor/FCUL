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

#include "network_client-private.h"
#include "message-private.h"
#include "client_stub-private.h"
#include "client_stub.h"

#define MAX_SIZE 81

void print_message(struct message_t *msg);

int main(int argc, char **argv){
	//struct server_t *server;
	char *in;
	char *tok, *tok_opc, *opah; //!!!!
	//struct message_t *msg_out, *msg_resposta;
	//struct entry_t *entry;
	int count_param;
	//int completed;
	char **tokens;

	signal(SIGPIPE,SIG_IGN);

	/* Testar os argumentos de entrada */
	if(argc < 2){
		fprintf(stderr, "Insufficient arguments\n");
		return -1;
	}
	
	/* Usar network_connect para estabelcer ligação ao servidor */
	//server = network_connect(argv[1]); 
	struct rtables_t *rtables; //!!!
	rtables = rtables_bind(argv[1]);

	if((in =(char *)malloc(MAX_SIZE)) == NULL){
		fprintf(stderr, "Failed malloc in\n");
		return -1;
	}

	/* Fazer ciclo até que o utilizador resolva fazer "quit" */
	while (1){
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
		
		//completed = 1;

		/*if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
			fprintf(stderr, "Failed malloc\n");
			return -1;
		}*/

		if((tokens = (char**) malloc(sizeof(char*)*3)) == NULL){
			fprintf(stderr, "Failed malloc\n");
			return -1;
		}

		count_param = 0;
		while((tok = strtok(NULL, " ")) != NULL){
			tokens[count_param] = strdup(tok);
			count_param++;
		}
		
		if(count_param > 1){
			opah = tokens[count_param-1];
			tokens[count_param-1] = strdup(strtok(tokens[count_param-1], " \n"));
			free(opah);	
		}
		
		

		if(strcasecmp(tok_opc, "put") == 0){
			if(count_param < 3){
				printf("Input inválido: put <table_num> <key> <value>\n");
				continue;
			}
			else{
				rtables->t_num = atoi(tokens[0]);
				char* key_o = strdup(tokens[1]); //verificar null??? //!!!
				struct data_t *value_o = data_create2(strlen(tokens[2]),(void*)tokens[2]); //!!!
				rtables_put(rtables, key_o, value_o);
				//retornar valor????
			}
		}
		else if(strcasecmp(tok_opc, "get") == 0){
			if(count_param < 2){
				printf("Input inválido: get <table_num> <key>\n");
				continue;
			}
			else{
				rtables->t_num = atoi(tokens[0]);
				if((strcmp(tokens[1], "*") == 0)){
					rtables_get_keys(rtables);
				}
				else{
					char* key = strdup(tokens[1]); //!!!key
					rtables_get(rtables, key);
				}
				//retornar valor????
			}

		}
		else if(strcasecmp(tok_opc, "update") == 0){
			if(count_param < 3){
				printf("Input inválido: update <table_num> <key> <value>\n");
				continue;
			}
			else{
				rtables->t_num = atoi(tokens[0]);
				char* key_o = strdup(tokens[1]); //verificar null??? //!!!key_o
				struct data_t *value_o = data_create2(strlen(tokens[2]),(void*)tokens[2]); //!!!value_o
				rtables_update(rtables, key_o, value_o);
				//retornar valor????
			}
		}
		else if(strcasecmp(tok_opc, "size") == 0){
			if(count_param < 1){
				printf("Input inválido: size <table_num>\n");
				continue;
			}
			else{
				rtables->t_num = atoi(tokens[0]);
				rtables_size(rtables);
				//retornar valor????
			}
		}
		else if(strcasecmp(tok_opc, "collisions") == 0){
			if(count_param < 1){
				printf("Input inválido: collisions <table_num>\n");
				continue;
			}
			else{
				rtables->t_num = atoi(tokens[0]);
				rtables_collisions(rtables);
				//retornar valor????
			}
		}
		else if(strcasecmp(tok_opc, "quit") == 0){
			//free(msg_out);
			//return network_close(server);
			//free(tokens);
			//rtables_unbind(rtables);
			break;
		}
		else{
			printf("Input inválido: put, get, update, size, collisions, quit\n");
			//free(msg_out);
			//completed = 0;
		}

		/* Verificar se o comando foi "quit". Em caso afirmativo
		   não há mais nada a fazer a não ser terminar decentemente.
		 */
		/* Caso contrário:

			Verificar qual o comando;

			Preparar msg_out;

			Usar network_send_receive para enviar msg_out para
			o server e receber msg_resposta.
		*/
		
		/*
		if(completed){
			msg_resposta = network_send_receive(server, msg_out);
			print_message(msg_resposta);
			free_message(msg_out);
			free_message(msg_resposta); //???
		}*/
	}
  	//return network_close(server);
	free(in);
	free(tokens);
	return rtables_unbind(rtables); 
}

