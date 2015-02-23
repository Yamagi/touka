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
#include "util.h"

// --------

static FILE *logfile;

// --------

/*
 * Converts a logtype to a human
 * readable string.
 *
 * type: Logtype to convert
 * str: String where the string is copied
 * len: Length of string
 */
static int32_t
log_typetostr(logtype type, char *str, size_t len)
{
	int8_t ret = 0;

	switch (type)
	{
		case LOG_INFO:
			strncpy(str, "INFO", len);
			break;

		case LOG_WARN:
			strncpy(str, "WARN", len);
			break;

		case LOG_ERROR:
			strncpy(str, "ERROR", len);
			break;

		default:
			strncpy(str, "UNKNOWN", len);
			ret = -1;
			break;
	}

	return ret;
}

// --------

void
log_insert(logtype type, const char *func, int32_t line, const char *fmt, ...)
{
	char *inpmsg = NULL;
	char *logmsg = NULL;
	char msgtime[32];
	char status[32];
	size_t msglen;
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
		quit_error("Couldn't allocate memory");
	}

	// Format the message
	va_start(args, fmt);
	vsnprintf(inpmsg, msglen, fmt, args);
	va_end(args);

	// Time
	tmp = time(NULL);

	if ((t = localtime(&tmp)) == NULL)
	{
		quit_error("Couldn't get local time");
	}

	strftime(msgtime, sizeof(msgtime), "%m-%d-%Y %H:%M:%S", t);

	// Status
	if (log_typetostr(type, status, sizeof(status)) != 0)
	{
		quit_error("Unknown logtype\n");
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
		quit_error("Couldn't log message");
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

	return;
}

void
log_init(const char *path, const char *name, int32_t seg)
{
	char newfile[PATH_MAX];
	char oldfile[PATH_MAX];
	int32_t i;
	struct stat sb;

	assert(!logfile);
	assert(seg < 99);

	// Create directory
	if ((stat(path, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			quit_error("Not not a directory\n");
		}
	}
	else
	{
		util_rmkdir(path);
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
			quit_error("Couldn't rotate log files");
		}
	}

	snprintf(newfile, sizeof(newfile), "%s/%s.00", path, name);
	snprintf(oldfile, sizeof(oldfile), "%s/%s", path, name);

	// Special case: First log file
	if ((stat(oldfile, &sb)) == 0)
	{
		if ((rename(oldfile, newfile)) != 0)
		{
			quit_error("Couldn't rotate log files");
		}
	}

	if ((logfile = fopen(oldfile, "w")) == NULL)
	{
		quit_error("Couldn't create log file");
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
			quit_error("Couldn't close log file");
		}

		logfile = NULL;
	}
}

