/*
 * misc.c
 * ------
 *
 * Miscellaneous global functions.
 */

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#include "log.h"
#include "quit.h"

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

char
*misc_bindir(void)
{
	char *path;
	char *tmp;
	size_t len;

	len = sizeof(char) * PATH_MAX;

	if ((path = malloc(len)) == NULL)
	{
		quit_error(POUTOFMEM);
	}

#ifdef FreeBSD
	int32_t mib[4];

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;

	sysctl(mib, 4, path, &len, NULL, 0);
#elif __linux__
	readlink("/proc/self/exe", path, len);
#endif

	tmp = dirname(path);
	stpncpy(path, tmp, len);

	return path;
}

void
misc_rmkdir(const char *dir)
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
					quit_error(PCOULDNTCREATEDIR);
				}
			}

			*p = '/';
		}
	}

	if ((mkdir(tmp, 0700)) != 0)
	{
		if (errno != EEXIST)
		{
			quit_error(PCOULDNTCREATEDIR);
		}
	}
}

