/*
 * This file is part of hx - a hex editor for the terminal.
 *
 * Copyright (c) 2016 Kevin Pors. See LICENSE for details.
 */

#ifndef HX_THINGY_H
#define HX_THINGY_H

/*
 * Contains definitions and functions to support "thingy tables" -- files
 * containing substitutions for characters or text sequences that are to be
 * displayed in the right-hand side of the hex editor.
 */ 

struct thingy_table;

struct thingy_table* thingy_table_init();
int thingy_table_assign(struct thingy_table* tbl, unsigned char length, const unsigned char* key, const unsigned char* value);
int thingy_table_destroy(struct thingy_table* tbl);
const unsigned char* thingy_table_search(struct thingy_table* tbl, unsigned char length, const unsigned char* key);
int thingy_table_delete(struct thingy_table* tbl, unsigned char length, const unsigned char* key);
int thingy_table_add_from_string(struct thingy_table* tbl, const char* string);
int thingy_table_add_from_file(struct thingy_table* tbl, const char* filename, int* o_thingies_loaded);
unsigned char thingy_table_longest_key(struct thingy_table* tbl);

#endif
