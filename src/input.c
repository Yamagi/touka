/*
 * input.c
 *  - Input processing
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "curses.h"
#include "input.h"
#include "main.h"
#include "log.h"
#include "util.h"
#include "data/darray.h"
#include "data/list.h"

// ---------

// History size
#define HISTSIZE 512

// ---------

// Represents one input command
typedef struct input_cmd
{
	const char *name;
	const char *help;
	void (*callback)(char *msg);
} input_cmd;

// Holds all input command
static darray *input_cmds;

// The history
static list *history;

// Current position in the history
static listnode *position;

// ---------

static void
cmd_help(char *msg)
{
	int32_t i;
	int32_t len;
	input_cmd *cur;

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
		curses_text(COLOR_NORM, "%-*s %s\n", len + 1, cur->name, cur->help);
	}
}

static void
cmd_quit(char *msg)
{
	quit_success();
}

static void
cmd_version(char *msg)
{
	curses_text(COLOR_NORM, "This is %s %s, (c) %s %s\n", APPNAME, VERSION, YEAR, AUTHOR);
	curses_text(COLOR_NORM, "This binary was build on %s.\n", __DATE__);
}

// ---------

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

// ---------

static void
input_register(const char *name, const char *help, void (*callback)(char *msg))
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
		perror("PANIC: Couldn't allocate memory");
		quit_error();
	}

	new->name = name;
	new->help = help;
	new->callback = callback;

	darray_push(input_cmds, new);
	darray_sort(input_cmds, input_sort_callback);
}

// ---------

char *
input_history_next(void)
{
	char *data;

	if (position)
	{
		data = position->data;
		position = position->next;

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
	// Special case for last element
	if (history->first && !position)
	{
		position = history->last;

		return position->prev->data;
	}

	if (position->prev)
	{
		position = position->prev;

		if (position->prev)
		{
			return position->prev->data;
		}
		else
		{
			position = position->next;
		}
	}

	return NULL;
}

void
input_init(void)
{
	log_info("Initializing input.");

	// Register commands
	input_register("help", "Prints this help", cmd_help);
	input_register("quit", "Exits the application", cmd_quit);
	input_register("version", "Prints the version number", cmd_version);

	// Initialize history
	history = list_create();
}

void
input_process(char *cmd)
{
	char *token;
	input_cmd *cur;
	int32_t i;
	int8_t match;
	size_t len;

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

	// Echo the input
	curses_text(COLOR_HIGH, "> ");
	curses_text(COLOR_NORM, "%s\n", cmd);

	// And put into history
	list_unshift(history, strdup(cmd));

	while (history->count > HISTSIZE)
	{
		list_pop(history);
	}

	position = history->first;

	// Ignore comments
	if (cmd[0] == '#')
	{
		return;
	}

	token = strsep(&cmd, " ");
	match = 0;

    for (i = 0; i < input_cmds->elements; i++)
	{
		cur = darray_get(input_cmds, i);

		if(!strcmp(token, cur->name))
		{
			cur->callback(cmd);
			match++;
		}
	}

	if (!match)
	{
		curses_text(COLOR_NORM, "%s: command not found\n", token);
	}

	// Empty line after each cmd-output
	curses_text(COLOR_NORM, "\n");
}

