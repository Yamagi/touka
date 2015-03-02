/*
 * input.c
 * -------
 *
 * Input Processing. The user interaction is done
 * in curses-.c, this file does the nicer job of
 * "upper layer" processing. Each command has a
 * callback function, which does the real work.
 * Also a command completion and input history
 * is implemented.
 */

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "curses.h"
#include "game.h"
#include "input.h"
#include "main.h"
#include "log.h"
#include "save.h"
#include "util.h"

#include "data/darray.h"
#include "data/list.h"

// ---------

// Represents one input command
typedef struct input_cmd
{
	boolean alias;
	const char *help;
	const char *name;
	void (*callback)(char *msg);
} input_cmd;

// Holds all input command
static darray *input_cmds;

// The history
static list *history;

// Current hist_position in the history
static listnode *hist_position;

// Current hist_position for tab completes
static int32_t tab_position;

// Buffers the completion stub
static char *tab_stub;

// ---------

/*********************************************************************
 *                                                                   *
 *                        Command Callbacks                          *
 *                                                                   *
 *********************************************************************/

/*
 * Prints a list of all glossary entries or descripes
 * the specified entry. Error handling is done in the
 * called functions.
 */
static void
cmd_glossary(char *msg)
{
	if (msg)
	{
		game_glossary_print(msg);
	}
	else
	{
		game_glossary_list();
	}
}

/*
 * Prints a list with all commands and
 * their associated help texts.
 */
static void
cmd_help(char *msg)
{
	input_cmd *cur;
	size_t len;
	uint16_t i;

	len = 0;

	for (i = 0; i < input_cmds->elements; i++)
	{
		cur = darray_get(input_cmds, i);

		if (strlen(cur->name) > len)
		{
			len = strlen(cur->name);
		}
	}

	curses_text(COLOR_NORM, "%-*s %s\n", len + 1, "Command", "Description");
	curses_text(COLOR_NORM, "%-*s %s\n", len + 1, "-------", "-----------");

	for (i = 0; i < input_cmds->elements; i++)
	{
		cur = darray_get(input_cmds, i);

        if (cur->alias)
		{
			continue;
		}

		curses_text(COLOR_NORM, "%-*s %s\n", len + 1, cur->name, cur->help);
	}

	log_info_f("Listed %i commands", i);
}

/*
 * Prints game info.
 */
static void
cmd_info(char *msg)
{
	curses_text(COLOR_NORM, "Attribute  Value\n");
	curses_text(COLOR_NORM, "---------  -----\n");
	curses_text(COLOR_NORM, "Game       %s\n", game_header->game);
	curses_text(COLOR_NORM, "Author     %s\n", game_header->author);
	curses_text(COLOR_NORM, "Date       %s\n", game_header->date);
	curses_text(COLOR_NORM, "UID        %s\n", game_header->uid);
}

/*
 * Loads a savegame and replays the last scene.
 * But only if the game was successfull loaded,
 * of course.
 */
static void
cmd_load(char *msg)
{
	if (msg)
	{
		if (save_read(msg))
		{
			game_scene_play(NULL);
		}
	}
	else
	{
		save_list();
	}
}

/*
 * Advances the game to the next scene and plays it.
 */
static void
cmd_next(char *msg)
{
	uint8_t choice;

	if (!msg)
	{
		if (!game_scene_next(0))
		{
			return;
		}
	}
	else
	{
		if (strlen(msg) != 1)
		{
			curses_text(COLOR_NORM, "Invalid choice");

			return;
		}
		else
		{
			if (isdigit(msg[0]))
			{
				choice = atoi(&msg[0]);

				if (!choice)
				{
					curses_text(COLOR_NORM, "Invalid choice");

					return;
				}

				if (!game_scene_next(choice))
				{
                    return;
				}
			}
		}
	}

	game_scene_play(NULL);
}

/*
 * Shuts the application cleanly down.
 */
static void
cmd_quit(char *msg)
{
	quit_success();
}

/*
 * List all rooms or descripes a room.
 * Error handling is done by the called
 * function.
 */
static void
cmd_room(char *msg)
{
	if (!msg)
	{
		game_rooms_list();
	}
	else
	{
		game_room_describe(msg);
	}
}

/*
 * Saves the game to a file with the given
 * name.
 */
static void
cmd_save(char *msg)
{
	if (msg)
	{
		save_write(msg);
	}
}

/*
 * Replays a scene. If no scene name is given,
 * the last scene is replayed. Error handling
 * is done by the called function.
 */
static void
cmd_scene(char *msg)
{
	if (!msg)
	{
		game_scene_list();
	}
	else
	{
		game_scene_play(msg);
	}
}

/*
 * Prints the version number and copyright.
 */
static void
cmd_version(char *msg)
{
	curses_text(COLOR_NORM, "This is %s %s, (c) %s %s\n", APPNAME, VERSION, YEAR, AUTHOR);
	curses_text(COLOR_NORM, "This binary was build on %s\n", __DATE__);
}

// ---------

/*********************************************************************
 *                                                                   *
 *                       Command Registration                        *
 *                                                                   *
 *********************************************************************/

/*
 * Callback function to qsort for the command darray.
 */
int32_t
input_sort_callback(const void *msg1, const void *msg2)
{
	const input_cmd *a;
	const input_cmd *b;
	int32_t ret;

	a = *(const input_cmd **)msg1;
	b = *(const input_cmd **)msg2;

	ret = strcmp(a->name, b->name);

	return ret;
}

/*
 * Registers a new command.
 *
 * name: Name of the command
 * help: A short help text
 * callback: Callback function for that command
 * alias: The command is an alias
 */
static void
input_register(const char *name, const char *help, void (*callback)(char *msg), boolean alias)
{
	input_cmd *new;

    assert(name);
	assert(help);
	assert(callback);

	if (!input_cmds)
	{
		input_cmds = darray_create();
	}

	if ((new = malloc(sizeof(input_cmd))) == NULL)
	{
		quit_error("Couldn't allocate memory");
	}

	new->name = name;
	new->help = help;
	new->callback = callback;
	new->alias = alias;

	darray_push(input_cmds, new);
	darray_sort(input_cmds, input_sort_callback);
}

// ---------

/*********************************************************************
 *                                                                   *
 *                          Input History                            *
 *                                                                   *
 *********************************************************************/

char *
input_history_next(void)
{
	char *data;

	if (hist_position)
	{
		data = hist_position->data;
		hist_position = hist_position->next;

		return data;
	}
	else
	{
		return NULL;
	}
}

char *
input_history_prev(void)
{
	if (!history->first)
	{
		return NULL;
	}

	// Special case for last element
	if (history->first && !hist_position)
	{
		hist_position = history->last;

		return hist_position->prev->data;
	}

	if (hist_position->prev)
	{
		hist_position = hist_position->prev;

		if (hist_position->prev)
		{
			return hist_position->prev->data;
		}
		else
		{
			hist_position = hist_position->next;
		}
	}

	return NULL;
}

void
input_history_reset(void)
{
	hist_position = history->first;
}

// ---------

/*********************************************************************
 *                                                                   *
 *                        Command Completion                         *
 *                                                                   *
 *********************************************************************/

char *
input_complete(char *msg)
{
	input_cmd *cur;

	if (!tab_stub)
	{
		tab_stub = strdup(msg);
	}

	if (tab_position >= input_cmds->elements)
	{
		tab_position = 0;
	}

	while (tab_position < input_cmds->elements)
	{
		cur = darray_get(input_cmds, tab_position);
		tab_position++;

		// Stub changed
		if (strncmp(msg, tab_stub, strlen(tab_stub)))
		{
			free(tab_stub);

			tab_stub = strdup(msg);
			tab_position = 0;
		}

		if (cur->alias)
		{
			continue;
		}

		// Exact match -> next one please
		if (!strcmp(cur->name, msg))
		{
			continue;
		}

		if (!strncmp(cur->name, tab_stub, strlen(tab_stub)))
		{
			return (char *)cur->name;
		}
	}

	return NULL;
}

void
input_complete_reset(void)
{
	free(tab_stub);

	tab_stub = NULL;
	tab_position = 0;
}

// ---------

/*********************************************************************
 *                                                                   *
 *                   Initialization and Shutdown                     *
 *                                                                   *
 *********************************************************************/

void
input_init(void)
{
	log_info("Initializing input");

	// Register commands
	input_register("glossary", "Prints a glossary entry", cmd_glossary, FALSE);
	input_register("help", "Prints this help", cmd_help, FALSE);
	input_register("info", "Prints informations about the current game", cmd_info, FALSE);
	input_register("load", "Loads a saved game", cmd_load, FALSE);
	input_register("n", "Advances to the next scene", cmd_next, TRUE);
	input_register("next", "Advances to the next scene", cmd_next, FALSE);
	input_register("quit", "Exits the application", cmd_quit, FALSE);
	input_register("room", "Descripes a room", cmd_room, FALSE);
	input_register("save", "Saves the game", cmd_save, FALSE);
	input_register("scene", "Replays a scene", cmd_scene, FALSE);
	input_register("version", "Prints the version number", cmd_version, FALSE);

	// Initialize history
	history = list_create();
}

void
input_quit(void)
{
	if (history)
	{
		list_destroy(history, NULL);
	}

	if (input_cmds)
	{
		darray_destroy(input_cmds, NULL);
	}
}

// ---------

/*********************************************************************
 *                                                                   *
 *                         Input Processing                          *
 *                                                                   *
 *********************************************************************/

void
input_process(char *cmd)
{
	boolean match;
	char *tmp;
	char *token;
	input_cmd *cur;
	size_t len;
	uint16_t i;

	// Strip whitespaces
	while (cmd[0] == ' ')
	{
		cmd++;
	}

	len = strlen(cmd) - 1;

	while (cmd[len] == ' ')
	{
		cmd[len] = '\0';
		len--;
	}

	// Null command is an alias for "next"
	if (!strlen(cmd))
	{
		cmd = "next";
	}

	// Echo the input
	curses_text(COLOR_PROMPT, "> ");
	curses_text(COLOR_NORM, "%s\n", cmd);

	// And put into history
	list_unshift(history, strdup(cmd));

	while (history->count > HISTSIZE)
	{
		tmp = list_pop(history);
		free(tmp);
	}

	input_history_reset();
	input_complete_reset();

	// Ignore comments
	if (cmd[0] == '%')
	{
		return;
	}

	token = strsep(&cmd, " ");
	match = FALSE;

    for (i = 0; i < input_cmds->elements; i++)
	{
		cur = darray_get(input_cmds, i);

		if(!strcmp(token, cur->name))
		{
			cur->callback(cmd);
			match = TRUE;
		}
	}

	if (!match)
	{
		curses_text(COLOR_NORM, "%s: command not found\n", token);
	}

	// Empty line after each cmd-output
	curses_text(COLOR_NORM, "\n");
}

