/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#include "network_client-private.h"

#include <stdlib.h>

struct server_t *network_connect(const char *address_port){

	int sockfd;
	struct sockaddr_in *server;
	char *delim = ":";
	struct server_t *servidor;

	/* Verificar parâmetro da função e alocação de memória */
	if (address_port == NULL){
		fprintf(stderr, "Null address_port\n");
		return NULL;
	}

	if (( servidor = (struct server_t*)malloc(sizeof(struct server_t))) == NULL){
		fprintf(stderr, "Failed malloc\n");
		return NULL;
	}

	if (( server = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in))) == NULL){
		fprintf(stderr, "Failed malloc\n");
		free(servidor);
		return NULL;
	}

	/* Estabelecer ligação ao servidor:
	
	Preencher estrutura struct sockaddr_in com dados do
	endereço do servidor. */
	server->sin_family = AF_INET;
	
	char *adr_p = strdup(address_port);
	if (inet_pton(AF_INET, strtok(adr_p,delim), &server->sin_addr) < 1) { // Endereço IP
		printf("Erro ao converter IP\n");
		free(servidor);
		free(server);
		free(adr_p);
		return NULL;
	}
	server->sin_port = htons(atoi(strtok(NULL,delim)));

	//Criar a socket.
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Unable to create socket\n");
		free(servidor);
		free(server);
		free(adr_p);
		return NULL;
	}
	
	//Estabelecer ligação.
	if (connect(sockfd,(struct sockaddr *)server, sizeof(*server)) < 0) {
		fprintf(stderr, "Unable to connect to server\n");
		close(sockfd);
		free(servidor);
		free(server);
		free(adr_p);
		return NULL;
	}

	/* Se a ligação não foi estabelecida, retornar NULL */
	free(server); 
	free(adr_p);	
	servidor->socket_fd = sockfd;
	return servidor;
}

struct message_t *network_send_receive(struct server_t *server, struct message_t *msg){
	char *message_out;
	int message_size, msg_size;
	struct message_t *msg_resposta;
	

	/* Verificar parâmetros de entrada */
	if (server == NULL){
		fprintf(stderr, "Server dado eh invalido\n");
		return message_error();
	}

	if(msg == NULL){
		fprintf(stderr, "Mensagem dada eh invalida\n");
		return message_error();
	}

	/* Serializar a mensagem recebida */

	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg, &message_out)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(message_out);
		return message_error();
	}

	/* Enviar ao servidor o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);

	int result;
	int first_try = 1;
	/* Verificar se o envio teve sucesso */
	while(first_try >= 0){
		if((result = write_all(server->socket_fd, (char *) &msg_size, _INT)) <= 0){
			if(result == 0 || first_try > 0){
				sleep(RETRY_TIME);
				first_try--;
			}
			else{
				fprintf(stderr, "Write failed - size write_all\n");
				free(message_out);
				return message_error();
			}	
		}
	}
	

	/* Enviar a mensagem que foi previamente serializada */
	first_try = 1;
	/* Verificar se o envio teve sucesso */
	while(first_try >= 0){
		if((result = write_all(server->socket_fd, message_out, message_size)) <= 0){
			if(result == 0 || first_try > 0){
				sleep(RETRY_TIME);
				first_try--;
			}
			else{
				fprintf(stderr, "Write failed - message write_all\n");
				free(message_out);
				return message_error();
			}	
		}
	}

	/* De seguida vamos receber a resposta do servidor:

		Com a função read_all, receber num inteiro o tamanho da 
		mensagem de resposta. */
	first_try = 1;
	while(first_try >= 0){
		if((result = read_all(server->socket_fd, (char *) &msg_size, _INT)) <= 0){
			if(result == 0  == 0|| first_try > 0){
				sleep(RETRY_TIME);
				first_try--;
			}
			else{
				fprintf(stderr, "Read failed - size read_all\n");
				free(message_out);
				return message_error();
			}	
		}
	}

	/* Alocar memória para receber o número de bytes da
	mensagem de resposta. */
		
	/* Com a função read_all, receber a mensagem de resposta. */
	char *message_in;
	if((message_in = (char *)malloc(ntohl(msg_size))) == NULL){
		printf("Failed malloc - message_in\n");
		free(message_out);
		return message_error();
	}
	
	/* Verificar se a receção teve sucesso */
	first_try = 1;
	while(first_try >= 0){
		if((result = read_all(server->socket_fd, message_in, ntohl(msg_size)) <= 0){
			if(result == 0 || first_try > 0){
				sleep(RETRY_TIME);
				first_try--;
			}
			else{
				fprintf(stderr, "Read failed - message read_all\n");
				free(message_out);
				free(message_in);
				return message_error();
			}	
		}
	}

	/* Desserializar a mensagem de resposta */
	msg_resposta = buffer_to_message(message_in, ntohl(msg_size));

	/* Verificar se a desserialização teve sucesso */
	if (msg_resposta == NULL){
		fprintf(stderr, "Failed unmarshalling\n");
		free(message_out);
		free(message_in);
		return message_error();
	}

	/* Libertar memória */
	free(message_in);
	free(message_out);
	return msg_resposta;
}

int network_close(struct server_t *server){
	/* Verificar parâmetros de entrada */
	if(server == NULL){
		return -1;
	}
	
	/* Terminar ligação ao servidor */
	close(server->socket_fd);

	/* Libertar memória */
	free(server);
	return 0;
}

