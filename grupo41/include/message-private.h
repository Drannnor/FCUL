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
#include "network_client-private.h"

#define OC_RT_ERROR 99

struct message_t* message_error();
void print_message(struct message_t *msg);

/* Função que recebe uma tabela e uma mensagem de pedido e:
	- aplica a operação na mensagem de pedido na tabela;
	- devolve uma mensagem de resposta com oresultado.
*/
struct message_t *process_message(struct message_t *msg_pedido, struct table_t *tabela);

#endif