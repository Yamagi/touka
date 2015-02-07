/*
 * log.c:
 *  - Logging
 *  - Log rotation
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
#include "utility.h"

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
logtypetostr(logtype type, char *str, size_t len)
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
logger(logtype type, const char *func, int32_t line, const char *fmt, ...)
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
		perror("PANIC: malloc() failed");
		goto error;
	}

    if ((logmsg = malloc(msglen)) == NULL)
	{
		perror("PANIC: malloc() failed");
		goto error;
	}

	// Format the message
	va_start(args, fmt);
	vsnprintf(inpmsg, msglen, fmt, args);
	va_end(args);

	// Time
	tmp = time(NULL);

	if ((t = localtime(&tmp)) == NULL)
	{
		perror("PANIC: Couldn't get local time");
		quit(1);
	}

	strftime(msgtime, sizeof(msgtime), "%m-%d-%Y %H:%M:%S", t);

	// Status
	if (logtypetostr(type, status, sizeof(status)) != 0)
	{
		fprintf(stderr, "PANIC: Unknown logtype\n");
		goto error;
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
		perror("PANIC: Couldn't log error message");
		quit(1);
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

error:
	free(inpmsg);
	free(logmsg);
	quit(1);
}

void
initlog(const char *path, const char *name, int32_t seg)
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
			printf("PANIC: %s is not a directory\n", path);
			quit(1);
		}
	}
	else
	{
		recursive_mkdir(path);
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
			perror("PANIC: Couldn't rotate log files");
			quit(1);
		}
	}

	snprintf(newfile, sizeof(newfile), "%s/%s.00", path, name);
	snprintf(oldfile, sizeof(oldfile), "%s/%s", path, name);

	// Special case: First log file
	if ((stat(oldfile, &sb)) == 0)
	{
		if ((rename(oldfile, newfile)) != 0)
		{
			perror("PANIC: Couldn't rotate log files");
			quit(1);
		}
	}

	if ((logfile = fopen(oldfile, "w")) == NULL)
	{
		perror("PANIC: Couldn't create log file");
		quit(1);
	}
}

void
closelog(void)
{
	if (logfile)
	{
		fflush(logfile);

		if ((fclose(logfile)) != 0)
		{
			perror("PANIC: Couldn't close log file");
			quit(1);
		}

		logfile = NULL;
	}
}

