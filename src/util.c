/*
 * utility.c:
 * ----------
 *
 * Various stuff like global utility functions
 * or the shutdown code.
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "curses.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "save.h"
#include "util.h"

// --------

void
util_rmkdir(const char *dir)
{
	char *p;
	char tmp[PATH_MAX];
	size_t len;

	log_info_f("Creating path %s", dir);
	stpncpy(tmp, dir, sizeof(tmp));
	len = strlen(tmp);

	if (tmp[len - 1] == '/')
	{
		tmp[len - 1] = 0;
	}

	for (p = tmp + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;

			if ((mkdir(tmp, 0700)) != 0)
			{
				if (errno != EEXIST)
				{
					quit_error("Couldn't create directory");
				}
			}

			*p = '/';
		}
	}

	if ((mkdir(tmp, 0700)) != 0)
	{
		if (errno != EEXIST)
		{
			quit_error("Couldn't create directory");
		}
	}
}

// --------

void
quit_error(const char *msg)
{
	int32_t err = errno;

	// Shutdown TUI
	curses_quit();

	// Save game
	save_write("shutdown");

	if (err)
	{
		fprintf(stderr, "PANIC: %s (%s)\n", msg, strerror(err));
	}
	else
	{
		fprintf(stderr, "PANIC: %s\n", msg);
	}

	_exit(1);
}

void
quit_success(void)
{
	static int32_t recursive;

	if (recursive)
	{
		log_warn("Recursive shutdown detected.");
		_exit(1);
	}
	else
	{
		recursive++;
	}

	// Save game
	save_write("shutdown");

	// Shutdown game
	game_quit();

	// Shutdown TUI
	curses_quit();

	// Shutdown input subsystem
	input_quit();

	// Close log handlers
	log_close();

	_exit(0);
}

