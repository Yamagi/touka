/*
 * curses.c:
 *  - TUI written with ncurses
 */

#include <ncurses.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "curses.h"
#include "input.h"
#include "log.h"
#include "utility.h"

// --------

/*
 * ncurses keyboard defines stalled somewhere
 * around 1985. So let's define some of the
 * most common special keys ourself.
 */
#define KEY_LF 10
#define KEY_CR 13
#define KEY_DEL 127

// --------

// Colorpairs
enum
{
	PAIR_HIGHLIGHT = 1,
	PAIR_INPUT,
	PAIR_STATUS,
	PAIR_TEXT
};

// Curses windows
WINDOW *input;
WINDOW *status;
WINDOW *text;

// --------

void
curses_init(void)
{
	log_info("%s", "Initializing ncurses");

	// Initialize ncurses
	initscr();
	clear();
	start_color();
	cbreak();
	nonl();
	noecho();

	if (!can_change_color())
	{
		log_warn("%s", "Terminal cannot change colors");
	}

	// And now the Colors
	init_pair(PAIR_HIGHLIGHT, COLOR_GREEN, COLOR_BLACK);
	init_pair(PAIR_INPUT, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_STATUS, COLOR_CYAN, COLOR_BLUE);
	init_pair(PAIR_TEXT, COLOR_WHITE, COLOR_BLACK);

    // Main window
	text = newwin(LINES - 2, COLS, 0, 0);
	wbkgd(text, COLOR_PAIR(PAIR_HIGHLIGHT));
	scrollok(text, TRUE);

	// Status
	status = newwin(1, COLS, LINES - 2, 0);
	wbkgd(status, COLOR_PAIR(PAIR_STATUS));

	// Input
	input = newwin(1, COLS, LINES - 1, 0);
	wbkgd(input, COLOR_PAIR(PAIR_INPUT));
	keypad(input, TRUE);

	// Update everything
	wnoutrefresh(input);
	wnoutrefresh(status);
	wnoutrefresh(text);
	doupdate();

	log_info("%s", "Curses initialized");
}

void
curses_input(const char *prompt)
{
	char buffer[1024] = {0};
	int cnt = 0;
	int fin = 0;
	int key;
	int x, y;

	// Show prompt
	wmove(input, 0, 0);
	wclrtoeol(input);
	waddstr(input, prompt);

	while ((key = wgetch(input)) != ERR)
	{

		switch (key)
		{
			// Break on enter / return
			case KEY_ENTER:
			case KEY_LF:
			case KEY_CR:
				fin = 1;
				break;

			// Backspace deletes
			case KEY_BACKSPACE:
			case KEY_DEL:
			case KEY_DC:
				// Get cursor position.
				getyx(input, y, x);

				// Move it backwards, but not into the prompt
				if (strlen(prompt) == x)
				{
					break;
				}
				else
				{
					wmove(input, y, x - 1);
				}

				// And delete the character
				wdelch(input);

				// Delete from buffer
                buffer[cnt - 1] = '\0';
				cnt--;

				break;

			default:
				// Do not overflow
				if (cnt == (sizeof(buffer) - 2))
				{
					break;
				}

				// Only ASCII chars are processed
				if ((key >= 31) && (key <= 126))
				{
					waddch(input, key);
					buffer[cnt] = key;
					cnt++;
				}

				break;
		}

  		// Update after each key stroke
        wnoutrefresh(input);
		doupdate();

		if (fin)
		{
			break;
		}
	}

    // Send to frontend
	log_info("User input: %s", buffer);
	process_input(buffer);
}

void
curses_quit(void)
{
	delwin(input);
	delwin(status);
	delwin(text);

	endwin();
}

void
curses_text(int highlight, const char *fmt, ...)
{
	char *msg;
	size_t len;
	va_list args;

	// Determine length
	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if ((msg = malloc(len)) == NULL)
	{
		perror("PANIC: Couldn't allocate memory");
		quit(1);
	}

	// Format the message
	va_start(args, fmt);
	vsnprintf(msg, len, fmt, args);
	va_end(args);

	// Print it
	if (highlight)
	{
		wattron(text, COLOR_PAIR(PAIR_HIGHLIGHT));
	}
	else
	{
		wattron(text, COLOR_PAIR(PAIR_TEXT));
	}

	waddstr(text, msg);

	// Update
	wnoutrefresh(text);
	doupdate();

	free(msg);
}

