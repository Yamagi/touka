/*
 * curses.c:
 *  - TUI written with ncurses
 */

#include <ncurses.h>
#include <string.h>

#include "curses.h"
#include "log.h"

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
	PAIR_HIGHLIGHT,
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
	wbkgd(text, COLOR_PAIR(PAIR_TEXT));

	// Status
	status = newwin(1, COLS, LINES - 2, 0);
	wbkgd(status, COLOR_PAIR(PAIR_STATUS));

	// Input
	input = newwin(1, COLS, LINES - 1, 0);
	wbkgd(input, COLOR_PAIR(PAIR_INPUT));
	keypad(input, TRUE);

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
	move(LINES - 1, 0);
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

        wnoutrefresh(input);
		doupdate();

		if (fin)
		{
			break;
		}
	}

	log_info("User input: %s", buffer);
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
curses_update(void)
{
	wnoutrefresh(stdscr);
	wnoutrefresh(input);
	wnoutrefresh(status);
	wnoutrefresh(text);

	doupdate();
}

