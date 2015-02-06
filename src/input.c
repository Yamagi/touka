/*
 * input.c
 *  - Input processing
 */

#include "curses.h"

// ---------

void
process_input(const char *input)
{
	curses_text(input);
	curses_text("\n");
}

