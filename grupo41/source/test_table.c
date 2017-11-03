#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "table-private.h"

/**************************************************************/

int testTableDestroy(){
	struct table_t *table;

	printf("Módulo table -> teste table destroy:");

	table_destroy(NULL);

	if ((table = table_create(10)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	table_destroy(table);

	printf(" passou\n");
	return 1;
}

/**************************************************************/

int testTabelaVazia() {
	struct table_t *table;
	int result;

	printf("Módulo table -> teste table vazia:");

	assert(table_create(0) == NULL);
	result = table_create(0) == NULL;

	result = result && 
		 (table = table_create(5)) != NULL &&
		 table_size(table) == 0;

	table_destroy(table);

	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int testPutInexistente() {
	int result, i;
	struct table_t *table;
	char *key[1024];
	struct data_t *data[1024], *d;

	printf("Módulo table -> teste put inexistente:");

	if ((table = table_create(1024)) == NULL)
		error(1, errno, "  O teste não pode prosseguir");

	for(i = 0; i < 1024; i++) {
		key[i] = (char*) malloc(16 * sizeof(char));
		sprintf(key[i], "a/key/b-%d", i);
		data[i] = data_create2(strlen(key[i]) + 1, key[i]);
		table_put(table, key[i], data[i]);
	}

	result = (table_size(table) == 1024);

	for(i=0; i<1024; i++) {
		d = table_get(table, key[i]);

		assert(d->datasize == data[i]->datasize);
		assert(memcmp(d->data, data[i]->data, d->datasize) == 0);
		assert(d->data != data[i]->data);

		result = result &&
			 d->datasize == data[i]->datasize &&
			 memcmp(d->data, data[i]->data, d->datasize) == 0 &&
 			 d->data != data[i]->data;

		data_destroy(d);
		data_destroy(data[i]);
		free(key[i]);
	}

	table_destroy(table);
	
	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int testPutExistente() {
	int result, i;
	struct table_t *table;
	char *key[1024];
	struct data_t *data[1024], *d;

	printf("Modulo table -> teste put existente:");

	table = table_create(2048);

	for(i = 0; i < 1024; i++) {
		key[i] = (char*) malloc(16 * sizeof(char));
		sprintf(key[i], "a/key/b-%d", i);
		data[i] = data_create2(strlen(key[i]) + 1, key[i]);
		table_put(table, key[i], data[i]);
	}

	for(i=0; i<1024; i++)
		table_put(table, key[i], data[i]);

	assert(table_size(table) == 1024);
	result = (table_size(table) == 1024);

	for(i = 0; i < 1024; i++) {
		free(key[i]);
		data_destroy(data[i]);
	}

	table_destroy(table);
	
	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int testExceedSize() {
	int result, i;
	struct table_t *table;
	char *key[1024];
	struct data_t *data[1024], *d;

	printf("Modulo table -> teste exceed size:");

	table = table_create(1024);

	for(i = 0; i < 1024; i++) {
		key[i] = (char*) malloc(16 * sizeof(char));
		sprintf(key[i], "a/key/b-%d", i);
		data[i] = data_create2(strlen(key[i]) + 1, key[i]);
		table_put(table, key[i], data[i]);
	}

	table_put(table, "abcdef", data[i - 1]);

	assert(table_size(table) == 1024);
	result = (table_size(table) == 1024);

	result = result && (table_get(table, "abcdef") == NULL);

	for(i = 0; i < 1024; i++) {
		free(key[i]);
		data_destroy(data[i]);
	}

	table_destroy(table);
	
	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int testUpdate() {
	int result, i;
	struct table_t *table;
	char *key[1024];
	char *key2[1024];
	struct data_t *data[1024], *d;

	printf("Módulo table -> teste update:");
	table = table_create(1024);

	for(i = 0; i < 1024; i++) {
		key[i] = (char*) malloc(13 * sizeof(char));
		sprintf(key[i], "a/key/b-%d", i);
		key2[i] = (char*) malloc(14 * sizeof(char));
		sprintf(key2[i], "a/key/bb-%d", i);
		data[i] = data_create2(strlen(key[i]) + 1, key[i]);
		table_put(table, key[i], data[i]);
		data_destroy(data[i]);
	}

	for(i = 0; i < 1024; i++) {
		data[i] = data_create2(strlen(key2[i]) + 1, key2[i]);
		table_update(table, key[i], data[i]);
	}

	assert(table_size(table) == 1024);

	result = (table_size(table) == 1024);

	for(i = 0; i < 1024; i++) {
		d = table_get(table, key[i]);
		
		result = result &&
			 d->datasize == data[i]->datasize &&
			 memcmp(d->data, data[i]->data, d->datasize) == 0 &&
			 d->data != data[i]->data;

		data_destroy(d);
		data_destroy(data[i]);
		free(key[i]);
		free(key2[i]);
	}

	table_destroy(table);	
	
	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int testGetKeys() {
	int result = 1, i, j, achou;
	struct table_t *table;
	char **keys;
	char *k[4] = {"abc","bcd","cde","def"};
	struct data_t *d;

	printf("Módulo table -> teste sacar chaves:");

	table = table_create(4);
	d = data_create(5);

	table_put(table, k[2], d);
	table_put(table, k[3], d);
	table_put(table, k[1], d);
	table_put(table, k[0], d);

	data_destroy(d);

	keys = table_get_keys(table);
	
	for(i = 0; keys[i] != NULL; i++) {
		achou = 0;
		for(j = 0; j < 4; j++)
			achou = (achou || (strcmp(keys[i], k[j]) == 0));
		result = (result && achou);
	}

	result = result &&
		 (table_size(table) == i);

	table_free_keys(keys);

	table_destroy(table);

	printf(" %s\n", result ? "passou" : "não passou");
	return result;
}

/**************************************************************/

int main() {
	int score = 0;

	printf("\nIniciando o teste do módulo table\n");

	score += testTableDestroy();

	score += testTabelaVazia();

	score += testPutInexistente();

	score += testPutExistente();

	score += testExceedSize();

	score += testUpdate();

	score += testGetKeys();

	printf("Resultados do teste do módulo table: %d em 7\n\n", score);

	return score;
}
