/*
 * quit.c:
 * -------
 *
 * Application shutdown.
 */

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "curses.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "save.h"
#include "quit.h"

#include "i18n/i18n.h"

// --------

/*********************************************************************
 *                                                                   *
 *                          Support Functions                        *
 *                                                                   *
 *********************************************************************/

/*
 * Converts an error code into a string.
 *
 * error: Error code to convert
 */
const char*
quit_errcodetostr(errcode error)
{
	switch (error)
	{
		case PBROKENSAVE:
			return i18n_error_brokensave;
			break;

		case PCOULDNTCLOSEFILE:
			return i18n_error_couldntclosefile;
			break;

		case PCOULDNTCREATEDIR:
			return i18n_error_couldntcreatedir;
			break;

		case PCOULDNTLOADHISTORY:
			return i18n_error_couldntloadhistory;
			break;

		case PCOULDNTLOADSAVE:
			return i18n_error_couldloadsave;
			break;

		case PCOULDNTOPENDIR:
			return i18n_error_couldntopendir;
			break;

		case PCOULDNTOPENFILE:
			return i18n_error_couldntopenfile;
			break;

		case PCOULDNTROTATELOGS:
			return i18n_error_couldntrotatelogs;
			break;

		case PCOULDNTSAVEHISTORY:
			return i18n_error_couldntsavehistory;
			break;

		case PCOULDNTWRITELOGMSG:
			return i18n_error_couldntwritelogmsg;
			break;

		case PFILENOTEXIST:
			return i18n_error_filenotexist;
			break;

		case PFIRSTSCENENOTFOUND:
			return i18n_error_firstscenenotfound;
			break;

		case PINVGAMEHEADER:
			return i18n_error_invgameheader;
			break;

		case PLOCALTIME:
			return i18n_error_localtime;
			break;

		case PNOTADIR:
			return i18n_error_notadir;
			break;

		case PNOTAFILE:
			return i18n_error_notafile;
			break;

		case POUTOFMEM:
			return i18n_error_outofmem;
			break;

		case PPARSERERR:
			return i18n_error_parsererr;
			break;

		case PROOMNOTFOUND:
			return i18n_error_roomnotfound;
			break;

		case PSCENENOTFOUND:
			return i18n_error_scenenotfound;
			break;

		case PUNKNOWNLOGTYPE:
			return i18n_error_unkownlogtype;
			break;

		default:
			return NULL;
			break;
	}
}

// --------

/*********************************************************************
 *                                                                   *
 *                           Signal Handlers                         *
 *                                                                   *
 *********************************************************************/

/*
 * The game crashed.
 *
 * sig: The signal
 */
void
quit_signal_error(int32_t sig)
{
	save_write("panic");
	curses_quit();

	fprintf(stderr, "PANIC: Crash\n");
	fflush(stderr);

	signal(SIGSEGV, SIG_DFL);
	signal(SIGILL, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGABRT, SIG_DFL);

    raise(sig);
}

/*
 * The game received a deadly signal.
 *
 * sig: The signal
 */
void
quit_signal_success(int32_t sig)
{
	log_info_f("Received signal %i", sig);

	quit_success();
}

void
quit_signal_register(void)
{
	/* Crash */
	signal(SIGSEGV, quit_signal_error);
	signal(SIGILL, quit_signal_error);
	signal(SIGFPE, quit_signal_error);
	signal(SIGABRT, quit_signal_error);

	/* User abort */
	signal(SIGINT, quit_signal_success);
	signal(SIGTERM, quit_signal_success);
}

// --------

/*********************************************************************
 *                                                                   *
 *                         Shutdown Functions                        *
 *                                                                   *
 *********************************************************************/

void
quit_error(errcode error)
{
	const char *status;
	int32_t err;
	static boolean recursive;

	err = errno;
	recursive = FALSE;

	if (recursive)
	{
		_exit(1);
	}
	else
	{
		recursive = TRUE;
	}

	status = quit_errcodetostr(error);
	save_write("panic");
	curses_quit();

	if (err)
	{
		fprintf(stderr, "PANIC: %s (%s)\n", status, strerror(err));
	}
	else
	{
		fprintf(stderr, "PANIC: %s\n", status);
	}

	fflush(stderr);

	_exit(1);
}

void
quit_success(void)
{
	static boolean recursive;

	if (recursive)
	{
		log_warn("Recursive shutdown detected.");
		_exit(1);
	}
	else
	{
		recursive = TRUE;
	}

	save_write("shutdown");
	game_quit();
	curses_quit();
	input_quit();
	log_close();

	_exit(0);
}
