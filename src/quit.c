/*
 * quit.c:
 * -------
 *
 * Application shutdown.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "curses.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "save.h"
#include "quit.h"

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

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

