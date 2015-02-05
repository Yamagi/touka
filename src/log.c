/*
 * log.c:
 *  - Logging
 *  - Log rotation
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "utility.h"

// --------

FILE *logfile;

// --------

void
errmsg(const char *func, int line, const char *msg)
{
	char entry[1024];
	char timestr[32];
	time_t tmp;
	struct tm *t;

	assert(logfile);
	assert(msg);
	assert(strlen(msg) < 896);
	tmp = time(NULL);

 	if ((t = localtime(&tmp)) == NULL)
	{
		perror("PANIC: Couldn't get local time");
		exit(1);
	}

	strftime(timestr, sizeof(timestr), "%m-%d-%Y %H:%M:%S", t);

#ifdef NDEBUG
	snprintf(entry, sizeof(entry), "%s [ERROR]: %s\n", timestr, msg);
#else
	snprintf(entry, sizeof(entry), "%s [ERROR] (%s:%i): %s\n", timestr, func, line, msg);
#endif

	if ((fwrite(entry, strlen(entry), 1, logfile)) != 1)
	{
		perror("PANIC: Couldn't log error message");
		exit(1);
	}

	fflush(logfile);

#ifndef NDEBUG
	fprintf(stderr, "%s\n", msg);
#endif
}

void
infomsg(const char *func, int line, const char *msg)
{
	char entry[1024];
	char timestr[32];
	time_t tmp;
	struct tm *t;

	assert(logfile);
	assert(msg);
	assert(strlen(msg) < 896);
	tmp = time(NULL);

	if ((t = localtime(&tmp)) == NULL)
	{
		perror("PANIC: Couldn't get local time");
		exit(1);
	}

	strftime(timestr, sizeof(timestr), "%m-%d-%Y %H:%M:%S", t);

#ifdef NDEBUG
	snprintf(entry, sizeof(entry), "%s [INFO]: %s\n", timestr, msg);
#else
	snprintf(entry, sizeof(entry), "%s [INFO] (%s:%i): %s\n", timestr, func, line, msg);
#endif

	if ((fwrite(entry, strlen(entry), 1, logfile)) != 1)
	{
		perror("PANIC: Couldn't log info message");
		exit(1);
	}

	fflush(logfile);
}

void
initlog(const char *path, const char *name, int seg)
{
	char newfile[PATH_MAX];
	char oldfile[PATH_MAX];
	int i;
	struct stat sb;

	assert(!logfile);
	assert(seg < 99);

	// Create directory
	if ((stat(path, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			printf("PANIC: %s is not a directory\n", path);
			exit(1);
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
			exit(1);
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
			exit(1);
		}
	}

	if ((logfile = fopen(oldfile, "w")) == NULL)
	{
		perror("PANIC: Couldn't create log file");
		exit(1);
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
			exit(1);
		}

		logfile = NULL;
	}
}

