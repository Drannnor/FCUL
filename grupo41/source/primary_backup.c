/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/

#include <error.h>
#include <stdio.h>
#include <pthread.h>
#include "table-private.h"
#include "message-private.h"
#include "primary_backup-private.h"

struct thread_params{
	struct server_t *server;
	struct message_t *msg;
};

int hello(struct server_t *server){//TODO:
    return -1;
}

int update_state(struct server_t *server){//TODO:
    return -1;
}

pthread_t *backup_update(struct message_t *msg, struct server_t *server){
    struct thread_params *t_params;
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));//FIXME:
    
    if((t_params = (struct thread_params*)malloc(sizeof(struct thread_params*))) == NULL){
        fprintf(stderr, "backup_update - failed malloc.\n");
        return NULL;
    }
    
    t_params->msg = msg;
    t_params->server = server;

    //criar a thread
    if (pthread_create(thread, NULL, &backup_update_thread, (void *) &t_params) != 0){
        fprintf(stderr,"\nThread não criada.\n");
        return NULL;
    }
    
    return thread;
}

void *backup_update_thread(void *params){
	struct thread_params *tp = (struct thread_params *) params;
    struct message_t *msg_out;

	msg_out = server_backup_send_receive(tp->server, tp->msg);

    int *res = (int *) malloc(sizeof(int));//FIXME:
    *res = msg_out -> content.result;

    free_message(msg_out);
	free(params);
    return res;
}

struct server_t *server_bind(const char *address_port){
    int socket_fd;
	struct sockaddr_in *p_server;
	char *delim = ":";
	struct server_t *server;
    
    if(address_port == NULL){
        return NULL;
    }

    if (( server = (struct server_t*)malloc(sizeof(struct server_t))) == NULL){
		fprintf(stderr, "Failed malloc\n");
		return NULL;
	}

	if (( p_server = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in))) == NULL){
		fprintf(stderr, "Failed malloc\n");
		free(server);
		return NULL;
	}

	/* Estabelecer ligação ao server:
	
	Preencher estrutura struct sockaddr_in com dados do
	endereço do server. */
	p_server->sin_family = AF_INET;
	
	char *adr_p = strdup(address_port);
	if (inet_pton(AF_INET, strtok(adr_p,delim), &p_server->sin_addr) < 1) { // Endereço IP
		printf("Erro ao converter IP\n");
		free(p_server);
		free(server);
		free(adr_p);
		return NULL;
	}
	p_server->sin_port = htons(atoi(strtok(NULL,delim)));

	//Criar a socket.
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Unable to create socket\n");
		free(p_server);
		free(server);
		free(adr_p);
		return NULL;
	}
	
	//Estabelecer ligação.
	if (connect(socket_fd,(struct sockaddr *)p_server, sizeof(*p_server)) < 0) {
		fprintf(stderr, "Unable to connect to server\n");
		close(socket_fd);
		free(p_server);
		free(server);
		free(adr_p);
		return NULL;
	}

	/* Se a ligação não foi estabelecida, retornar NULL */
	free(p_server); 
	free(adr_p);	
	server->socket_fd = socket_fd;
	return server;
}

//devolve uma string da forma <ip>:<port> pronto para escrever TODO: pelo Cruz
char *get_address_port(struct sockaddr *p_server){
	return NULL;
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
        return -1;
    }

	msg_out->opcode = OC_TABLE_INFO;
    msg_out->c_type = CT_KEYS;
	msg_out->table_num = 0;
    msg_out->content.keys = n_tables;	

	msg_in = server_backup_send_receive(server, msg_out);

	int res = msg_in->content.result;
	msg_out->content.keys = NULL;
    free_message(msg_in);
    free_message(msg_out);
    return res;
}

char **get_table_info(int socket_fd){
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
		fprintf(stderr, "send_table_info - wrong opcode");
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

int update_successful(pthread_t thread){
 	int *r, res;
    if (pthread_join(thread, (void **) &r) != 0){
		fprintf(stderr, "pthread_join error");
	}

	res = *r;
	free(r);
	return res;
}


struct message_t *server_backup_send_receive(struct server_t *server, struct message_t *msg){
	char *message_out;
	int message_size, msg_size, result;
	struct message_t *msg_resposta;


    /* Verificar parâmetros de entrada */
    if (server == NULL){
        fprintf(stderr, "Server dado eh invalido\n");
        return message_error(CLIENT_ERROR); 
    }

    if(msg == NULL){
        fprintf(stderr, "Mensagem dada eh invalida\n");
        return message_error(CLIENT_ERROR);
    }

    /* Serializar a mensagem recebida */

    /* Verificar se a serialização teve sucesso */
    if((message_size = message_to_buffer(msg, &message_out)) < 0){
        fprintf(stderr, "Failed marshalling\n");
        free(message_out);
        return message_error(CLIENT_ERROR);
    }

    /* Enviar ao servidor o tamanho da mensagem que será enviada
    logo de seguida
    */
    msg_size = htonl(message_size);
    /* Verificar se o envio teve sucesso */
    if((result = write_all(server->socket_fd, (char *) &msg_size, _INT)) <= 0){
        fprintf(stderr, "Write failed - size write_all\n");
        free(message_out);
        return message_error(CONNECTION_ERROR);
    }
    

    /* Enviar a mensagem que foi previamente serializada */


    /* Verificar se o envio teve sucesso */
    if((result = write_all(server->socket_fd, message_out, message_size)) <= 0){
        fprintf(stderr, "Write failed - message write_all\n");
        free(message_out);
        return message_error(CONNECTION_ERROR);
    }
   
    /* De seguida vamos receber a resposta do servidor:

        Com a função read_all, receber num inteiro o tamanho da 
        mensagem de resposta. */

    if((result = read_all(server->socket_fd, (char *) &msg_size, _INT)) <= 0){
        fprintf(stderr, "Read failed - size read_all\n");
        free(message_out);
        return message_error(CONNECTION_ERROR);
    }

    /* Alocar memória para receber o número de bytes da
    mensagem de resposta. */
        
    /* Com a função read_all, receber a mensagem de resposta. */
    char *message_in;
    if((message_in = (char *)malloc(ntohl(msg_size))) == NULL){
        printf("Failed malloc - message_in\n");
        free(message_out);
        return message_error(CLIENT_ERROR);
    }
    
    /* Verificar se a receção teve sucesso */
    if((result = read_all(server->socket_fd, message_in, ntohl(msg_size))) <= 0){
        fprintf(stderr, "Read failed - message read_all\n");
        free(message_out);
        free(message_in);
        return message_error(CONNECTION_ERROR);
    }

    /* Desserializar a mensagem de resposta */
    msg_resposta = buffer_to_message(message_in, ntohl(msg_size));

    /* Verificar se a desserialização teve sucesso */
    if (msg_resposta == NULL){
        fprintf(stderr, "Failed unmarshalling\n");
        free(message_out);
        free(message_in);
        return message_error(CLIENT_ERROR);
    }
    

    /* Libertar memória */
    free(message_in);
    free(message_out);
    return msg_resposta;
}