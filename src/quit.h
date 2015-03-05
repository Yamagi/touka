/*
 * quit.h
 * ------
 *
 * Application shutdown.
 */

#ifndef QUIT_H_
#define QUIT_H_

// ---------

/*
 * Shuts the application down and returns 1.
 */
void quit_error(const char *msg);

/*
 * Shuts the application down and returns 0.
 */
void quit_success(void);

// --------

#endif // QUIT_H_

