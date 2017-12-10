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
#include "primary_backup.h"
#include "primary_backup-private.h"
#include "table_skel-private.h"

#define MAX_SIZE 81

struct thread_params{
	struct server_t *server;
	struct message_t *msg;
};

int server_bind(struct server_t *server){
    int socket_fd;
	struct sockaddr_in *p_server;
	char *delim = ":";
    
    if(server -> address_port == NULL){
        return -1;
    }

	if (( p_server = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in))) == NULL){
		fprintf(stderr, "Failed malloc\n");
		return -1;
	}

	/* Estabelecer ligação ao server:
	
	Preencher estrutura struct sockaddr_in com dados do
	endereço do server. */
	p_server->sin_family = AF_INET; 
	char *adr_p = strdup(server -> address_port);
	if (inet_pton(AF_INET, strtok(adr_p,delim), &p_server->sin_addr) < 1) { // Endereço IP
		printf("Erro ao converter IP\n");
		free(p_server);
		free(adr_p);
		return -1;
	}

	p_server->sin_port = htons(atoi(strtok(NULL,delim)));

	p_server->sin_addr.s_addr = htonl(INADDR_ANY);

	//Criar a socket.
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		fprintf(stderr, "Unable to create socket\n");
		free(p_server);
		free(adr_p);
		return -1;
	}

	//Estabelecer ligação.
	if (connect(socket_fd,(struct sockaddr *)p_server, sizeof(*p_server)) < 0) {
		fprintf(stderr, "Unable to connect to server\n");
		close(socket_fd);
		free(p_server);
		free(adr_p);
		return -1;
	}

	/* Se a ligação não foi estabelecida, retornar NULL */
	free(p_server); 
	free(adr_p);	
	server -> socket_fd = socket_fd;
	return 0;
}

int hello(struct server_t *server){
	struct message_t *msg_pedido, *msg_resposta;

	if((msg_pedido = (struct message_t*) malloc(sizeof(struct message_t)))==NULL){
		fprintf(stderr, "Failed malloc\n");
		return -1;
	}

	msg_pedido -> opcode = OC_HELLO;
	msg_pedido -> c_type = CT_RESULT;
	msg_pedido -> table_num = 0;
	msg_pedido -> content.result = 0;

	msg_resposta = server_backup_send_receive(server, msg_pedido);
	if((msg_resposta -> opcode) == OC_RT_ERROR){ 
			fprintf(stderr, "sync_backup - msg_resposta error");
			free(msg_resposta);
			free(msg_pedido);
			return -1;
	}
	free(msg_resposta);
	free(msg_pedido);
	return update_state(server);
}

int update_state(struct server_t *server){
	int size;
	int i,j;
	struct message_t *msg;

	for(i = 0; i < (server -> ntabelas); i++){
		msg = server_backup_receive_send(server);
		if(msg -> opcode != OC_SIZE){
			fprintf(stderr, "update_state - not size");
			free_message(msg);
			return -1;
		}

		memcpy(&size,&(msg->content.result),_INT);

		if(size < 0){
			fprintf(stderr, "update_state - invalido size");
			free_message(msg);
			return -1;
		}

		free_message(msg);

		for(j = 0; j < size; j++){
			msg = server_backup_receive_send(server);

			if(msg -> opcode != OC_PUT){
				fprintf(stderr, "update_state - not a put");
				free_message(msg);
				return -1;
			}
			free_message(msg);
		}
	}
	return 0;
}

int sync_backup(struct server_t *server){
	int size;
	int i, j;
	struct entry_t **tables_entries;
	struct message_t *msg_pedido, *msg_resposta;

	for(i = 0; i < (server -> ntabelas); i++){
		if((size = table_skel_size(i)) <= 0){
			fprintf(stderr, "sync_backup - incorrect size");
			return -1;
		}
		if((msg_pedido = (struct message_t*) malloc(sizeof(struct message_t)))==NULL){
			fprintf(stderr, "Failed malloc\n");
			return -1;
		}
		msg_pedido -> opcode = OC_SIZE;
		msg_pedido -> c_type = CT_RESULT;
		msg_pedido -> table_num = i;
		msg_pedido -> content.result = size;

		msg_resposta = server_backup_send_receive(server, msg_pedido);
		if((msg_resposta -> opcode) == OC_RT_ERROR){ 
			fprintf(stderr, "sync_backup - msg_resposta error");
			return -1;
		}
		if((tables_entries = table_skel_get_entries(i)) == NULL){
			fprintf(stderr, "sync_backup - tables_entries empty");
			return -1;
		}
		for(j = 0; j < size; j++){
				//faz put de tables_entries[j]
			if((server_backup_put(server, tables_entries[j], i)) < 0){
				fprintf(stderr, "sync_backup - put failed");
				free(tables_entries);
				return -1;
			}	
		}
	}
	free(tables_entries);
	return 0;
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

struct message_t *server_backup_receive_send(struct server_t *server){
  	char *buff_resposta, *buff_pedido;
  	int message_size, msg_size, result;
  	struct message_t *msg_pedido, *msg_resposta;

	/* Verificar parâmetros de entrada */
	if(server->socket_fd < 0){
		fprintf(stderr, "server_backup_receive_send - Socket dada eh menor que zero\n");
		return message_error(SERVER_ERROR);
	}

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/

	/* Verificar se a receção teve sucesso */
	if((result = read_all(server->socket_fd, (char *) &msg_size, _INT)) <= 0){
		if(result < 0){
			fprintf(stderr, "Read failed - size read_all\n");
			return message_error(SERVER_ERROR);
		}
		fprintf(stderr, "Read failed - size - disconnected\n");
		return message_error(CONNECTION_ERROR);
	}

	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	if((buff_pedido = (char *) malloc(htonl(msg_size))) == NULL){
		fprintf(stderr, "Failed malloc buff_pedido \n");
		return message_error(SERVER_ERROR);
	}

	/* Com a função read_all, receber a mensagem de resposta. */

	/* Verificar se a receção teve sucesso */
	if((result = read_all(server->socket_fd, buff_pedido, ntohl(msg_size))) < 0){
		if(result < 0) {
			fprintf(stderr, "Read failed - message read_all\n");
			free(buff_pedido);
			return message_error(SERVER_ERROR);
		}
		fprintf(stderr, "Read failed - msg - disconnected2\n");
		free(buff_pedido);
		return message_error(CONNECTION_ERROR);
	}

	/* Desserializar a mensagem do pedido */
	/* Verificar se a desserialização teve sucesso */
	if ((msg_pedido = buffer_to_message(buff_pedido, msg_size)) == NULL){
		fprintf(stderr, "Failed unmarshalling\n");
		free(buff_pedido);
		free(msg_pedido);
		return message_error(SERVER_ERROR);
	}
	
	print_message(msg_pedido);

	int op_code = msg_pedido -> opcode;

	switch(op_code){
		case OC_PUT:{
			if((msg_resposta = invoke(msg_pedido)) == NULL){
				fprintf(stderr, "Failed invoke\n");
				free(buff_pedido);
				free(msg_pedido);
				msg_resposta = message_error(SERVER_ERROR);
			}
			break;
		}
		case OC_SIZE:{
			msg_resposta = message_success(msg_pedido);
			break;
		}
		case OC_TABLE_INFO:{
			msg_resposta = message_success(msg_pedido);
			break;
		}
		case OC_ADDRESS_PORT:{
			msg_resposta = message_success(msg_pedido);
			break;
		}
		default:{
			fprintf(stderr, "Opcode invalido... server shutting down");
			msg_resposta = message_error(SERVER_ERROR);
		}
	}

	print_message(msg_resposta);

	/* Verificar se a serialização teve sucesso */
	if((message_size = message_to_buffer(msg_resposta, &buff_resposta)) < 0){
		fprintf(stderr, "Failed marshalling\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return message_error(SERVER_ERROR);
	}

	/* Enviar ao cliente o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);

	/* Verificar se o envio teve sucesso */
	if(write_all(server->socket_fd, (char *) &msg_size, _INT) < 0){
		fprintf(stderr, "Write failed - size write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return message_error(CONNECTION_ERROR);
	}

	/* Enviar a mensagem que foi previamente serializada */

	/* Verificar se o envio teve sucesso */
	if(write_all(server->socket_fd, buff_resposta, message_size) < 0){
		fprintf(stderr, "Write failed - message write_all\n");
		free(buff_pedido);
		free_message(msg_pedido);
		free_message(msg_resposta);
		return message_error(CONNECTION_ERROR);
	}
	
	/* Libertar memória */
	free(buff_resposta);
	free(buff_pedido);
	free_message(msg_resposta);

	return msg_pedido;
}

int server_backup_put(struct server_t *server, struct entry_t *entry, int tablenum){
    if(server == NULL || entry == NULL){
        fprintf(stderr, "NULL params");
        return CLIENT_ERROR;
    }

    struct message_t *msg_out;
    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Failed to malloc!\n");
        return CLIENT_ERROR;
    }

    msg_out->opcode = OC_PUT;
    msg_out->c_type = CT_ENTRY;
	msg_out->table_num = tablenum;

    if((msg_out->content.entry = entry_dup(entry)) == NULL){
        fprintf(stderr, "Failed to create entry!\n");
        free_message(msg_out);
    }
    
    print_message(msg_out);

    struct message_t *msg_res;
    msg_res = server_backup_send_receive(server,msg_out);
    print_message(msg_res);

    int res = msg_res->content.result;
	free(entry);
    free_message(msg_res);
    free_message(msg_out);
    return res;
}


pthread_t *backup_update(struct message_t *msg, struct server_t *server){
    struct thread_params *t_params;
    pthread_t *thread;
	if((thread = (pthread_t *)malloc(sizeof(pthread_t))) == NULL){
		fprintf(stderr, "backup_update - thread - failed malloc");
		return NULL;
	}
    
    if((t_params = (struct thread_params*)malloc(sizeof(struct thread_params))) == NULL){
        fprintf(stderr, "backup_update - failed malloc\n");
        return NULL;
    }
    
    t_params->msg = msg;
    t_params->server = server;

    //criar a thread
    if (pthread_create(thread, NULL, &backup_update_thread, (void *) t_params) != 0){
        fprintf(stderr,"\nThread não criada.\n");
        return NULL;
    }    
    return thread;
}

void *backup_update_thread(void *params){
	struct thread_params *tp = (struct thread_params *) params;
    struct message_t *msg_out;
	int *res;

	print_message(tp->msg);

	msg_out = server_backup_send_receive(tp->server, tp->msg);

	print_message(msg_out);

	if((res = (int *) malloc(sizeof(int))) == NULL){
		fprintf(stderr, "backup_update_thread - failed malloc\n");
		return NULL;
	}
    *res = msg_out -> content.result;

    free_message(msg_out);
	free(tp);
    return res;
}

int send_port(struct server_t *server, char *port){
	//initializar a msg a enviar
	struct message_t *msg_out, *msg_in;

	if(server == NULL){
		fprintf(stderr,"send_table_info - bad params");
		return -1;
	}

    if((msg_out = (struct message_t*) malloc(sizeof(struct message_t))) == NULL){
        fprintf(stderr, "Send_table_info - Failed to malloc!\n");
        return -1;
    }

	msg_out->opcode = OC_ADDRESS_PORT;
    msg_out->c_type = CT_KEY;
	msg_out->table_num = 0;
    msg_out->content.key = strdup(port);

	print_message(msg_out);

	msg_in = server_backup_send_receive(server, msg_out);

	print_message(msg_in);

	int res = msg_in->content.result;

    free_message(msg_in);
    free_message(msg_out);
    return res;
}

int get_address_port(struct server_t *server,struct sockaddr *socket_address){
	char *port, *ip;
	struct sockaddr_in *addr;

	struct message_t *msg_pedido;
	
	if((server -> address_port = (char *) malloc(MAX_SIZE)) == NULL){
		fprintf(stderr, "Failed malloc address_port \n");
		return -1;
	}

	msg_pedido = server_backup_receive_send(server);

	int opc = msg_pedido->opcode;

	if(opc != OC_ADDRESS_PORT){
		fprintf(stderr, "get_addres_port - wrong opcode");
		free_message(msg_pedido);
		return -1;
	}

	if((port = strdup(msg_pedido->content.key)) == NULL){
		fprintf(stderr, "get_address_port - strdup failed");
		free_message(msg_pedido);
		return -1;
	}

	addr = (struct sockaddr_in*) &socket_address;
	ip = inet_ntoa(addr ->sin_addr);
	sprintf(server -> address_port, "%s:%s", ip,port);//FIXME: verificar

	free(port);
	free_message(msg_pedido);
	return 0;
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

	print_message(msg_out);

	msg_in = server_backup_send_receive(server, msg_out);

	print_message(msg_in);

	int res = msg_in->content.result;
	msg_out->content.keys = NULL;
    free_message(msg_in);
    free_message(msg_out);
    return res;
}

char **get_table_info(struct server_t *server){
	int i,j;
	char **n_tables;


	struct message_t *msg_pedido;
	msg_pedido = server_backup_receive_send(server);

	int opc = msg_pedido->opcode;

	if(opc != OC_TABLE_INFO){
		fprintf(stderr, "get_table_info - wrong opcode");
		free_message(msg_pedido);
		return NULL;
	}
	int n_tables_size = atoi(msg_pedido->content.keys[0]) + 2;

	if((n_tables = (char**) malloc(sizeof(char*) * n_tables_size)) == NULL){
		fprintf(stderr, "get_table_info - failed malloc, n_tables\n");
		free(msg_pedido);
		return NULL;
	}

	for (i = 0; i < n_tables_size - 1; i++){
		if ((n_tables[i] = strdup(msg_pedido->content.keys[i])) == NULL){
			for(j = 0; j < i; j++){
				free(n_tables[j]);
			}
			free(n_tables);
		}
	}

	n_tables[n_tables_size - 1] = NULL;

	free_message(msg_pedido);
	return n_tables;
}

int update_successful(pthread_t *thread){
 	int *r, res;
    if (pthread_join(*thread, (void **) &r) != 0){
		fprintf(stderr, "pthread_join error");
	}

	res = *r;
	free(r);
	
	return res;
}

int server_close(struct server_t *server){
	if(server == NULL){
		return 0;
	}

	if(server ->address_port != NULL){
		free(server->address_port);
	}


	close(server ->socket_fd);//FIXME: not sure se posso fazer free de fd nao existent
	free(server);
	return 0;
}
