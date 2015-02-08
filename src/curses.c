/*
 * curses.c:
 *  - TUI written with ncurses
 */

#include <ncurses.h>
#include <stdarg.h>
#include <stdint.h>
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
#define HSCROLLOFF 5

// Number of lines the text window is scrolled
#define VSCROLLOFF 5

// Number of lines in the scrollback buffer
#define SCROLLBACK 1024

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

// How many lines have we scrolled up?
static int32_t scrolled;

// --------

static void
curses_resize(void)
{
    log_info("Terminal resize detected");

	// Alter stdscr, otherwise pads will break
    wresize(stdscr, LINES, COLS);
	wclear(stdscr);
	wnoutrefresh(stdscr);

	// Main window
	wresize(text, SCROLLBACK, COLS);
	wclear(text);
	pnoutrefresh(text, 0, 0, 0, 0, LINES - 3, COLS);

	// Status
	wresize(status, 1, COLS);
	mvwin(status, LINES - 2, 0);
	wclear(status);
	wnoutrefresh(status);

	// Input
	wresize(input, 1, COLS);
	mvwin(input, LINES - 1, 0);
	wclear(input);
	wnoutrefresh(input);

	doupdate();
	log_info_f("New terminal size is: %ix%i", LINES, COLS);
}

/*
 * Scrolls the text window up (positiv
 * offset) or down (negativ offset).
 *
 * offset: Number of lines to scroll
 */
static void
curses_scroll(int32_t offset)
{
	int32_t x, y;

	getyx(text, y, x);

	// No scrollback buffer until now
	if (y < LINES - 3)
	{
		return;
	}

	// Clamp scroll up
	if ((y - LINES + 2 - 1 - scrolled <= 0)
			&& (offset > 0))
	{
		return;
	}

	// Clamp scroll down
	if (scrolled + offset < 0)
	{
		scrolled = 0;
	}
	else
	{
		scrolled += offset;
	}

	/* Scrolls the text window. The math is:
		y:        cursor position
		LINES:    Window height
		+2:		  Compensate input and status line
		-1:       Compensate cursor
		scrolled: Scroll offset */
	pnoutrefresh(text, y - LINES + 2 - 1 - scrolled, 0, 0, 0, LINES - 3, COLS);
	doupdate();
}

// --------

void
curses_init(void)
{
	log_info("Initializing ncurses");

	// Initialize ncurses
	initscr();
	clear();
	start_color();
	cbreak();
	nonl();
	noecho();

	if (!can_change_color())
	{
		log_warn("Terminal cannot change colors");
	}

	log_info_f("Terminal size is: %i:%i", LINES, COLS);

	// And now the Colors
	init_pair(PAIR_HIGHLIGHT, COLOR_GREEN, COLOR_BLACK);
	init_pair(PAIR_INPUT, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_STATUS, COLOR_CYAN, COLOR_BLUE);
	init_pair(PAIR_TEXT, COLOR_WHITE, COLOR_BLACK);

    // Main window
	text = newpad(SCROLLBACK, COLS);
	wbkgd(text, COLOR_PAIR(PAIR_TEXT));
	scrollok(text, TRUE);

	// Status
	status = newwin(1, COLS, LINES - 2, 0);
	wbkgd(status, COLOR_PAIR(PAIR_STATUS));

	// Input
	input = newwin(1, COLS, LINES - 1, 0);
	wbkgd(input, COLOR_PAIR(PAIR_INPUT));
	keypad(input, TRUE);

	// Update everything
	wnoutrefresh(stdscr);
	wnoutrefresh(input);
	wnoutrefresh(status);
	pnoutrefresh(text, 0, 0, 0, 0, LINES - 3, COLS);
	doupdate();

	log_info("Curses initialized");
}

void
curses_input(const char *prompt)
{
	char buffer[1024];
	char *tmp;
	int8_t fin;
	int16_t key;
	int32_t begin;
	int32_t chars;
	int32_t i;
	int32_t num;
    int32_t position;
	int32_t start;
	int32_t x, y;

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


			// Terminal was resized
			case KEY_RESIZE:
				curses_resize();

				wmove(input, 0, 0);
				wclrtoeol(input);
				waddstr(input, prompt);

				break;


			// Scroll up
			case KEY_PPAGE:
				curses_scroll(VSCROLLOFF);
				break;


			// Scroll down
			case KEY_NPAGE:
				curses_scroll(-VSCROLLOFF);
				break;


			// Move cursor left
			case KEY_LEFT:
				if (position <= 0)
				{
					break;
				}

				if (start == position)
				{
					start -= HSCROLLOFF;

					wmove(input, 0, strlen(prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - strlen(prompt); i++)
					{
						waddch(input, buffer[start + i]);
					}

					wmove(input, 0, strlen(prompt) + HSCROLLOFF);
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
					start += HSCROLLOFF;

                    wmove(input, 0, strlen(prompt));
					wclrtoeol(input);

                    for (i = 0; i < COLS - strlen(prompt)
						   && buffer[start + i] != '\0'; i++)
					{
						waddch(input, buffer[start + i]);
					}

					// 1 for the cursor
                    wmove(input, 0, COLS - HSCROLLOFF - 1);
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

				num = COLS - strlen(prompt) - HSCROLLOFF - 1;
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
						start += HSCROLLOFF;

						wmove(input, 0, strlen(prompt));
						wclrtoeol(input);

						// 1 for the cursor
						for (i = 0; i < COLS - strlen(prompt)
								- HSCROLLOFF - 1; i++)
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

	log_info_f("User input: %s", buffer);
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
curses_text(int8_t highlight, const char *fmt, ...)
{
	char *msg;
	int32_t x, y;
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
	getyx(text, y, x);

	if (y < LINES - 3)
	{
		/* If the cursor hasn't reached the bottom
		   of the pad, just show it's beginning. */
		pnoutrefresh(text, 0, 0, 0, 0, LINES - 3, COLS);
	}
	else
	{
		/* If the cursor has reached the bottom of
		   the pad, show the last filled lines. Math:
			y:     Cursor position.
			LINES: Screen height
			+2:    Compensate input und status lines
			-1:    Compensate cursor height */
		pnoutrefresh(text, y - LINES + 2 - 1, 0, 0, 0, LINES - 3, COLS);
	}

	scrolled = 0;
	doupdate();

	free(msg);
}

