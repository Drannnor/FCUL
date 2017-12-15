/*
Grupo 41
Alexandre Chícharo 47815
Bruno Andrade 47829
Ricardo Cruz 47871
*/
#ifndef _PRIMARY_BACKUP_PRIVATE_H
#define _PRIMARY_BACKUP_PRIVATE_H

#include "message-private.h"

struct server_t{
    int up;
    int socket_fd;
    int ntabelas;
    char *address_port;
};

pthread_t *backup_update(struct message_t *msg, struct server_t *server);

void *backup_update_thread(void *params);

int server_bind(struct server_t *server);

int send_port(struct server_t *server, char *port);

int get_address_port(struct server_t *server, struct sockaddr *socket_address);

int send_table_info(struct server_t *server, char **n_tables);

char **get_table_info(struct server_t *server);

int update_successful(pthread_t *thread);

struct message_t *server_backup_send_receive(struct server_t *server, struct message_t *msg);

struct message_t *server_backup_receive_send(struct server_t *server);

int sync_backup(struct server_t *server);

int server_backup_put(struct server_t *server, struct entry_t *entry, int tablenum);

int server_close(struct server_t* server);
#endif

