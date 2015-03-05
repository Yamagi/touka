/*
 * util.h
 * ------
 *
 * A collection of various more or less
 * usefull utility functions, that don't
 * fit anywhere else.
 */

#ifndef UTIL_H_
#define UTIL_H_

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

#endif // UTIL_H_

