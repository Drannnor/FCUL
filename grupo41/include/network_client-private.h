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

#define RETRY_TIME = 5000;

struct server_t{
	/* Atributos importantes para interagir com o servidor, */
	/* tanto antes da ligação estabelecida, como depois.    */
    //lista de tableas
    int socket_fd;
};

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len);

/* Função que garante a receção de len bytes através da socket sock,
   armazenando-os em buf.
*/
int read_all(int sock, char *buf, int len);

#endif
