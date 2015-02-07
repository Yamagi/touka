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

// Number of characters the input is scrolled
#define SCROLLOFF 5

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
static WINDOW *input;
static WINDOW *status;
static WINDOW *text;

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
	char buffer[1024];
	char *tmp;
	int begin;
	int chars;
	int fin;
	int i;
	int key;
	int num;
    int position;
	int start;
	int x, y;

	memset(buffer, '\0', sizeof(buffer));
	chars = 0;
	fin = 0;
	position = 0;
	start = 0;

	wmove(input, 0, 0);
	wclrtoeol(input);
	waddstr(input, prompt);

	while ((key = wgetch(input)) != ERR)
	{

		switch (key)
		{
			// Process input
			case KEY_ENTER:
			case KEY_LF:
			case KEY_CR:
				fin = 1;
				break;


			// Move cursor left
			case KEY_LEFT:
				if (position <= 0)
				{
					break;
				}

				if (start == position)
				{
					start -= SCROLLOFF;

					wmove(input, 0, strlen(prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - strlen(prompt); i++)
					{
						waddch(input, buffer[start + i]);
					}

					wmove(input, 0, strlen(prompt) + SCROLLOFF);
				}

				getyx(input, y, x);
				wmove(input, y, x - 1);
				position--;

				break;


			// Move cursor to the start
			case KEY_HOME:
				if (position <= 0)
				{
					break;
				}

				position = 0;
				start = 0;

				wmove(input, 0, strlen(prompt));
				wclrtoeol(input);

				for (i = 0; i < COLS - strlen(prompt)
					   && buffer[start + i] != '\0'; i++)
				{
					waddch(input, buffer[start + i]);
				}

				wmove(input, 0, strlen(prompt));

				break;


			// Move cursor right
			case KEY_RIGHT:
				if (position >= chars)
				{
					break;
				}

				getyx(input, y, x);

				if (x == (COLS - 1))
				{
					start += SCROLLOFF;

                    wmove(input, 0, strlen(prompt));
					wclrtoeol(input);

                    for (i = 0; i < COLS - strlen(prompt)
						   && buffer[start + i] != '\0'; i++)
					{
						waddch(input, buffer[start + i]);
					}

					// 1 for the cursor
                    wmove(input, 0, COLS - SCROLLOFF - 1);
				}

				getyx(input, y, x);
				wmove(input, y, x + 1);
				position++;

				break;


			// Move cursor to the end
			case KEY_END:
				if (position >= chars)
				{
					break;
				}

				num = COLS - strlen(prompt) - SCROLLOFF - 1;
				begin = chars - num < 0 ? 0 : chars - num;

				wmove(input, 0, strlen(prompt));
				wclrtoeol(input);

				for (i = 0; i <= num && buffer[begin + i] != '\0'; i++)
				{
					waddch(input, buffer[begin + i]);
				}

				position = chars;

				break;


			// Delete character right of the cursor
			case KEY_BACKSPACE:
			case KEY_DEL:
				if (position == 0)
				{
					break;
				}

                if (position == chars)
				{
					getyx(input, y, x);
					wmove(input, y, x - 1);
					position--;

					wdelch(input);
					buffer[position] = '\0';
					chars--;
				}
				else
				{
					getyx(input, y, x);
					wmove(input, y, x - 1);
					position--;

					wdelch(input);
					tmp = buffer + position;

					while (*tmp != '\0')
					{
						*tmp = *(tmp + 1);
						tmp++;
					}

					chars--;
				}

				getyx(input, y, x);
				wclrtoeol(input);

				for (i = 0; i < COLS - x && buffer[position + i] != '\0'; i++)
				{
					waddch(input, buffer[position + i]);
				}

				wmove(input, y, x);

				break;


			// Delete character under the cursor
			case KEY_DC:
				if (position == chars)
				{
					break;
				}

				getyx(input, y, x);
				wdelch(input);

				tmp = buffer + position;

				while (*tmp != '\0')
				{
					*tmp = *(tmp + 1);
					tmp++;
				}

				chars--;

				getyx(input, y, x);
				wclrtoeol(input);

				for (i = 0; i < COLS - x && buffer[position + i] != '\0'; i++)
				{
					waddch(input, buffer[position + i]);
				}

				wmove(input, y, x);

				break;


             // Normal ASCII input
			default:
				if (position == (sizeof(buffer) - 1))
				{
					break;
				}

				if ((key >= 31) && (key <= 126))
				{
					getyx(input, y, x);

                    if (x == COLS - 1)
					{
						start += SCROLLOFF;

						wmove(input, 0, strlen(prompt));
						wclrtoeol(input);

						// 1 for the cursor
						for (i = 0; i < COLS - strlen(prompt)
								- SCROLLOFF - 1; i++)
						{
							waddch(input, buffer[start + i]);
						}
					}

					if (position == chars)
					{
						waddch(input, key);
						buffer[position] = key;
						position++;
						chars++;
					}
					else
					{
						getyx(input, y, x);
						winsch(input, key);

						/* This is a hack. Normaly the character
						   is inserted under the cursor. Move the
						   cursor one character to the right to
						   simulate the expected behavior. */
						wmove(input, y, x + 1);

                        for (i = chars; i >= position; i--)
						{
							buffer[i + 1] = buffer[i];
						}

						buffer[position] = key;
						position++;
						chars++;
					}
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

