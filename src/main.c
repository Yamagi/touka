/*
 * main.c:
 *  - Application startup
 *  - Application shutdown
 *  - Argument processing
 *  - Main loop
 */

#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "curses.h"
#include "log.h"
#include "main.h"
#include "util.h"

// --------

static void
mainloop(void)
{
	while (1)
	{
		curses_input("Touka: ");
	}
}

// --------

void
signalhandler(int sig)
{
	quit_success();
}

int
main(int argc, char *argv[])
{
	char homedir[PATH_MAX];
	char logbuf[512];
	char logdir[PATH_MAX];
	struct stat sb;

	// Clean aborts
	atexit(quit_success);

	// Signal handler
	signal(SIGINT, signalhandler);
	signal(SIGTERM, signalhandler);

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
		util_rmkdir(homedir);
	}

	// Bring up logging
	log_init(logdir, LOGNAME, LOGNUM);

	snprintf(logbuf, sizeof(logbuf), "This it %s %s.", APPNAME, VERSION);
	log_info_f("This is %s %s, (c) %s %s", APPNAME, VERSION, YEAR, AUTHOR);
	log_info_f("This binary was build on %s.", __DATE__);

	// Initialize TUI
	curses_init();
	curses_status("Cool status message :)");

	// Loop forever
	mainloop();

	// Terminate
    quit_success();

	// Never reached
	return 0;
}

