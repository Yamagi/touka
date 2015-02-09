/*
 * utility.c:
 *  - Various utility functions
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
#include "log.h"
#include "util.h"

// --------

void
util_rmkdir(const char *dir)
{
	char *p;
	char tmp[PATH_MAX];
	size_t len;

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
					perror("PANIC: Couldn't create directory");
					exit(1);
				}
			}

			*p = '/';
		}
	}

	if ((mkdir(tmp, 0700)) != 0)
	{
		if (errno != EEXIST)
		{
			perror("PANIC: Couldn't create directory");
			exit(1);
		}
	}
}

void
quit(int32_t ret)
{
	static int32_t recursive = 0;

	if (recursive)
	{
		printf("Recursive!\n");
		exit(1);
	}
	else
	{
		recursive++;
	}

	curses_quit();
	log_close();

	_exit(0);
}

