/*
 * This file is part of hx - a hex editor for the terminal.
 *
 * Copyright (c) 2016 Kevin Pors. See LICENSE for details.
 */

#include "thingy.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


struct linked_list {
	unsigned char length;
	unsigned char* key;
	unsigned char* value;
	struct linked_list* next;
};

struct thingy_table {
	unsigned char longest_key;
	unsigned char* values[256]; //values for single-byte sequences
	struct linked_list* mbseqs[256]; //linked list for mb seqs starting with each byte
};

struct thingy_table* thingy_table_init() {
	struct thingy_table* nt = malloc(sizeof(struct thingy_table));
	memset(nt, 0, sizeof(struct thingy_table));
	return nt;
}

int thingy_table_assign(struct thingy_table* tbl, unsigned char length, const unsigned char*
		key, const unsigned char* value) { 

	if (length == 0) return 1;

	if (length == 1) {
		if (tbl->values[key[0]]) {
			//key already exists, erase the old one first
			free(tbl->values[key[0]]);
			tbl->values[key[0]] = strdup((char*)value);
			if (tbl->longest_key < length) tbl->longest_key = length;
			return 0;
		} else {
			//key does not exist
			tbl->values[key[0]] = strdup((char*)value);
			if (tbl->longest_key < length) tbl->longest_key = length;
			return 0;
		}
	} else {
		unsigned char first_char = key[0];

		struct linked_list** curitem = &(tbl->mbseqs[first_char]);

		while (*curitem != NULL) {

			if ( ((*curitem)->length == length) &&
					(memcmp((*curitem)->key,key,length) == 0) ) {
				//matching key already exists, we need
				//to replace the value

				free((*curitem)->value);
				(*curitem)->value = strdup((char*)value);
				return 0;
			}

			curitem=&((*curitem)->next);
		}

		struct linked_list* newitem = malloc(sizeof(struct
					linked_list));

		newitem->length = length;
		newitem->key = strdup((char*)key);
		newitem->value = strdup((char*)value);
		*curitem = newitem;
		if (tbl->longest_key < length) tbl->longest_key = length;
		return 0;
	}
}

int thingy_table_destroy(struct thingy_table* tbl) {
	for (int i=0; i<256; i++) {
		if (tbl->values[i]) free(tbl->values[i]);

		struct linked_list* curitem = tbl->mbseqs[i];

		while (curitem != NULL) {
			struct linked_list* nextitem = curitem->next;
			free(curitem->key);
			free(curitem->value);
			free(curitem);
			curitem = nextitem;
		}
	}
	free(tbl);
	return 0;
}

const unsigned char* thingy_table_search(struct thingy_table* tbl, unsigned char length, const unsigned char* key) {
	if (key == NULL) return NULL;
	if (length == 0) return NULL;
	if (length > tbl->longest_key) return NULL;
	if (length == 1) return tbl->values[key[0]];

	struct linked_list* curitem = tbl->mbseqs[key[0]];

	while (curitem != NULL) {
		if ((curitem->length == length) && (memcmp(key,curitem->key,length) == 0))
			return curitem->value;
		curitem=curitem->next;
	}
	return NULL;
}

int thingy_table_delete(struct thingy_table* tbl, unsigned char length, const unsigned char* key) {
	
	if (key == NULL) return 1;
	if (length == 0) return 1;
	if (length > tbl->longest_key) return 1;
	if (length == 1) {
		if (!tbl->values[key[0]]) return 1;
		free(tbl->values[key[0]]);
		tbl->values[key[0]] = NULL;
		return 0;
	}

	struct linked_list** curitem_p = &(tbl->mbseqs[key[0]]);

	while (*curitem_p != NULL) {

		if (((*curitem_p)->length == length) && (memcmp(key,(*curitem_p)->key,length) == 0)) {
	
			struct linked_list* nextitem = (*curitem_p)->next;
			free((*curitem_p)->key);
			free((*curitem_p)->value);
			free(*curitem_p);
			*curitem_p=nextitem;
			return 0;
		}
		curitem_p=&((*curitem_p)->next);
	}
	return 1;
}

int thingy_table_add_from_string(struct thingy_table* tbl, const char* string) {

	char* sd = (char*)strdup(string);

	char* key_hex = strtok(sd,"=");
	if (!key_hex) {free(sd); return 1;}

	char* value = NULL;

	char* key_pointer = key_hex;

	//these two are special. they mean the the key starts at the next character
	//and the values are a newline and a terminator respectively.

	if (key_hex[0] == '/') { value = "\n"; key_pointer = key_hex+1; }
	if (key_hex[0] == '*') { value = "\0"; key_pointer = key_hex+1; }

	size_t keylen_hex = strspn(key_pointer,"0123456789ABCDEFabcdef"); //find hex span
	if (keylen_hex != strlen(key_pointer)) { free(sd); return 2; }
	size_t keylen = (keylen_hex + 1)/2;
	if (keylen == 0) { free(sd); return 3; } //empty key would be a mistake
	if (keylen > 255) { free(sd); return 4; } //key length is a unsigned char
	char key[keylen];
	memset(key,0,sizeof key);

	for (size_t i=0; i < keylen_hex; i++) {
		int oi = i + (keylen_hex % 2); //if the key has an odd length, it should be read offset,
		// so that 123 turns into 01 23, and not 12 30

		if ((key_pointer[i] >= '0') && (key_pointer[i] <= '9'))
			key[oi/2] |= (key_pointer[i] - '0') << ((oi % 2) ? 0 : 4);
		if ((key_pointer[i] >= 'A') && (key_pointer[i] <= 'F'))
			key[oi/2] |= (key_pointer[i] + 10 - 'A') << ((oi % 2) ? 0 : 4);
		if ((key_pointer[i] >= 'a') && (key_pointer[i] <= 'f'))
			key[oi/2] |= (key_pointer[i] + 10 - 'a') << ((oi % 2) ? 0 : 4);
	}

	if (value) {
		thingy_table_assign(tbl, keylen, (const unsigned char*) key, (const unsigned char*) value);
		free(sd);
		return 0;
	} else {
		value = strtok(NULL,"");
		if (!value) { 
			int r = thingy_table_delete(tbl, keylen, key);
			free(sd);
			return 16 + r;
		}
		thingy_table_assign(tbl, keylen, (const unsigned char*) key, (const unsigned char*) value);
		free(sd);
		return 0;
	}
}

int thingy_table_add_from_file(struct thingy_table* tbl, const char* filename, int* o_thingies_loaded) {

	FILE* file = fopen(filename,"r");
	if (!file) { perror("Unable to open thingy table file"); return 1; }

	char buf[1024];

	int thingy_count = 0;

	char* curstr = NULL;
	while ( (curstr = fgets(buf,1024,file)) != NULL) {

		char* finalnewline = strrchr(curstr,'\n');
		if (finalnewline) *finalnewline = 0; //erase the final newline

		while ( (*curstr == ' ') | (*curstr == '\t') ) curstr++; //skip initial whitespace
		if (*curstr == 0) continue; //skip empty lines
		if (buf[0] == '#') continue; //skip lines that have comments in them

		if (thingy_table_add_from_string(tbl, curstr) == 0) {
			thingy_count++;
		} else {
		}

	}

	if (o_thingies_loaded) *o_thingies_loaded = thingy_count;

	fclose(file);

	return 0;
}

unsigned char thingy_table_longest_key(struct thingy_table* tbl) {
	return tbl->longest_key;
}
