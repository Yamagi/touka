/*
 * misc.c
 * ------
 *
 * Miscellaneous global functions.
 */

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "quit.h"

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

void
misc_rmkdir(const char *dir)
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

