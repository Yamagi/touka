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
#include "util.h"
#include "data/darray.h"

// ---------

// Represents one input command
typedef struct input_cmd
{
	const char *name;
	const char *help;
	void (*callback)(char *msg);
} input_cmd;

// Holds all input command
darray *input_cmds;

// ---------

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

static void
input_register(const char *name, const char *help, void (*callback)(char *msg))
{
	input_cmd *new;

    assert(name);
	assert(help);
	assert(callback);

	if (!input_cmds)
	{
		input_cmds = darray_create(sizeof(input_cmd));
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
}

// ---------

void
input_init(void)
{
	input_register("quit", "Exits the application", cmd_quit);
	input_register("version", "Prints the version number", cmd_version);
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

