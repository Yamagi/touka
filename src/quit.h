/*
 * quit.h
 * ------
 *
 * Application shutdown.
 */

#ifndef QUIT_H_
#define QUIT_H_

// ---------

// Error codes.
typedef enum
{
	PBROKENSAVE,
	PCOULDNTCLOSEFILE,
	PCOULDNTCREATEDIR,
	PCOULDNTLOADHISTORY,
	PCOULDNTLOADSAVE,
	PCOULDNTOPENDIR,
	PCOULDNTOPENFILE,
	PCOULDNTROTATELOGS,
	PCOULDNTSAVEHISTORY,
	PCOULDNTWRITELOGMSG,
	PFILENOTEXIST,
	PFIRSTSCENENOTFOUND,
	PINVGAMEHEADER,
	PLOCALTIME,
	PNOTADIR,
	PNOTAFILE,
	POUTOFMEM,
	PPARSERERR,
	PROOMNOTFOUND,
	PSCENENOTFOUND,
	PUNKNOWNLOGTYPE,
} errcode;

// ---------

/*
 * Registers the signal handlers.
 */
void quit_signal_register(void);

// ---------

/*
 * Shuts the application down and returns 1.
 *
 * error: Error code
 */
void quit_error(errcode error);

/*
 * Shuts the application down and returns 0.
 */
void quit_success(void);

// --------

#endif // QUIT_H_
