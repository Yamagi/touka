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

// --------

#include <stdint.h>

#include "main.h"

// --------

/*
 * Color in which the text is printed
 */
enum
{
	TINT_GLOSSARY = 0,
	TINT_HIGH,
	TINT_NORM,
	TINT_PROMPT,
	TINT_ROOM,
	TINT_SCENE
};

// The prompt
extern char *curses_prompt;

// --------

/*
 * Initializes ncurses.
 */
void curses_init(void);

/*
 * Shuts ncurses down.
 */
void curses_quit(void);

// --------

/*
 * Processes user key strokes, combines them
 * into a buffer and sends the buffer up into
 * input frontend. This function blocks, until
 * user input was received.
 */
void curses_input(void);

// --------

/*
 * Prints text to the status bar. The text
 * is cut off at terminal width.
 *
 * fmt: Format
 * ...: Parameters to print
 */
void curses_status(const char *fmt, ...);

/*
 * Prints a string into the main window.
 *
 * color: Color in which the text is printed
 * string: Text to print
 */
void curses_text(uint32_t color, const char *fmt, ...);

// --------

#endif // CURSES_H_

