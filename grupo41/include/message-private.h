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

struct message_t* message_error(int erro);
void print_message(struct message_t *msg);

#endif