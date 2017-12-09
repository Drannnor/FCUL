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
#include <pthread.h>
#include "table_skel-private.h"
#include "message-private.h"
#include "primary_backup-private.h"

#define NFDESC 7
#define MAX_SIZE 81
#define MAX_ADDRESS_SIZE 20
#define	N_TABLES_MSIZE 180
#define PRIMARY_FILE "primary-server.conf"
#define BACKUP_FILE "backup-server.conf" //TODO: perguntar ao neelo como podemos ter a 
										//certeza que eh escrito no sitio certo

static int quit = 0;
static char* nome_ficheiro;
int primary, secondary_up, first_time;

static struct server_t *other_server;

//funcao que verifica se a mensagem se trata de uma put ou de um updatez
//se for esse o caso devolve 1
//caso contrario devolve 0
int is_write(struct message_t *msg){ 
	return msg->opcode == OC_PUT || msg->opcode == OC_UPDATE;
}


int read_file(char *file_name,char **adrport,char ***n_tables){
	int i, j;
	char in[MAX_SIZE];
	int table_num;
	FILE *fp;

	if((fp = fopen(file_name,"r")) == NULL){
		return 0;
	}

	if((*adrport = (char*)malloc (MAX_ADDRESS_SIZE)) == NULL){
		fprintf(stderr,"Failed malloc\n");
		return -1;
	}

	fgets(*adrport, MAX_ADDRESS_SIZE,fp);

	fgets(in,N_TABLES_MSIZE,fp);

	table_num = atoi(in);

	if((*n_tables = (char**)malloc(sizeof(char*)*(table_num + 2))) == NULL){
		fprintf(stderr, "Failed malloc tables\n");
		return -1;
	}
			

	if((*n_tables[0] = (char*)malloc(strlen(in)))== NULL){
		fprintf(stderr,"Failed malloc\n");
		return -1;
	}

	sprintf(*n_tables[0], "%d", table_num);
	for(i = 1;i < table_num;i++){
		fgets(in,N_TABLES_MSIZE,fp);
		if((*n_tables[i] = (char*)malloc(strlen(in))) == NULL){// estava n em vez do i logo estava mal
			for(j = 0; j < i; j++){
				free(*n_tables[j]);
			}
			free(*n_tables);
			fprintf(stderr, "Failed malloc tables3\n");
			return -1;
		}
		memcpy(*n_tables[i],in,strlen(in) + 1);
	}

	n_tables[table_num + 1] = NULL;

	return 0;
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
int network_receive_send(int socket_fd){
  	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result;
  	struct message_t *msg_pedido, *msg_resposta;
	pthread_t *thread;
	/* Verificar parâmetros de entrada */

	if(socket_fd < 0){
		fprintf(stderr, "Socket dada eh menor que zero\n");
		return -2;
	}

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/

	/* Verificar se a receção teve sucesso */
	if((result = read_all(socket_fd, (char *) &msg_size, _INT)) <= 0){
		if(result < 0) fprintf(stderr, "Read failed - size read_all\n");
		fprintf(stderr, "ERRO\n");//FIXME:
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
	if((result = read_all(socket_fd, buff_pedido, ntohl(msg_size))) < 0){
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

	// se estivermos no servidor secundario assegurar exclusao mutua TODO:

	if((msg_resposta = invoke(msg_pedido)) == NULL){
		fprintf(stderr, "Failed invoke\n");
		free(buff_pedido);
		free(msg_pedido);
		return -2;
	}
	print_message(msg_resposta);

	//TODO: copiar msg

	if(primary && is_write(msg_pedido) && secondary_up){
		if((thread = backup_update(msg_pedido, other_server)) == NULL){
			secondary_up = 0;
		}
	}


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
	if(write_all(socket_fd, (char *) &msg_size, _INT) < 0){
		fprintf(stderr, "Write failed - size write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -2;
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(socket_fd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return -2;
	}

	// if(!update_successful(thread)){
	// 	secondary_up = 0;
	// }
	//verificar que a thread fez o seu trabalho TODO:
	//com sucesso, caso contrario, marca o servidor secundario com DOWN

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
	if(strcasecmp(tok, "quit") == 0){
		quit = 1;
		remove(nome_ficheiro); //FIXME: verificar se isto se pode fazer -Burno <3 acho que nao viste o que fizeste
		
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

int write_file(char *filename,char *adrport,char **n_tables){//FIXME: CRUZZ!! nao compila
<<<<<<< HEAD
	FILE *fp;
	int i;
	int n = sizeof(*n_tables)/sizeof(*n_tables[0]);
=======
	// char* in;
	// FILE *fp;
	// int i,n;
>>>>>>> f5caf108012739e05864cc4c50ca395aba61de29

	// fp = fopen(filename,"w");

<<<<<<< HEAD
	fputs(adrport,fp);
	fputs(("%d\n",n),fp);

	for(i = 0;i < n;i++){
		fputs(*n_tables[i],fp);
	}
	return 1;
=======
	// fgets(in, MAX_ADDRESS_SIZE,fp);

	// fputs(("%s\n",in),fp);
	// fgets(in,N_TABLES_MSIZE,fp);
	// fputs(("%s\n",in),fp);
	// n = atoi(in)+2;

	// for(i = 1;i < n;i++){
	// 	fgets(in,N_TABLES_MSIZE,fp);
	// 	fputs(("%s\n",in),fp);
	// }
	// return 1;
	return -1;
>>>>>>> f5caf108012739e05864cc4c50ca395aba61de29
}

int main(int argc, char **argv){
	struct sigaction a;
	int socket_de_escuta, i, j, nfds, res;
	char *address_port;//FIXME: talvez seja necessario diferenciar os ficheiros dos 2 servidores,caso sejam criados na mesma maquina
	char **n_tables;
	//FIXME: arrumar esta merda

	struct pollfd connections[NFDESC];
	struct sockaddr *p_server = (struct sockaddr*)malloc(sizeof(struct sockaddr*));
	a.sa_handler = sign_handler;
	a.sa_flags = 0;
	sigemptyset(&a.sa_mask);
	sigaction(SIGINT, &a, NULL);
	signal(SIGPIPE,SIG_IGN);
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
	if(primary){
		nome_ficheiro = PRIMARY_FILE;
	} else {
		nome_ficheiro = BACKUP_FILE;
	}

	

	if(( socket_de_escuta = make_server_socket((unsigned short)atoi(argv[1]))) < 0){
		fprintf(stderr, "Error creating server socket");
		return -1;
	}

	if((res = read_file(nome_ficheiro,&address_port,&n_tables))){//se existe o ficheiro este servidor esta a recuperar de um crash
		if ( res == -1 ){
			fprintf(stderr, "Unable to read file");
			return -1;
		}
		first_time = 0;

	} else {//primeira execucao
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
					return -1;
				}
				memcpy(n_tables[i],argv[i + 2],strlen(argv[i + 2]) + 1);
			}
			n_tables[table_num + 1] = NULL;

			address_port = strdup(argv[2]);//FIXME: nao esquecer de fazer free
			if((other_server = server_bind(address_port))){
				secondary_up = 1;
				if((send_table_info(other_server,n_tables)) < 0){
					secondary_up = 0;
				}
			}

		} else {//Servidor Secundario, primeira vez
			printf("sou o secundario\n");
			if (( other_server = (struct server_t*)malloc(sizeof(struct server_t))) == NULL){
				fprintf(stderr, "Failed malloc other_server\n");
				secondary_up = 0;
			} else {
				printf("awating connection...\n");
				if((other_server->socket_fd = accept(socket_de_escuta,p_server,&primary_size)) > 0){//FIXME: esta certo? -Bruno
					if((address_port = get_address_port(other_server, p_server)) != NULL){
						printf("getting tables ...\n");
						if((n_tables = get_table_info(other_server->socket_fd)) != NULL){
							secondary_up = 1;
						}
					}
				}
			}
		}
	}
	
	if((table_skel_init(n_tables) < 0)){
		fprintf(stderr, "Failed to init\n");
		return -1;
	}

	if(first_time){
		// if((write_file(nome_ficheiro, address_port, n_tables)) < 0){
		// 	fprintf(stderr, "Failed to write configuration file");
		// 	return -1;
		// }
	} else {//sync
		other_server = server_bind(address_port);
		printf("Couldn't sync!"); //FIXME: para tirar
		//hello(other_server);TODO: fazer o hello e o update
	}

	/* inicialização */

	//initializacao da lista de conections
	for (i = 0; i < NFDESC; i++){
    	connections[i].fd = -1; // poll ignora estruturas com fd < 0
		connections[i].revents = 0;
		connections[i].events = 0;
	}

  	connections[0].fd = socket_de_escuta;  // Vamos detetar eventos na welcoming socket
  	connections[0].events = POLLIN;
    
    connections[1].fd = fileno(stdin);  // Vamos detetar eventos no standard in
  	connections[1].events = POLLIN;
	
	connections[2].fd = other_server -> socket_fd;//FIXME: vai abaixo se o secundario nao ligar
	connections[2].events = POLLIN;

	nfds = 3;
	
	while(!quit){ /* espera por dados nos sockets abertos */

		res = poll(connections,nfds,-1);
		if (res<0){
			if (errno == EINTR){
				continue;
			}else{
                quit = 1;
            }
		}

        i = 3;

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
