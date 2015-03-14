/*
 * main.c
 * ------
 *
 * Application startup and the main loop.
 */

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
#include "main.h"
#include "misc.h"
#include "quit.h"
#include "save.h"

#include "i18n/i18n.h"

// --------

/*********************************************************************
 *                                                                   *
 *                            Main Loop                              *
 *                                                                   *
 *********************************************************************/

int
main(int argc, char *argv[])
{
	char *exepath;
	char gamefile[PATH_MAX];
	char homedir[PATH_MAX];
	char logbuf[512];
	char logdir[PATH_MAX];
	char *tmp;
	struct stat sb;

	// Clean aborts
	atexit(quit_success);

	// Signal handler
	quit_signal_register();

	// Create home directory
	snprintf(homedir, sizeof(homedir), "%s/%s", getenv("HOME"), HOMEDIR);
	snprintf(logdir, sizeof(logdir), "%s/%s", homedir, LOGDIR);

	if ((stat(homedir, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			printf("PANIC: %s is not a directory\n", homedir);
			exit(1);
		}
	}
	else
	{
		misc_rmkdir(homedir);
	}

	// Bring up logging
	log_init(logdir, LOGNAME, LOGNUM);

	snprintf(logbuf, sizeof(logbuf), "This it %s %s.", APPNAME, VERSION);
	log_info_f("%s %s %s, (c) %s %s", i18n_version_thisis, APPNAME, VERSION, YEAR, AUTHOR);
	log_info_f("%s %s.", i18n_version_buildon, __DATE__);

	// Path to the gamefile
	memset(gamefile, 0, sizeof(gamefile));

	if (strlen(GAMEFILE))
	{
		if (GAMEFILE[0] == '/')
		{
			stpncpy(gamefile, GAMEFILE, sizeof(gamefile));
		}
		else
		{
			exepath = misc_bindir();

			stpncpy(gamefile, exepath, sizeof(gamefile));
			strncat(gamefile, "/", sizeof(gamefile) - 1);
			strncat(gamefile, GAMEFILE, sizeof(gamefile) - 1);

			tmp = realpath(gamefile, NULL);

			stpncpy(gamefile, tmp, sizeof(gamefile));
		}
	}
	else
	{
		if (argc != 2)
		{
			fprintf(stderr, "USAGE: %s /path/to/game\n", argv[0]);
			exit(1);
		}
		else
		{
			stpncpy(gamefile, argv[1], sizeof(gamefile));
		}
	}

	// Load the game
	game_init(gamefile);

	// Initialize savegames
	save_init(homedir);

	// Initialize TUI
	curses_init();

	// Initialize input
	input_init(homedir);

	// Show startscreen
	game_scene_play(NULL);

	// Mainloop
	while (TRUE)
	{
		curses_input();
	}

	// Terminate
    quit_success();

	// Never reached
	return 0;
}

