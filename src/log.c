/*
 * log.c
 * -----
 *
 * A simple logger. It supports three log levels
 * (INFO, WARN, ERROR), more can be added easily.
 * Log files are rotated.
 */

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "main.h"
#include "misc.h"
#include "quit.h"

// --------

static FILE *logfile;

// --------

/*********************************************************************
 *                                                                   *
 *                            Support Functions                      *
 *                                                                   *
 *********************************************************************/

/*
 * Converts a logtype to a human readable
 * string.
 *
 * type: Logtype to convert
 * str: String to which the return value is copied
 * len: Length of the return string
 */
static boolean
log_typetostr(logtype type, char *str, size_t len)
{
	boolean ret;

    ret = TRUE;

	switch (type)
	{
		case LOG_INFO:
			misc_strlcpy(str, "INFO", len);
			break;

		case LOG_WARN:
			misc_strlcpy(str, "WARN", len);
			break;

		case LOG_ERROR:
			misc_strlcpy(str, "ERROR", len);
			break;

		default:
			misc_strlcpy(str, "UNKNOWN", len);
			ret = FALSE;
			break;
	}

	return ret;
}

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

void
log_insert(logtype type, const char *func, int32_t line, const char *fmt, ...)
{
	char *inpmsg;
	char *logmsg;
	char msgtime[32];
	char status[32];
	int msglen;
	struct tm *t;
	time_t tmp;
	va_list args;

	// 256 is enough room for the prepended stuff
	va_start(args, fmt);
	msglen = vsnprintf(NULL, 0, fmt, args) + 256;
	va_end(args);

    if ((inpmsg = malloc(msglen)) == NULL)
	{
		perror("Couldn't allocate memory");
	}

    if ((logmsg = malloc(msglen)) == NULL)
	{
		quit_error(POUTOFMEM);
	}

	// Format the message
	va_start(args, fmt);
	vsnprintf(inpmsg, msglen, fmt, args);
	va_end(args);

	// Time
	tmp = time(NULL);

	if ((t = localtime(&tmp)) == NULL)
	{
		quit_error(PLOCALTIME);
	}

	strftime(msgtime, sizeof(msgtime), "%m-%d-%Y %H:%M:%S", t);

	// Status
	if (!log_typetostr(type, status, sizeof(status)) != 0)
	{
		quit_error(PUNKNOWNLOGTYPE);
	}

	// Prepend informational stuff
#ifdef NDEBUG
	snprintf(logmsg, msglen, "%s [%s]: %s\n", msgtime, status, inpmsg);
#else
	snprintf(logmsg, msglen, "%s [%s] (%s:%i): %s\n", msgtime, status, func, line, inpmsg);
#endif

	// Write it
	if ((fwrite(logmsg, strlen(logmsg), 1, logfile)) != 1)
	{
		quit_error(PCOULDNTWRITELOGMSG);
	}

	fflush(logfile);

#ifndef NDEBUG
	if (type == LOG_ERROR)
	{
		fprintf(stderr, "%s", logmsg);
	}
#endif

	free(inpmsg);
	free(logmsg);
}

void
log_init(const char *path, const char *name, int16_t seg)
{
	char newfile[PATH_MAX];
	char oldfile[PATH_MAX];
	int16_t i;
	struct stat sb;

	assert(!logfile);
	assert(seg < 99);

	// Create directory
	if ((stat(path, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			quit_error(PNOTADIR);
		}
	}
	else
	{
		misc_rmkdir(path);
	}

    // Rotate logs
	for (i = seg; i >= 0; i--)
	{
		snprintf(newfile, sizeof(newfile), "%s/%s.%02i", path, name, i + 1);
		snprintf(oldfile, sizeof(oldfile), "%s/%s.%02i", path, name, i);

		// Doesn't exists
		if ((stat(oldfile, &sb)) != 0)
		{
			continue;
		}

		// Delete the oldest file
		if (i == seg)
		{
			unlink(oldfile);
			continue;
		}

		if ((rename(oldfile, newfile)) != 0)
		{
			quit_error(PCOULDNTROTATELOGS);
		}
	}

	snprintf(newfile, sizeof(newfile), "%s/%s.00", path, name);
	snprintf(oldfile, sizeof(oldfile), "%s/%s", path, name);

	// Special case: First log file
	if ((stat(oldfile, &sb)) == 0)
	{
		if ((rename(oldfile, newfile)) != 0)
		{
			quit_error(PCOULDNTROTATELOGS);
		}
	}

	if ((logfile = fopen(oldfile, "w")) == NULL)
	{
		quit_error(PCOULDNTOPENFILE);
	}
}

void
log_close(void)
{
	if (logfile)
	{
		fflush(logfile);

		if ((fclose(logfile)) != 0)
		{
			quit_error(PCOULDNTCLOSEFILE);
		}

		logfile = NULL;
	}
}
