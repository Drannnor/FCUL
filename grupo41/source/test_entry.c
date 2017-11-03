#include "data.h"
#include "entry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

/* Sem comentários! Não há tempo... */

int testDup() {
	char *key = "123abc";
	char *data_s = "1234567890abc";
	struct data_t *value;
	struct entry_t *entry, *entry2;
	int result;

	printf("Módulo entry -> teste entry_dup:");

	if ((value = data_create2(strlen(data_s) + 1, data_s)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry = (struct entry_t *) malloc(sizeof(struct entry_t))) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry->key = strdup(key)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry->value = data_dup(value)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	entry->next = entry;

	assert(entry_dup(NULL) == NULL);
	result = (entry_dup(NULL) == NULL);

	if ((entry2 = entry_dup(entry)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	result = result && (entry->key != entry2->key)
			&& (strcmp(entry->key, entry2->key) == 0)
			&& (entry->value != entry2->value)
			&& (entry->value->datasize == entry2->value->datasize)
			&& (memcmp(entry->value->data, entry2->value->data, entry->value->datasize) == 0)
			&& entry2->next == entry;

	data_destroy(value);
	data_destroy(entry->value);
	free(entry->key);
	free(entry);
	data_destroy(entry2->value);
	free(entry2->key);
	free(entry2);

	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

int testInitialize(){
	char *key = "123abc", *key_p;
	char *data_s = "1234567890abc";
	struct data_t *value, *value_p;
	struct entry_t *entry;
	int result;

	printf("Módulo entry -> teste entry_initialize:");

	if ((value = data_create2(strlen(data_s) + 1, data_s)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry = (struct entry_t *) malloc(sizeof(struct entry_t))) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry->key = strdup(key)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	if ((entry->value = data_dup(value)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	entry->next = entry;

	key_p = entry->key;
	value_p = entry->value;

	entry_initialize(NULL);

	entry_initialize(entry);

	result = entry->key == NULL
			&& entry->value == NULL
			&& entry->next == NULL;

	free(key_p);
	data_destroy(value_p);
	free(entry);
	data_destroy(value);

	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}


int main() {
	int score = 0;

	printf("\nIniciando o teste do módulo entry\n");

	score += testDup();

	score += testInitialize();

	printf("\nResultados do teste do módulo entry: %d em 2\n\n",score);
	return score;
}
