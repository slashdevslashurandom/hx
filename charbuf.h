/*
 * This file is part of hx - a hex editor for the terminal.
 *
 * Copyright (c) 2016 Kevin Pors. See LICENSE for details.
 */

#include <stdlib.h> // size_t

#ifndef _HX_CHARBUF
#define _HX_CHARBUF


/**
 * This charbuf contains the character sequences to render the current
 * 'screen'. The charbuf is changed as a whole, then written to the screen
 * in one go to prevent 'flickering' in the terminal. The charbuf behaves
 * like a sort-of interface to a changeable array of characters.
 */
struct charbuf {
	char* contents;
	int len;
};

/**
 * Create a charbuf on the heap and return it.
 */
struct charbuf* charbuf_create();

/**
 * Deletes the charbuf's contents, and the charbuf itself.
 */
void charbuf_free(struct charbuf* buf);

/**
 * Appends `what' to the charbuf, writing at most `len' bytes. Note that
 * if we use snprintf() to format a particular string, we have to subtract
 * 1 from the `len', to discard the null terminator character.
 */
void charbuf_append(struct charbuf* buf, const char* what, size_t len);

/**
 * Draws (writes) the charbuf to the screen.
 */
void charbuf_draw(struct charbuf* buf);

#endif // _HX_CHARBUF