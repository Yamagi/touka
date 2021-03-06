/*
 * curses.c
 * --------
 *
 * Text user interface based upon ncurses. Since
 * input is tighly integrated with the TUI low
 * level input is also part of the file.
 */

#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "curses.h"
#include "misc.h"
#include "input.h"
#include "log.h"
#include "quit.h"

#include "data/list.h"
#include "i18n/i18n.h"

// --------

/*
 * ncurses keyboard defines stalled somewhere
 * around 1985. So let's define some of the
 * most common special keys ourself.
 */
#define KEY_TAB 9
#define KEY_LF 10
#define KEY_CR 13
#define KEY_ESC 27
#define KEY_DEL 127

// Number of events to be replayed at resize.
#define REPLAY (SCROLLBACK * 64)

// --------

// Colors
enum
{
	COLOR_GLOSSARY_RED = 17,
	COLOR_HIGHLIGHT_YELLOW,
	COLOR_PROMPT_GREEN,
	COLOR_ROOM_BLUE,
	COLOR_SCENE_GREEN,
	COLOR_STAT_GREY
};

// Colorpairs
enum
{
	PAIR_GLOSSARY = 1,
	PAIR_HIGHLIGHT,
	PAIR_INPUT,
	PAIR_PROMPT,
	PAIR_ROOM,
	PAIR_SCENE,
	PAIR_STATUS,
	PAIR_TEXT
};

// One message saved for replay
typedef struct
{
	uint32_t color;
	char *msg;
} repl_msg_s;

// Linked list for replay buffer
static list *repl_buf;

// The prompt
char *curses_prompt;

// Curses windows
static WINDOW *input;
static WINDOW *status;
static WINDOW *text;

// How many lines have we scrolled up?
static int32_t scrolled;

// Caches the current status message
static char status_line[STATUSBAR];

// --------

/*********************************************************************
 *                                                                   *
 *                       Callback Functions                          *
 *                                                                   *
 *********************************************************************/

/*
 * Callback for the replay buffer destruction.
 */
void
curses_replay_callback(void *data)
{
	repl_msg_s *repl;
	repl = data;

	free(repl->msg);
	free(repl);
}

// --------

/*********************************************************************
 *                                                                   *
 *                        Support Functions                          *
 *                                                                   *
 *********************************************************************/

/*
 * Special version of strlen() which works
 * with UTF-8 multibyte strings.
 *
 * s: String which length is measured
 */
static size_t
curses_utf8strlen(const char *s)
{
	size_t i;
	size_t j;

	assert(s);

	i = 0;
	j = 0;

	while (s[j])
	{
		if ((s[i] & 0xc0) != 0x80)
		{
			j++;
		}

		i++;
	}

	return j;
}

/*
 * Prints text into the main window.
 *
 * highlight: Highlight status of the text
 * msg: Text to print
 */
static void
curses_print(uint32_t color, const char *msg)
{
	char *first;
	char *last;
	int16_t i;
	int32_t x;

	if (color == TINT_GLOSSARY)
	{
		wattron(text, COLOR_PAIR(PAIR_GLOSSARY));
	}
	else if (color == TINT_HIGH)
	{
		wattron(text, COLOR_PAIR(PAIR_HIGHLIGHT));
	}
	else if (color == TINT_PROMPT)
	{
		wattron(text, COLOR_PAIR(PAIR_PROMPT));
	}
	else if (color == TINT_ROOM)
	{
		wattron(text, COLOR_PAIR(PAIR_ROOM));
	}
	else if (color == TINT_SCENE)
	{
		wattron(text, COLOR_PAIR(PAIR_SCENE));
	}
	else
	{
		wattron(text, COLOR_PAIR(PAIR_TEXT));
	}

	x = getcurx(text);

	// Split line
	if (COLS - x < curses_utf8strlen(msg))
	{
		first = strdup(msg);
		last = NULL;

		for (i = curses_utf8strlen(first); i >= 0; i--)
		{
			if (first[i] == ' ')
			{
				if (i < COLS - x)
				{
					first[i] = '\0';
					last = first + i + 1;

					break;
				}
			}
		}

		if (last)
		{
			waddstr(text, first);
			waddstr(text, "\n");
			waddstr(text, last);

			return;
		}
		else
		{
			waddstr(text, "\n");
		}

		free(first);
	}

	waddstr(text, msg);
}

/*
 * Called at terminal resize. Resizes all
 * windows and replays their content.
 */
static void
curses_resize(void)
{
	int32_t y;
	listnode *cur;
	repl_msg_s *rep;

	// Alter stdscr, otherwise pads will break
	wresize(stdscr, LINES, COLS);
	wclear(stdscr);
	wnoutrefresh(stdscr);

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

	// Replay text
	wresize(text, SCROLLBACK, COLS);
	wclear(text);

	cur = repl_buf->first;

	while (cur)
	{
		rep = cur->data;
		curses_print(rep->color, rep->msg);
		cur = cur->next;
	}

	y = getcury(text);

	/* Scrolls the text window to the same position
	   as before. The math is:
		y:        cursor position
		LINES:    Window height
		+2:		  Compensate input and status line
		-1:       Compensate cursor
		scrolled: Scroll offset */
	pnoutrefresh(text, y - LINES + 2 - 1 - scrolled, 0, 0, 0, LINES - 3, COLS);
	curses_status(status_line);

	doupdate();

	log_info(i18n_curses_termresize);
	log_info_f("%s: %i*%i", i18n_curses_newtermsize, LINES, COLS);
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
	int32_t y;

	y = getcury(text);

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

/*********************************************************************
 *                                                                   *
 *                       Startup and Shutdown                        *
 *                                                                   *
 *********************************************************************/

void
curses_init(void)
{
	log_info(i18n_curses_init);

	repl_buf = list_create();

	if (!curses_prompt)
	{
		curses_prompt = strdup("# ");
	}

	// Reset the character interpretion
	setlocale(LC_CTYPE, "");

	// Initialize ncurses
	initscr();
	clear();
	start_color();
	cbreak();
	nonl();
	noecho();

	if (!can_change_color())
	{
		log_warn(i18n_curses_8colorsonly);

		init_pair(PAIR_GLOSSARY, COLOR_RED, COLOR_BLACK);
		init_pair(PAIR_HIGHLIGHT, COLOR_GREEN, COLOR_BLACK);
		init_pair(PAIR_INPUT, COLOR_WHITE, COLOR_BLACK);
		init_pair(PAIR_PROMPT, COLOR_GREEN, COLOR_BLACK);
		init_pair(PAIR_ROOM, COLOR_BLUE, COLOR_BLACK);
		init_pair(PAIR_SCENE, COLOR_GREEN, COLOR_BLACK);
		init_pair(PAIR_STATUS, COLOR_CYAN, COLOR_BLUE);
		init_pair(PAIR_TEXT, COLOR_WHITE, COLOR_BLACK);
	}
	else
	{
		init_color(COLOR_GLOSSARY_RED, 753, 333, 333);
		init_color(COLOR_HIGHLIGHT_YELLOW, 767, 767, 144);
		init_color(COLOR_PROMPT_GREEN, 168, 613, 277);
		init_color(COLOR_ROOM_BLUE, 473, 753, 895);
		init_color(COLOR_SCENE_GREEN, 266, 856, 203);
		init_color(COLOR_STAT_GREY, 679, 669, 578);

		init_pair(PAIR_GLOSSARY, COLOR_GLOSSARY_RED, COLOR_BLACK);
		init_pair(PAIR_HIGHLIGHT, COLOR_HIGHLIGHT_YELLOW, COLOR_BLACK);
		init_pair(PAIR_INPUT, COLOR_WHITE, COLOR_BLACK);
		init_pair(PAIR_PROMPT, COLOR_PROMPT_GREEN, COLOR_BLACK);
		init_pair(PAIR_ROOM, COLOR_ROOM_BLUE, COLOR_BLACK);
		init_pair(PAIR_SCENE, COLOR_SCENE_GREEN, COLOR_BLACK);
		init_pair(PAIR_STATUS, COLOR_BLACK, COLOR_STAT_GREY);
		init_pair(PAIR_TEXT, COLOR_WHITE, COLOR_BLACK);
	}

	log_info_f("%s: %i*%i", i18n_curses_termsize, LINES, COLS);

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
}

void
curses_quit(void)
{
	log_info(i18n_curses_quit);

	delwin(input);
	delwin(status);
	delwin(text);
	endwin();

	if (repl_buf)
	{
		list_destroy(repl_buf, curses_replay_callback);
	}

	if (curses_prompt)
	{
		free(curses_prompt);
	}
}

// --------

/*********************************************************************
 *                                                                   *
 *                        Input Processing                           *
 *                                                                   *
 *********************************************************************/

void
curses_input(void)
{
	cchar_t render;
	char utf8buf[INPUTBUF * 4];
	char *utf8tmp;
	boolean fin;
	int16_t i;
	int32_t begin;
	int32_t chars;
	int32_t num;
	int32_t position;
	int32_t start;
	int32_t x;
	int32_t y;
	wchar_t buffer[INPUTBUF];
	wchar_t key;
	wchar_t *widetmp;

	memset(buffer, '\0', sizeof(buffer));
	chars = 0;
	fin = false;
	position = 0;
	start = 0;

	wmove(input, 0, 0);
	wclrtoeol(input);
	waddstr(input, curses_prompt);

	while (wget_wch(input, (wint_t *)&key) != ERR)
	{
		switch (key)
		{
			// Process input
			case KEY_ENTER:
			case KEY_LF:
			case KEY_CR:
				fin = true;
				break;


			// Terminal was resized
			case KEY_RESIZE:
				curses_resize();

				wmove(input, 0, 0);
				wclrtoeol(input);
				waddstr(input, curses_prompt);

				start = wcslen(buffer) - (COLS - 1 - curses_utf8strlen(curses_prompt));
				start = start < 0 ? 0 : start;

				for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
							&& buffer[start + i] != L'\0'; i++)
				{
					setcchar(&render, &buffer[start + i], 0, 0, NULL);
					wadd_wch(input, &render);
				}

				if (position > start)
				{
					getyx(input, y, x);
					wmove(input, y, strlen(curses_prompt) + position - start);
				}
				else
				{
					position = chars;
				}

				break;


			// Delete current line
			case KEY_ESC:
				wmove(input, 0, curses_utf8strlen(curses_prompt));
				wclrtoeol(input);

				memset(buffer, 0, sizeof(buffer));
				chars = 0;
				position = 0;
				start = 0;

				input_history_reset();
				input_complete_reset();

				break;


			// History up
			case KEY_UP:
				input_complete_reset();
				utf8tmp = input_history_next();

				if (utf8tmp)
				{
					memset(buffer, 0x00, sizeof(buffer));
					mbstowcs(buffer, utf8tmp, sizeof(buffer));

					start = wcslen(buffer) - (COLS - 1 - curses_utf8strlen(curses_prompt));
					start = start < 0 ? 0 : start;
					chars = wcslen(buffer);

					wmove(input, 0, strlen(curses_prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
								&& buffer[start + i] != L'\0'; i++)
					{
						setcchar(&render, &buffer[start + i], 0, 0, NULL);
						wadd_wch(input, &render);
					}

					position = chars;
				}

				break;


			// History down
			case KEY_DOWN:
				input_complete_reset();
				utf8tmp = input_history_prev();

				if (utf8tmp)
				{
					memset(buffer, 0x00, sizeof(buffer));
					mbstowcs(buffer, utf8tmp, sizeof(buffer));

					start = wcslen(buffer) - (COLS - 1 - curses_utf8strlen(curses_prompt));
					start = start < 0 ? 0 : start;
					chars = wcslen(buffer);

					wmove(input, 0, strlen(curses_prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
								&& buffer[start + i] != L'\0'; i++)
					{
						setcchar(&render, &buffer[start + i], 0, 0, NULL);
						wadd_wch(input, &render);
					}

					position = chars;
				}

				break;


			// Tab completion
			case KEY_TAB:
				input_history_reset();
				wcstombs(utf8buf, buffer, sizeof(utf8buf));
				utf8tmp = input_complete(utf8buf);

				if (utf8tmp)
				{
					memset(buffer, 0x00, sizeof(buffer));
					mbstowcs(buffer, utf8tmp, sizeof(buffer));

					start = wcslen(buffer) - (COLS - 1 - curses_utf8strlen(curses_prompt));
					start = start < 0 ? 0 : start;
					chars = wcslen(buffer);

					wmove(input, 0, curses_utf8strlen(curses_prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
								&& buffer[start + i] != L'\0'; i++)
					{
						setcchar(&render, &buffer[start + i], 0, 0, NULL);
						wadd_wch(input, &render);
					}

					position = chars;
				}

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

					wmove(input, 0, strlen(curses_prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - curses_utf8strlen(curses_prompt); i++)
					{
						setcchar(&render, &buffer[start + i], 0, 0, NULL);
						wadd_wch(input, &render);
					}

					wmove(input, 0, curses_utf8strlen(curses_prompt) + HSCROLLOFF);
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

				wmove(input, 0, curses_utf8strlen(curses_prompt));
				wclrtoeol(input);

				for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
							&& buffer[start + i] != L'\0'; i++)
				{
					setcchar(&render, &buffer[start + i], 0, 0, NULL);
					wadd_wch(input, &render);
				}

				wmove(input, 0, strlen(curses_prompt));

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

					wmove(input, 0, curses_utf8strlen(curses_prompt));
					wclrtoeol(input);

					for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
								&& buffer[start + i] != L'\0'; i++)
					{
						setcchar(&render, &buffer[start + i], 0, 0, NULL);
						wadd_wch(input, &render);
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

				num = COLS - curses_utf8strlen(curses_prompt) - HSCROLLOFF - 1;
				begin = chars - num < 0 ? 0 : chars - num;

				wmove(input, 0, curses_utf8strlen(curses_prompt));
				wclrtoeol(input);

				for (i = 0; i <= num && buffer[begin + i] != L'\0'; i++)
				{
					setcchar(&render, &buffer[begin + i], 0, 0, NULL);
					wadd_wch(input, &render);
				}

				position = chars;

				break;


			// Delete character left of the cursor
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
					buffer[position] = L'\0';
					chars--;
				}
				else
				{
					getyx(input, y, x);
					wmove(input, y, x - 1);
					position--;

					wdelch(input);
					widetmp = buffer + position;

					while (*widetmp != L'\0')
					{
						*widetmp = *(widetmp + 1);
						widetmp++;
					}

					chars--;
				}

				getyx(input, y, x);
				wclrtoeol(input);

				for (i = 0; i < COLS - x && buffer[position + i] != L'\0'; i++)
				{
					setcchar(&render, &buffer[position + i], 0, 0, NULL);
					wadd_wch(input, &render);
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

				widetmp = buffer + position;

				while (*widetmp != L'\0')
				{
					*widetmp = *(widetmp + 1);
					widetmp++;
				}

				chars--;

				getyx(input, y, x);
				wclrtoeol(input);

				for (i = 0; i < COLS - x && buffer[position + i] != L'\0'; i++)
				{
					setcchar(&render, &buffer[position + i], 0, 0, NULL);
					wadd_wch(input, &render);
				}

				wmove(input, y, x);

				break;


             // Normal ASCII input
			default:
				if (position == (sizeof(buffer) - 1))
				{
					break;
				}

				if (key >= 31 && wcwidth(key) == 1)
				{
					getyx(input, y, x);

					if (x == COLS - 1)
					{
						start += HSCROLLOFF;

						wmove(input, 0, curses_utf8strlen(curses_prompt));
						wclrtoeol(input);

						// 1 for the cursor
						for (i = 0; i < COLS - curses_utf8strlen(curses_prompt)
										- HSCROLLOFF - 1; i++)
						{
							setcchar(&render, &buffer[start + i], 0, 0, NULL);
							wadd_wch(input, &render);
						}
					}

					if (position == chars)
					{
						setcchar(&render, &key, 0, 0, NULL);
						wadd_wch(input, &render);
						buffer[position] = key;
						position++;
						chars++;
					}
					else
					{
						getyx(input, y, x);
						setcchar(&render, &key, 0, 0, NULL);
						wins_wch(input, &render);

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

	wcstombs(utf8buf, buffer, sizeof(utf8buf));
	log_info_f("%s: %s", i18n_curses_userinput, utf8buf);
	input_process(utf8buf);
}

// --------

/*********************************************************************
 *                                                                   *
 *                Status And Text Windows Manipulation               *
 *                                                                   *
 *********************************************************************/

void
curses_status(const char *fmt, ...)
{
	char *msg;
	size_t len;
	uint16_t i;
	va_list args;

	// Determine length
	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if ((msg = malloc(len)) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	va_start(args, fmt);
	vsnprintf(msg, len, fmt, args);
	va_end(args);

	wmove(status, 0, 0);
	wclrtoeol(status);

	for (i = 0; i < COLS && msg[i] != '\0'; i++)
	{
		waddch(status, msg[i]);
	}

	misc_strlcpy(status_line, msg, sizeof(status_line));

	wnoutrefresh(status);
	doupdate();

	free(msg);
}

void
curses_text(uint32_t color, const char *fmt, ...)
{
	char *msg;
	int32_t y;
	repl_msg_s *rep;
	size_t len;
	va_list args;

	// Determine length
	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	if ((msg = malloc(len)) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	// Format the message
	va_start(args, fmt);
	vsnprintf(msg, len, fmt, args);
	va_end(args);

	curses_print(color, msg);
	y = getcury(text);

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

	// Save to replay buffer
	if ((rep = malloc(sizeof(repl_msg_s))) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	rep->msg = msg;
	rep->color = color;
	list_push(repl_buf, rep);

	while (repl_buf->count > REPLAY)
	{
		rep = list_shift(repl_buf);

		free(rep->msg);
		free(rep);
	}
}
