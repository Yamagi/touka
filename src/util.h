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

/*
 * Terminates the program and returns
 * the status code 'ret'.
 *
 * ret: Status code to return
 */
void quit(int32_t ret);

#endif // UTIL_H_

