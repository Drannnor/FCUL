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
#include "table-private.h"
#include "message-private.h"
#include "table_skel.h"

#define NFDESC 6

static int quit = 0;

/* Função para preparar uma socket de receção de pedidos de ligação.
*/
int make_server_socket(short port){
	int socket_fd;
	struct sockaddr_in server;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Erro ao criar socket.\n");
		return -1;
	}
	
	int sim = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (int *)&sim, sizeof(sim)) < 0 ) {
		fprintf(stderr,"SO_REUSEADDR setsockopt error");
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

/* Função "inversa" da função network_send_receive usada no table-client.
   Neste caso a função implementa um ciclo receive/send:

	Recebe um pedido;
	Aplica o pedido na tabela;
	Envia a resposta.
*/
int network_receive_send(int sockfd){
  	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result;
  	struct message_t *msg_pedido, *msg_resposta;

	/* Verificar parâmetros de entrada */

	if(sockfd < 0){
		fprintf(stderr, "Socket dada eh menor que zero\n");
		return -2;
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
		return -2;
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
		return -2;
	}
	
	print_message(msg_pedido);
	if((msg_resposta = invoke(msg_pedido)) == NULL){
		fprintf(stderr, "Failed invoke\n");
		free(buff_pedido);
		free(msg_pedido);
		return -2;
	}
	print_message(msg_resposta);
	

	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg_resposta, &buff_resposta)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -2;
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
		return -2;
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(sockfd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -2;
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
	struct sigaction a;
	int socket_de_escuta, i, nfds, res;

	char **n_tables;

	struct pollfd connections[NFDESC];

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

	/* inicialização */
	if(( socket_de_escuta = make_server_socket(atoi(argv[1]))) < 0){
		fprintf(stderr, "Error creating server socket");
		return -1;
	}

	/* inicializar o n_tables*/
	if((n_tables = (char**)malloc(sizeof(char*)*argc - 1)) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		return -1;
	}

	if((n_tables[0] = (char *)malloc(_INT)) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		free(n_tables);
		return -1;
	}

	sprintf(n_tables[0], "%d", argc-2);

	for(i = 1; i <= argc - 2; i++){
		n_tables[i] = (char *) malloc(strlen(argv[i + 1]) + 1);//verificar os mallocs
    	memcpy(n_tables[i],argv[i + 1],strlen(argv[i + 1]) + 1);
	 } 

	if((table_skel_init(n_tables) < 0)){
		fprintf(stderr, "Failed to init\n");
		return -1;
	}

	//initializacao da lista de conections
	for (i = 0; i < NFDESC; i++){
    	connections[i].fd = -1; // poll ignora estruturas com fd < 0
		connections[i].revents = 0;
		connections[i].events = 0;
	}

  	connections[0].fd = socket_de_escuta;  // Vamos detetar eventos na welcoming socket
  	connections[0].events = POLLIN;
    
    connections[1].fd = fileno(stdin);  // Vamos detetar eventos no standart in
  	connections[1].events = POLLIN;

	nfds = 1;

	while(!quit){ /* espera por dados nos sockets abertos */

		res = poll(connections,nfds,-1);
		if (res<0){
			if (errno == EINTR){
				continue;
			}else{
                quit = 1;
            }
		}

        i = 2;

		if ((connections[0].revents & POLLIN) && (nfds < NFDESC)) {// Pedido na listening socket ?
            	while(connections[i].fd != -1){
					i++;
            	}
        		if ((connections[i].fd = accept(connections[0].fd, NULL, NULL)) > 0){ // Ligação feita ?
          			connections[i].events = POLLIN; // Vamos esperar dados nesta socket
					nfds++;
				}
      	}
		/* um dos sockets de ligação tem dados para ler */
		for (i = 1; i < NFDESC; i++) {
			if (connections[i].revents & POLLIN) {
				if(i == 1){
					//tratar do input do standart in
				} else {
					res = network_receive_send(connections[i].fd);
				}
			}
			if (connections[i].revents & POLLERR || connections[i].revents & POLLHUP || res < 0) {
				if(res == -1){
					close(connections[i].fd);
					connections[i].fd = -1;
					connections[i].revents = 0;
					connections[i].events = 0;
					nfds--;
				} else {
					quit = 1;
					fprintf(stderr,"Closing server...");
				}
			}
		}
	}

	//FREE do n_tables
	/* fechar as ligações */
	for (i = 0; i < nfds; i++) {
		close(connections[i].fd);
	}
	
	return table_skel_destroy();
}
