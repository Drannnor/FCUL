#ifndef _TABLE_SKEL_PRIVATE_H
#define _TABLE_SKEL_PRIVATE_H

#include "table_skel.h"

void table_skel_print(int n);
int table_skel_send_tablenum (int socketfd);
struct entry_t **table_skel_get_entries(int numero_da_tabela);
int table_skel_size(int numero_da_tabela);
struct message_t *process_message(struct message_t *msg_pedido, struct table_t *tabela);

#endif
