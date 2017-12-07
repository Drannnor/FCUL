#ifndef _PRIMARY_BACKUP_PRIVATE_H
#define _PRIMARY_BACKUP_PRIVATE_H

#include "primary_backup.h"
struct server_t{
    int socket_fd;
};


//main da thread -- vai enviar uma msg ao servidor secundario 
void *secondary_update(void *params);

#endif

