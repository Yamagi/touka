/*
 * main.c:
 *  - Application startup
 *  - Application shutdown
 *  - Argument processing
 *  - Main loop
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "log.h"
#include "touka.h"
#include "utility.h"

int
main(int argc, char *argv[])
{
	char homedir[PATH_MAX];
	char logbuf[512];
	char logdir[PATH_MAX];
	struct stat sb;

	snprintf(homedir, sizeof(homedir), "%s/%s", getenv("HOME"), HOMEDIR);
	snprintf(logdir, sizeof(logdir), "%s/%s", homedir, LOGDIR);

	// Create home directory
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
		recursive_mkdir(homedir);
	}

	// Bring up logging
	initlog(logdir, LOGNAME, LOGNUM);

	snprintf(logbuf, sizeof(logbuf), "This it %s %s.", APPNAME, VERSION);
	log_info("This is %s %s, (c) %s %s", APPNAME, VERSION, YEAR, AUTHOR);
	log_info("This binary was build on %s.", __DATE__);

	// Terminate
    quit(0);

	// Never reached
	return 0;
}

