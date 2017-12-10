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

#define SERVER_ERROR     -1
#define CLIENT_ERROR     -2
#define CONNECTION_ERROR -3

#define OC_TABLE_INFO   60
#define OC_HELLO 	    70
#define OC_ADDRESS_PORT 80
#define OC_TABLE_NUM    90

struct message_t *message_error(int errcode);
void print_message(struct message_t *msg);
struct message_t *message_success(struct message_t *msg_pedido);

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len);

// Função que garante a receção de len bytes através da socket sock,
// armazenando-os em buf.
int read_all(int sock, char *buf, int len);

#endif