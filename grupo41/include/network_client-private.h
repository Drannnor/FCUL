/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#ifndef _NETWORK_CLIENT_PRIVATE_H
#define _NETWORK_CLIENT_PRIVATE_H

#include "inet.h"
#include "network_client.h"

#define RETRY_TIME 5

struct server_t{
    int socket_fd;
    char* address_port_pri;
    char* address_port_sec;
};

int server_switcharoo(struct server_t *server);

#endif