#ifndef _PRIMARY_BACKUP_PRIVATE_H
#define _PRIMARY_BACKUP_PRIVATE_H

//#include "primary_backup.h"
#include "message-private.h" //FIXME:

struct server_t{
    int up;
    int socket_fd;
    int ntabelas;
};

pthread_t *backup_update(struct message_t *msg, struct server_t *server);

void *backup_update_thread(void *params);

struct server_t *server_bind(const char *address_port);

char *get_address_port(struct server_t *server, struct sockaddr *p_server);

int send_table_info(struct server_t *server, char **n_tables);

char **get_table_info(int socket_fd);

int update_successful(pthread_t *thread);

struct message_t *server_backup_send_receive(struct server_t *server, struct message_t *msg);

int server_backup_receive_send(struct server_t *server);

int sync_backup(struct server_t *server);

int server_backup_put(struct server_t *server, struct entry_t *entry, int tablenum);
#endif

