/*
 * input.c
 *  - Input processing
 */

#include <string.h>

#include "curses.h"
#include "main.h"
#include "util.h"

// ---------

static void
quit_cmd(void)
{
	quit_success();
}

static void
version_cmd(void)
{
	curses_text(COLOR_NORM, "This is %s %s, (c) %s %s\n", APPNAME, VERSION, YEAR, AUTHOR);
	curses_text(COLOR_NORM, "This binary was build on %s.\n", __DATE__);
}

// ---------

void
process_input(char *cmd)
{
	char *token;
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

	if (!strcmp(token, "quit"))
	{
		quit_cmd();
	}
	else if (!strcmp(token, "version"))
	{
		version_cmd();
	}
	else
	{
		curses_text(COLOR_NORM, "%s: command not found\n", token);
	}

	// Empty line after each cmd-output
	curses_text(COLOR_NORM, "\n");
}

