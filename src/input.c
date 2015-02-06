/*
 * input.c
 *  - Input processing
 */

#include <string.h>

#include "curses.h"
#include "main.h"

// ---------

static void
quit_cmd(void)
{
	break_mainloop = 1;
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
	else
	{
		curses_text(COLOR_NORM, "%s: command not found\n\n", token);
	}
}

