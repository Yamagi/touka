/*
 * A collection of various more or less
 * usefull utility functions, that don't
 * fit anywhere else.
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

/*
 * Create a directory with all intermediate
 * directories as required.
 *
 * dir: Directory to create.
 */
void util_rmkdir(const char *dir);

// ----

/*
 * Shuts the application down and returns 1.
 */
void quit_error(void);

/*
 * Shuts the application down and returns 0.
 */
void quit_success(void);

#endif // UTIL_H_

