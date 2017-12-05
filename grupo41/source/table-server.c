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
#include <stdio.h>
#include "table-private.h"
#include "message-private.h"
#include "table_skel-private.h"

#define NFDESC 6
#define MAX_SIZE 1000
#define ADDRPORT_SIZE 15
#define	N_TABLES_MSIZE 180

static int quit = 0; 
int primary, secondary_up, first_time;


struct thread_params{
	struct rtables_t *rtables;
	char **n_tables;
};

int read_file(char *file_name,char **adrport,char ***n_tables){
	int i,n;
	char *in, *n_tables_read[n];

	if((fopen(file_name,"r")) == NULL){
		return 0;//FIXME:
	}
	fgets(in,ADDRPORT_SIZE,f);

	if((adrport = (char**)malloc(ADDRPORT_SIZE)) == NULL){
		fprintf(stderr,"Failed malloc\n");
		return -1;//FIXME:
	}

	adrport = in;
	fgets(in,N_TABLES_MSIZE,f);
	n = atoi(in)+2;
	if((n_tables_read[0] = (char*)malloc(strlen(in))) == NULL){
			fprintf(stderr,"Failed malloc\n");
			return -1;//FIXME:
	}
	for(i = 1;i < n;i++){
		fgets(in,N_TABLES_MSIZE,f);
		if((n_tables_read[n] = (char*)malloc(strlen(in))) == NULL){
			fprintf(stderr,"Failed malloc\n");
			return -1;//FIXME:
		}
	}
	return 1;
}
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

	if(bind(socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0){
		fprintf(stderr, "Erro ao fazer bind.\n");
		close(socket_fd);
		return -1;
	}

	if(listen(socket_fd, 0) < 0){
		fprintf(stderr, "Erro ao executar listen.\n");
		close(socket_fd);
		return -1;
	}

	return socket_fd;
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
	// se estivermos no servidor secundario asegurar exclusao mutua TODO:
	if((msg_resposta = invoke(msg_pedido)) == NULL){
		fprintf(stderr, "Failed invoke\n");
		free(buff_pedido);
		free(msg_pedido);
		return -2;
	}
	print_message(msg_resposta);

	/*se estivermos no servidor primarioTODO:
		inicializar a estrutura com os paramentros necessarios, e
		criar a thread que vai enviar a msg ao client
	*/

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

int tratar_input(){
	char in[MAX_SIZE];
	char *tok;

	fgets(in,MAX_SIZE,stdin);
	in[strlen(in) - 1] = '\0';
	if((tok = strtok(in," ")) == NULL){
		fprintf(stderr,"Input inválido: quit, print <número da tabela>\n");
		return -1;
	}
	if((tok = strdup(tok)) == NULL){
		fprintf(stderr,"Erro ao alocar memória para o primeiro token");
		quit = 1;
		return -1;
	}
	if(strcasecmp( tok, "quit") == 0){
		quit = 1;
		
	} else if (strcasecmp( tok, "print") == 0){
		free(tok);
		if((tok = strtok(NULL," ")) == NULL){
			fprintf(stderr,"Input inválido: print <número da tabela>\n");
			return -1;
		}
		if((tok = strdup(tok)) == NULL){
			fprintf(stderr,"Erro ao alocar memória para o token");
			quit = 1;
			return -1;
		}
		table_skel_print(atoi(tok));
	} else {
		fprintf(stderr,"Input inválido: quit, print <número da tabela>\n");
	}
	free(tok);
	return 0;
}

//escreve o ficheiro com a configuracao do servidor TODO:
int write_file(char *file_name,char **adrport,char ***n_tables){

}

//rtable_bind mas para server TODO:
struct idk server_bind(const char *addrport){

}

//espera que o servidor primario envie os tamanhos das tabelas TODO:
char **get_table_sizes(int socket_fd){

}

//devolve uma string da forma <ip>:<port> pronto para escrever TODO:
char *get_addres_port(sockaddr *p_server){

}



int main(int argc, char **argv){
	struct sigaction a;
	int socket_de_escuta, i, j, nfds, res;
	char *in, *token, *nome_ficheiro = "server.conf";//FIXME: talvez seja necessario diferenciar os ficheiros dos 2 servidores, 
	char *port_ip[2];								 //caso sejam criados na mesma maquina
	char **n_tables;
	char **n_tables, **addrport;

	struct pollfd connections[NFDESC];
	struct thread_params *params;
	struct sockaddr *p_server;

	a.sa_handler = sign_handler;
	a.sa_flags = 0;
	sigemptyset(&a.sa_mask);
	sigaction(SIGINT, &a, NULL);
	signal(SIGPIPE,SIG_IGN);
	pthread_t sec_connect;
	socklen_t primary_size = sizeof(p_server);
	
	if (argc >= 3){//servidor primario

		primary = 1;
		secondary_up = 0;
	} else if (argc == 2){//servidor secundario
		primary = 0;
	} else {//input invalido
		printf("Uso para servidor primario: ./server <porta TCP> <IP do secundario:porta TCP do secundario> <table1_size> [<table2_size> ...]\n");
		printf("Exemplo de uso: ./server 54321 127.0.0.1:54322 10 15 20 25\n");
		printf("-----------------------------------------------------\n");
		printf("Uso para servidor secundario: ./server <porta TCP>\n");
		printf("Exemplo de uso: ./server 54322\n");
		return -1;
	}

	int res;
	if(((res = read_file(nome_ficheiro,addrport,n_tables))){//se existe o ficheiro 
		if ( res == -1 ){
			fprintf(stderr, "Unable to read file");
			return -1;
		}
		first_time = 0;

	} else {
		first_time = 1;
		if(primary){//Servidor Primario, primeira vez
			int table_num = argc - 3;
			if((n_tables = (char**)malloc(sizeof(char*)*(table_num + 2))) == NULL){
				fprintf(stderr, "Failed malloc tables1\n");
				return -1;
			}
			
			if((n_tables[0] = (char *)malloc(_INT)) == NULL){
				fprintf(stderr, "Failed malloc tables2\n");
				free(n_tables);
				return -1;
			}

			sprintf(n_tables[0], "%d", table_num);

			for(i = 1; i <= table_num; i++){
				if((n_tables[i] = (char *) malloc(strlen(argv[i + 1]) + 1)) == NULL){
					for(j = 0; j < i; j++){
						free(n_tables[j]);
					}
					free(n_tables);
					fprintf(stderr, "Failed malloc tables3\n");
					return -1
				}
				memcpy(n_tables[i],argv[i + 2],strlen(argv[i + 2]) + 1);
			}
			n_tables[table_num + 1] = NULL;
			addrport = argv[2];//FIXME: verificar se ]e valido

		} else {//Servidor Secundario, primeira vez 
			int server_fd = accept(argv[1],&p_server,&primary_size);//FIXME: verificar se o argv1 é um num
			n_tables = get_table_size(server_fd);//TODO: verificar o resultado da funcao de sec_connect1
			addrport = get_addres_port(p_server);
		}
	}

	
	if((table_skel_init(n_tables) < 0)){
		fprintf(stderr, "Failed to init\n");
		return -1
	}

	if(first_time){
		if((write_file(nome_ficheiro, addrport, n_tables)) < 0){
			fprinf(stderr, "Failed to write server.conf");
			return -1;
		}
	} else {//sync
		stables = server_bind(addrport);
		hello(stables.server);
	}

	/* inicialização */
	if(( socket_de_escuta = make_server_socket((unsigned short)atoi(argv[1]))) < 0){
		fprintf(stderr, "Error creating server socket");
		return -1;
	}

	//initializacao da lista de conections
	for (i = 0; i < NFDESC; i++){
    	connections[i].fd = -1; // poll ignora estruturas com fd < 0
		connections[i].revents = 0;
		connections[i].events = 0;
	}
	//TODO: talvez adicionar o socket do outro servidor	
  	connections[0].fd = socket_de_escuta;  // Vamos detetar eventos na welcoming socket
  	connections[0].events = POLLIN;
    
    connections[1].fd = fileno(stdin);  // Vamos detetar eventos no standard in
  	connections[1].events = POLLIN;

	nfds = 2;
	
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
        		if ((connections[i].fd = accept(connections[0].fd, NULL, NULL)) > 0){ // Ligação feita ? TODO: guardar a o sockaddr do cliente, 
																					  // se for a secundario verificar se o client se trata do servidor primario
																					  // caso negativo passa a ser o servidor primario
          			connections[i].events = POLLIN;
					if ((res = table_skel_send_tablenum(connections[i].fd)) <= 0){
						if (res == 0){
							close(connections[i].fd);
							connections[i].fd = connections[nfds-1].fd;
							connections[i].revents = connections[nfds-1].revents;
							connections[i].events = connections[nfds-1].events;
							connections[nfds-1].fd = -1;
							connections[nfds-1].revents = 0;
							connections[nfds-1].events = 0;
						} else {
							quit = 1;
						}

					} // Vamos esperar dados nesta socket
					nfds++;
				}
      	}
		/* um dos sockets de ligação tem dados para ler */
		for (i = 1; i < nfds; i++) {
			if (connections[i].revents & POLLIN) {
				if(i == 1){
					tratar_input();
				} else {
					res = network_receive_send(connections[i].fd);
					//se for o primario, criar uma thread que envia para o secundario TODO:
					if (connections[i].revents & POLLERR || connections[i].revents & POLLHUP || res < 0) {
						if(res == -1){		
							close(connections[i].fd);
							connections[i].fd = connections[nfds-1].fd;
							connections[i].revents = connections[nfds-1].revents;
							connections[i].events = connections[nfds-1].events;
							connections[nfds-1].fd = -1;
							connections[nfds-1].revents = 0;
							connections[nfds-1].events = 0;
							nfds--;
						} else {
							quit = 1;
							break;
						}
					}
				}
			}
		}
	}
	fprintf(stderr,"Closing server...");

	for (i = 0; i < nfds; i++) {
		close(connections[i].fd);
	}
	
	return table_skel_destroy();
}
