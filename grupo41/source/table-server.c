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
#include "primary_backup-private.h"

#define NFDESC 6
#define MAX_SIZE 1000
#define adress_port_SIZE 15
#define	N_TABLES_MSIZE 180

static int quit = 0; 
int primary, secondary_up, first_time;
struct server_t *other_server;


struct thread_params{
	struct server_t *server;
	struct message_t *msg;
};

int read_file(char *file_name,char **adrport,char ***n_tables){//FIXME: pelo Cruz Cruuuuuzz!!!!!!! eh favor corrigir!!! ctrl + shift + p --> toggle error squiggles, e pensar um bocadinho
	int i,n;
	char *in, *n_tables_read[n];

	return NULL; //FIXME: para apagar

	if((fopen(file_name,"r")) == NULL){
		return 0;//FIXME:
	}
	//fgets(in,adress_port_SIZE,f);

	if((adrport = (char**)malloc(adress_port_SIZE)) == NULL){
		fprintf(stderr,"Failed malloc\n");
		return -1;//FIXME:
	}

	adrport = in;
	//fgets(in,N_TABLES_MSIZE,f);
	n = atoi(in)+2;
	if((n_tables_read[0] = (char*)malloc(strlen(in))) == NULL){
			fprintf(stderr,"Failed malloc\n");
			return -1;//FIXME:
	}
	for(i = 1;i < n;i++){
		//fgets(in,N_TABLES_MSIZE,f);
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
int network_receive_send(int socket_fd){
  	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result;
  	struct message_t *msg_pedido, *msg_resposta;
	struct thread_params *t_params;
	pthread_t thread;
	/* Verificar parâmetros de entrada */

	if(socket_fd < 0){
		fprintf(stderr, "Socket dada eh menor que zero\n");
		return NULL;
	}

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/

	/* Verificar se a receção teve sucesso */
	if((result = read_all(socket_fd, (char *) &msg_size, _INT)) <= 0){
		if(result < 0) fprintf(stderr, "Read failed - size read_all\n");
		return -1;
	}

	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	if((buff_pedido = (char *) malloc(htonl(msg_size))) == NULL){
		fprintf(stderr, "Failedmalloc buff_pedido \n");
		return NULL;
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
		return NULL;
	}
	
	print_message(msg_pedido);

	// se estivermos no servidor secundario asegurar exclusao mutua TODO:

	if((msg_resposta = invoke(msg_pedido)) == NULL){
		fprintf(stderr, "Failed invoke\n");
		free(buff_pedido);
		free(msg_pedido);
		return NULL;
	}
	print_message(msg_resposta);

	if(primary && is_write(msg_pedido) && secondary_up){

		if((t_params = (struct thread_params*)malloc(sizeof(struct thread_params*))) == NULL){
			fprintf(stderr, "Failed malloc thread_params\n");
			free(buff_pedido);
			free_message(msg_pedido);
			free_message(msg_resposta);
			secondary_up = 0;

		} else {

			t_params->msg = msg_pedido;
			t_params->server = other_server;

			//criar a thread
			if (pthread_create(&thread, NULL, &secondary_update, (void *) &t_params) != 0){
				perror("\nThread não criada.\n");
				secondary_up = 0;
			}
		}		
	}


	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg_resposta, &buff_resposta)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return NULL;
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
		return NULL;
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(socket_fd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return NULL;
	}

	//verificar que a a thread vez o seu trabalho TODO:
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
	if(strcasecmp( tok, "quit") == 0){
		quit = 1;
		//apagar o ficheiro server.conf, remove() TODO:
		
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

//escreve o ficheiro com a configuracao do servidor TODO: pelo Cruz
int write_file(char *file_name,char **adrport,char ***n_tables){

}

//bind to the other server

struct server_t *server_bind(const char *address_port){
	struct server_t *server;
    if(address_port == NULL){
        return NULL;
    }
    if((server = network_connect(address_port)) == NULL){
		free(server);
        return NULL;
    }
    return server;
}

//main da thread -- vai enviar uma msg ao servidor secundario 
void *secondary_update(void *params){
	struct thread_params *tp = (struct thread_params *) params;
	struct message_t *msg_in;

	msg_in = network_send_receive(&(tp->server), tp->msg);

	int res = (int *) malloc(sizeof(int));
	res = msg_in->content.result;

    free_message(msg_in);
    free_message(tp->msg);
	free(params);
    return res;
}

//devolve uma string da forma <ip>:<port> pronto para escrever TODO: pelo Cruz
char *get_addres_port(struct sockaddr *p_server){

}

//funcao que verifica se a mensagem se trata de uma put ou de um updatez
//se for esse o caso devolve 1
//caso contrario devolve 0
int is_write(struct message_t *msg){ 
	return msg->opcode == OC_PUT || msg->opcode == OC_UPDATE;
}

//envia a informacao das tabelas para o servidor secundario
int send_table_info(struct server_t *server, char **n_tables){
	//initializar a msg a enviar
	struct message_t *msg_out, *msg_in;

	if(server == NULL || n_tables == NULL){
		fprintf(stderr,"send_table_info - bad params");
		return -1;
	}

    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Send_table_info - Failed to malloc!\n");
        return NULL;
    }

	msg_out->opcode = OC_TABLE_INFO;
    msg_out->c_type = CT_KEYS;
	msg_out->table_num = 0;
    msg_out->content.keys = n_tables;	

	msg_in = network_send_receive(server, msg_out);

	int res = msg_in->content.result;
    free_message(msg_in);
    free_message(msg_out);
    return res;
}

char **get_table_info(int socket_fd){ //FIXME: este copy paste todo estah a dar-me cancro, talvez haja outra maneira.
	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result, i;
  	struct message_t *msg_pedido, *msg_resposta;
	char ** n_tables;

	if(socket_fd < 0){
		fprintf(stderr, "Socket dada eh menor que zero\n");
		return NULL;
	}

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/

	/* Verificar se a receção teve sucesso */
	if((result = read_all(socket_fd, (char *) &msg_size, _INT)) <= 0){
		if(result < 0) fprintf(stderr, "Read failed - size read_all\n");
		return NULL;
	}

	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	if((buff_pedido = (char *) malloc(htonl(msg_size))) == NULL){
		fprintf(stderr, "Failedmalloc buff_pedido \n");
		return NULL;
	}

	/* Com a função read_all, receber a mensagem de resposta. */

	/* Verificar se a receção teve sucesso */
	if((result = read_all(socket_fd, buff_pedido, ntohl(msg_size))) < 0){
		if(result < 0) fprintf(stderr, "Read failed - message read_all\n");
		free(buff_pedido);
		return NULL;
	}

	/* Desserializar a mensagem do pedido */
	/* Verificar se a desserialização teve sucesso */
	if ((msg_pedido = buffer_to_message(buff_pedido, msg_size)) == NULL){
		fprintf(stderr, "Failed unmarshalling\n");
		free(buff_pedido);
		free(msg_pedido);
		return NULL;
	}

	int opc = msg_pedido->opcode;

	if(opc != OC_TABLE_INFO){
		fprinf(stderr, "send_table_info - wrong opcode");
		msg_resposta = message_error(SERVER_ERROR);
	} else {
		int n_tables_size = atoi(msg_pedido->content.keys[0]) + 2;

		if((n_tables = (char**) malloc(sizeof(char*) * n_tables_size)) == NULL){
			fprintf(stderr, "get_table_info - failed malloc, n_tables\n");
			msg_resposta = message_error(SERVER_ERROR);
		}

		for (i = 0; i < n_tables_size - 1; i++){
			n_tables[i] = strdup(msg_pedido->content.keys[i]);//FIXME: verify
		}

		n_tables[n_tables_size - 1] = NULL;

		if((msg_resposta = (struct message_t*) malloc(sizeof(struct message_t)))==NULL){
			fprintf(stderr, "get_table_info - failed malloc, msg_resposta\n");
			msg_resposta = message_error(SERVER_ERROR);
		}

		msg_resposta->opcode = opc + 1;
		msg_resposta->c_type = CT_RESULT;
		msg_resposta->table_num = msg_pedido->table_num;
		msg_resposta->content.result = 0;
	}

	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg_resposta, &buff_resposta)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return NULL;
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
		return NULL;
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(socket_fd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return NULL;
	}

	/* Libertar memória */
	free(buff_resposta);
	free(buff_pedido);
	free_message(msg_resposta);
	free_message(msg_pedido);

	return n_tables;
}

int main(int argc, char **argv){
	struct sigaction a;
	int socket_de_escuta, i, j, nfds, res;
	char *in, *token, *nome_ficheiro = "server.conf";//FIXME: talvez seja necessario diferenciar os ficheiros dos 2 servidores, 
	char *port_ip[2];								 //caso sejam criados na mesma maquina
	char **n_tables;
	char **n_tables, **address_port;
	//FIXME: arrumar esta merda

	struct pollfd connections[NFDESC];
	struct sockaddr *p_server;
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

	if((res = read_file(nome_ficheiro,address_port,n_tables))){//se existe o ficheiro este servidor esta a recuperar de um crash
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

			address_port = argv[2];//FIXME: verificar se eh valido
			if((other_server = server_bind(address_port))){
				secondary_up = 1;
				if((send_table_info(other_server,n_tables)) < 0){
					secondary_up = 0;
				}
			}

		} else {//Servidor Secundario, primeira vez

			//malloc do secondary server TODO:
			if (( other_server = (struct server_t*)malloc(sizeof(struct server_t))) == NULL){
				fprintf(stderr, "Failed malloc other_server\n");
				return NULL;
			}
			other_server->sockfd = accept(argv[1],&p_server,&primary_size);//FIXME: verificar se o argv1 é um num, e se o accpet nao deu erro

			address_port = get_addres_port(p_server);//FIXME: verificar o resutlado
			n_tables = get_table_info(other_server->sockfd);//FIXME: verificar o resultado da funcao
			secondary_up = 1;
		}
	}

	
	if((table_skel_init(n_tables) < 0)){
		fprintf(stderr, "Failed to init\n");
		return -1;
	}

	if(first_time){
		// if((write_file(nome_ficheiro, address_port, n_tables)) < 0){
		// 	fprinf(stderr, "Failed to write server.conf");
		// 	return -1;
		// }
	} else {//sync
		other_server = server_bind(address_port);
		hello(other_server);
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
