/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include <errno.h>
#include <signal.h>
#include "inet.h"
#include "message.h"
#include "table-private.h"

#define OC_RT_ERROR 99
#define SERVER_ERROR -1
#define CLIENT_ERROR -2
#define CONNECTION_ERROR -3


struct message_t* message_error(int errcode);
void print_message(struct message_t *msg);

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len);

// Função que garante a receção de len bytes através da socket sock,
// armazenando-os em buf.
int read_all(int sock, char *buf, int len);

/* Função que recebe uma tabela e uma mensagem de pedido e:
	- aplica a operação na mensagem de pedido na tabela;
	- devolve uma mensagem de resposta com oresultado.
*/
struct message_t *process_message(struct message_t *msg_pedido, struct table_t *tabela);

#endif