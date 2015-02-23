/*
 * curses.h
 * --------
 *
 * Text User Interface written with ncurses. This is a
 * rather simple approach with only three windows and
 * no global refresh cycle.
 */

#ifndef CURSES_H_
#define CURSES_H_

#include <stdint.h>
#include "main.h"

/*
 * Highlight state:
 *  - COLOR_NORM: White on black
 *  - COLOR_HIGH: Green on Black
 */
enum
{
	COLOR_NORM,
	COLOR_HIGH
};

// --------

/*
 * Initializes ncurses.
 */
void curses_init(void);

/*
 * Processes user key strokes, combines them
 * into a buffer and sends the buffer up into
 * input frontend. This function blocks, until
 * user input was received.
 *
 * prompt: Prompt to display
 */
void curses_input(const char *prompt);

/*
 * Shuts ncurses down.
 */
void curses_quit(void);

/*
 * Prints text to the status bar. The text
 * is cut off at terminal width.
 *
 * msg: Text to print
 */
void curses_status(const char *msg);

/*
 * Prints a string into the main window.
 *
 * string: Text to print
 */
void curses_text(uint32_t color, const char *fmt, ...);

// --------

#endif // CURSES_H_

