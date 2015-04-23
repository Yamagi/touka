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

#include "misc.h"
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
	misc_strlcpy(path, tmp, len);

	return path;
}

void
misc_rmkdir(const char *dir)
{
	char *p;
	char tmp[PATH_MAX];
	size_t len;

	misc_strlcpy(tmp, dir, sizeof(tmp));
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

size_t
misc_strlcat(char *dst, const char *src, size_t size)
{
	char *d;
	const char *s;
	size_t n;
	size_t dlen;

	d = dst;
	n = size;
	s = src;

	while (n-- != 0 && *d != '\0')
	{
		d++;
	}

	dlen = d - dst;
	n = size - dlen;

	if (n == 0)
	{
		return dlen + strlen(s);
	}

	while (*s != '\0')
	{
		if (n != 1)
		{
			*d++ = *s;
			n--;
		}

		s++;
	}

	*d = '\0';

	return dlen + (s - src);
}

size_t
misc_strlcpy(char *dst, const char *src, size_t size)
{
	char *d;
	const char *s;
	size_t n;

	d = dst;
	n = size;
	s = src;

	if (n != 0)
	{
		while (--n != 0)
		{
			if ((*d++ = *s++) == '\0')
			{
				break;
			}
		}
	}

	if (n == 0)
	{
		if (size != 0)
		{
			*d = '\0';
		}

		while (*s++)
		{
		}
	}

	return s - src - 1;
}
