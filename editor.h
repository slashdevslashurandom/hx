/*
 * This file is part of hx - a hex editor for the terminal.
 *
 * Copyright (c) 2016 Kevin Pors. See LICENSE for details.
 */

#ifndef HX_EDITOR_H
#define HX_EDITOR_H

#include "charbuf.h"
#include "thingy.h"
#include <stdbool.h>

/*
 * Mode the editor can be in.
 */
enum editor_mode {
	MODE_APPEND        = 1 << 0, // append value after the current cursor position.
	MODE_APPEND_ASCII  = 1 << 1, // append literal typed value after the current cursor position.
	MODE_REPLACE_ASCII = 1 << 2, // replace literal typed value over the current cursor position.
	MODE_NORMAL        = 1 << 3, // normal mode i.e. for navigating, commands.
	MODE_INSERT        = 1 << 4, // insert values at cursor position.
	MODE_INSERT_ASCII  = 1 << 5, // insert literal typed value at cursor position.
	MODE_REPLACE       = 1 << 6, // replace values at cursor position.
	MODE_COMMAND       = 1 << 7, // command input mode.
	MODE_SEARCH        = 1 << 8, // search mode.
};

/*
 * Search directions.
 */
enum search_direction {
	SEARCH_FORWARD,
	SEARCH_BACKWARD,
};

/*
 * Current status severity.
 */
enum status_severity {
	STATUS_INFO,    // appear as lightgray bg, black fg
	STATUS_WARNING, // appear as yellow bg, black fg
	STATUS_ERROR,   // appear as red bg, white fg
};

#define INPUT_BUF_SIZE 80

/*
 * This struct contains internal information of the state of the editor.
 */
struct editor {
	int octets_per_line; // Amount of octets (bytes) per line. Ideally multiple of 2.
	int grouping;        // Amount of bytes per group. Ideally multiple of 2.

	int line;        // The 'line' in the editor. Used for scrolling.
	int cursor_x;    // Cursor x pos on the current screen
	int cursor_y;    // Cursor y pos on the current screen
	int screen_rows; // amount of screen rows after init
	int screen_cols; // amount of screen columns after init

	enum editor_mode mode; // mode the editor is in

	bool         dirty;          // whether the buffer is modified
	char*        filename;       // the filename currently open
	char*        contents;       // the file's contents
	unsigned int content_length; // length of the contents

	enum status_severity status_severity;     // status severity
	char                 status_message[120]; // status message

	char inputbuffer[INPUT_BUF_SIZE]; // input buffer for commands
	                                   // or search strings etc.
	int inputbuffer_index; // the index of the current typed key shiz.

	char searchstr[INPUT_BUF_SIZE]; // the current search string or NULL if none.

	struct action_list* undo_list; // tail of the list
	struct thingy_table* thingies; // thingy table or NULL if none.
};

/*
 * Initializes the editor struct with basic values.
 */
struct editor* editor_init();

/*
 * Finds the cursor position at the given offset, taking the lines into account.
 * The result is set to the pointers `x' and `y'. We can therefore 'misuse' this
 * to set the cursor position of the editor to a given offset.
 *
 * Note that this function will NOT scroll the editor to the proper line.
 */
void editor_cursor_at_offset(struct editor* e, int offset, int* x, int *y);

/*
 * Deletes the character (byte) at the current cursor position (in other
 * words, the current offset the cursor is at).
 */
void editor_delete_char_at_cursor(struct editor* e);

void editor_delete_char_at_offset(struct editor* e, unsigned int offset);

void editor_free(struct editor* e);

void editor_increment_byte(struct editor* e, int amount);

/*
 * Inserts the character byte at the current offset, or after the current
 * offset if `after' is set to true.
 */
void editor_insert_byte(struct editor* e, char x, bool after);

/*
 * Inserts the character byte at the given offset, or after the given offset if
 * `after' is set to true.
 */
void editor_insert_byte_at_offset(struct editor* e, unsigned int offset, char x, bool after);

/*
 * Moves the cursor. The terminal cursor positions are all 1-based, so we
 * take that into account. When we scroll past boundaries (left, right, up
 * and down) we react accordingly. Note that the cursor_x/y are also 1-based,
 * and we calculate the actual position of the hex values by incrementing it
 * later on with the address size, amount of grouping spaces etc.
 */
void editor_move_cursor(struct editor* e, int dir, int amount);

/*
 * Gets the current offset at which the cursor is.
 */
int editor_offset_at_cursor(struct editor* e);

/*
 * Opens a file denoted by `filename', or exit if the file cannot be opened.
 * The editor struct is used to contain the contents and other metadata
 * about the file being opened.
 */
void editor_openfile(struct editor* e, const char* filename);

/*
 * Processes a manual command input when the editor mode is set
 * to MODE_COMMAND.
 */
void editor_process_command(struct editor* e, const char* cmd);

/*
 * Processes a search string.
 */
void editor_process_search(struct editor* e, const char* str, enum search_direction dir);

/*
 * Reads inputstr and inserts 1 byte per "object" into parsedstr.
 * parsedstr can then be used directly to search the file.
 * err_info is a pointer into inputstr to relevant error information.
 *
 * Objects are:
 *  - ASCII bytes entered normally e.g. 'a', '$', '2'.
 *  - "\xXY" where X and Y match [0-9a-fA-F] (hex representation of bytes).
 *  - "\\" which represents a single '\'
 *
 * parsedstr must be able to fit all the characters in inputstr,
 * including the terminating null byte.
 *
 * On success, PARSE_SUCCESS is returned and parsedstr can be used. On failure,
 * an error from parse_errors is returned, err_info is set appropriately,
 * and parsedstr is undefined.
 *
 * err_info:
 *  PARSE_INVALID_HEX     - pointer "XY..." where XY is the invalid hex code.
 *  PARSE_INVALID_ESCAPE  - pointer to "X..." where X is the invalid character
 *                          following \.
 *  other errors          - inputstr.
 *  success               - inputstr.
 */
int editor_parse_search_string(const char* inputstr, struct charbuf* parsedstr,
			       const char** err_info);

/*
 * Processes a keypress accordingly.
 */
void editor_process_keypress(struct editor* e);

/*
 * This function is looped while in REPLACE mode until two valid hex characters
 * are read from the user. The result is placed in the char pointed to by `out'.
 * `output' is therefore not a string, but a pointer to a single char!
 */
int editor_read_hex_input(struct editor* e, char* output);

/*
 * 'Generic' function to read an input string (such as a command or
 * a search string). An internal buffer 'inputbuffer' is filled, purely
 * for displaying purposes. The actual result will be placed in the
 * `dst' string.
 */
int editor_read_string(struct editor* e, char* dst, int len);

/*
 * Renders ASCII values of the editor's contents to the buffer `b'. The
 * start_offset is used to reference the editor's contents start position
 * to render. The `rownum' specified should be the row number being rendered
 * in an iteration in editor_render_contents. This function will render the
 * selected byte with a different color in the ASCII row to easily identify
 * which byte is being highlighted.
 */
void editor_render_ascii(struct editor* e, int rownum, unsigned int start_offset, struct charbuf* b);

/*
 * Renders the contents of the current state of the editor `e'
 * to the buffer `b'.
 */
void editor_render_contents(struct editor* e, struct charbuf* b);

/*
 * Renders on-line help on the screen. This is implemented without the
 * usage of a MODE since the commands etc. are not applicable in this state.
 */
void editor_render_help(struct editor* e);

/*
 * Renders a ruler at the bottom right part of the screen, containing
 * the current offset in hex and in base 10, the byte at the current
 * cursor position, and how far the cursor is in the file (as a percentage).
 */
void editor_render_ruler(struct editor* e, struct charbuf* buf);

/*
 * Renders the status line to the buffer `b'.
 */
void editor_render_status(struct editor* e, struct charbuf* buf);

/*
 * Refreshes the screen. It uses a temporary buffer to write everything that's
 * eligible for display to an internal buffer, and then 'draws' it to the screen
 * in one call.
 */
void editor_refresh_screen(struct editor* e);

/*
 * Replaces the byte(char) at the current selected offset with the given char.
 */
void editor_replace_byte(struct editor* e, char x);

/*
 * Replaces a byte(char) `x' at the given `offset'.
 */
void editor_replace_byte_at_offset(struct editor* e, unsigned int offset, char x);

/*
 * Scrolls the editor by updating the `line' accordingly, within
 * the bounds of the readable parts of the buffer.
 */
void editor_scroll(struct editor* e, int units);

/*
 * Scrolls the editor to a particular given offset. The given offset
 * can be given any value, the function will limit it to the upper and
 * lower bounds of what can be displayed.
 *
 * The cursor will be centered on the screen.
 */
void editor_scroll_to_offset(struct editor* e, unsigned int offset);

/*
 * Sets editor to mode to one of the modes defined in the enum.
 */
void editor_setmode(struct editor *e, enum editor_mode mode);

/*
 * Sets statusmessage, including color depending on severity.
 */
int editor_statusmessage(struct editor* e, enum status_severity s, const char* fmt, ...);

/*
 * Undoes an action.
 */
void editor_undo(struct editor* e);

/*
 * Redoes an action.
 */
void editor_redo(struct editor* e);

/*
 * Writes the contents of the editor's buffer the to the same filename.
 */
void editor_writefile(struct editor* e);

#endif // _HX_EDITOR_H
