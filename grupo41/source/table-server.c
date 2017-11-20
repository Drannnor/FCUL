/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
/*
   Programa que implementa um servidor de uma tabela hash com chainning.
   Uso: table-server <port> <table1_size> [<table2_size> ...]
   Exemplo de uso: ./table_server 54321 10 15 20 25
*/
#include <error.h>

#include "inet.h"
#include "table-private.h"
#include "message-private.h"

static int quit = 0;
static int tablenums;

/* Função para preparar uma socket de receção de pedidos de ligação.
*/
int make_server_socket(short port){
	int socket_fd;
	struct sockaddr_in server;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Erro ao criar socket.\n");
		return -1;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(port);  
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0){
		fprintf(stderr, "Erro ao fazer bind.\n");
		close(socket_fd);
		return -1;
	}

	if (listen(socket_fd, 0) < 0){
		fprintf(stderr, "Erro ao executar listen.\n");
		close(socket_fd);
		return -1;
	}
	return socket_fd;
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
		return message_error(-1);
	}

	if(tabela == NULL){
		fprintf(stderr, "Tabela dada igual a NULL\n");
		return message_error(-1);
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
				return message_error(-1);
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
				return message_error(-1);
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
			return message_error(-1);
	}

	/* Preparar mensagem de resposta */
	msg_resposta->opcode = opc_p + 1;
	msg_resposta->table_num = msg_pedido->table_num;

	return msg_resposta;
}

/* Função "inversa" da função network_send_receive usada no table-client.
   Neste caso a função implementa um ciclo receive/send:

	Recebe um pedido;
	Aplica o pedido na tabela;
	Envia a resposta.
*/
int network_receive_send(int sockfd, struct table_t **tables){
  	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result;
  	struct message_t *msg_pedido, *msg_resposta;

	/* Verificar parâmetros de entrada */
	if(*tables == NULL){
		fprintf(stderr, "Lista de tabelas inválida\n");
		return -1;
	}

	if(sockfd < 0){
		fprintf(stderr, "Socket dada eh menor que zero\n");
		return -1;
	}

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/

	/* Verificar se a receção teve sucesso */
	if((result = read_all(sockfd, (char *) &msg_size, _INT)) <= 0){
		if(result < 0) fprintf(stderr, "Read failed - size read_all\n");
		return -1;
	}

	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	if((buff_pedido = (char *) malloc(htonl(msg_size))) == NULL){
		fprintf(stderr, "Failed malloc buff_pedido \n");
		return -1;
	}

	/* Com a função read_all, receber a mensagem de resposta. */

	/* Verificar se a receção teve sucesso */
	if((result = read_all(sockfd, buff_pedido, ntohl(msg_size))) < 0){
		if(result < 0) fprintf(stderr, "Read failed - message read_all\n");
		free(buff_pedido);
		return -1;
	}

	/* Desserializar a mensagem do pedido */
	/* Verificar se a desserialização teve sucesso */
	if ((msg_pedido = buffer_to_message(buff_pedido, msg_size)) == NULL){
		fprintf(stderr, "Failed unmarshalling\n");
		free(buff_pedido);
		free(msg_pedido);
		return -1;
	}
	
	/* Processar a mensagem */
	if(msg_pedido->table_num >= tablenums){
		fprintf(stderr, "Numero de tabela dado inválido.\n");
		msg_resposta = message_error(-1);
	}
	else{
		msg_resposta = process_message(msg_pedido, tables[msg_pedido->table_num]);
	}

	/* Serializar a mensagem a enviar */

	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg_resposta, &buff_resposta)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -1;
	}

	/* Enviar ao cliente o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);

	/* Verificar se o envio teve sucesso */
	if(write_all(sockfd, (char *) &msg_size, _INT) < 0){
		fprintf(stderr, "Write failed - size write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -1;
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(sockfd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -1;
	}

	/* Libertar memória */
	free(buff_resposta);
	free(buff_pedido);
	free_message(msg_resposta);
	free_message(msg_pedido);

	return 0;
}

void sign_handler(int signum){
	quit = 1;
	printf("\nClosing server...\n");
	return;
}

int main(int argc, char **argv){
	int listening_socket, connsock, result;
	struct table_t **tables;
	struct sigaction a;

	a.sa_handler = sign_handler;
	a.sa_flags = 0;
	sigemptyset( &a.sa_mask );
	sigaction( SIGINT, &a, NULL );
	signal(SIGPIPE,SIG_IGN);

	if (argc < 3){
		printf("Uso: ./server <porta TCP> <table1_size> [<table2_size> ...]\n");
		printf("Exemplo de uso: ./table-server 54321 10 15 20 25\n");
		return -1;
	}

	tablenums = (argc-2);

	if ((listening_socket = make_server_socket(atoi(argv[1]))) < 0) return -1;
	
	/*********************************************************/
	/* Criar as tabelas de acordo com linha de comandos dada */
	/*********************************************************/
	if((tables = (struct table_t**)malloc(sizeof(struct table_t*)*(tablenums))) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		return -1;
	}

	int i;
	for(i = 2; i < argc; i++){
		tables[i-2] = table_create(atoi(argv[i]));
	}

	while (!quit) {
		if ((connsock = accept(listening_socket, NULL, NULL)) == -1){
			break;
		}
			
		printf(" * Client is connected!\n");

		while (!quit && (result = network_receive_send(connsock, tables)) >= 0){
			
			/* Fazer ciclo de pedido e resposta */
			
			/* Ciclo feito com sucesso ? Houve erro?
			   Cliente desligou? */

		}
		printf(" * Client is disconnected!\n");
	}



	//table_skel_destroy();

	for(i = 0; i < tablenums; i++){
		table_destroy(tables[i]);
	}

	free(tables);

	return 0;
}