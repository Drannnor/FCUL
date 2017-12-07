#ifndef _PRIMARY_BACKUP_PRIVATE_H
#define _PRIMARY_BACKUP_PRIVATE_H

#include "primary_backup.h"
struct server_t{
    int socket_fd;
};


//main da thread -- vai enviar uma msg ao servidor secundario 
void *secondary_update(void *params);

pthread_t backup_update(struct message_t *msg, struct server_t *server);

void *backup_update_thread(void *params);

struct server_t *server_bind(const char *address_port);

char *get_address_port(struct sockaddr *p_server);

int send_table_info(struct server_t *server, char **n_tables);

int update_successful(pthread_t thread);







#endif

