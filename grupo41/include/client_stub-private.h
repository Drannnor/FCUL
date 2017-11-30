#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "client_stub.h"
#include "network_client-private.h"

//declarar a estrutura
struct rtables_t{
    struct server_t *server; 
    int numberOfTables;
    int table_index;
};

#endif
