/*
 * misc.c
 * ------
 *
 * Miscellaneous global functions.
 */

#ifndef MISC_H_
#define MISC_H_

// --------

#include <stdlib.h>

// --------

/*
 * Returns the path to the binary.
 */
char *misc_bindir(void);

/*
 * Create a directory with all intermediate
 * directories as required.
 *
 * dir: Directory to create.
 */
void misc_rmkdir(const char *dir);

/*
 * Secure version of strncat.
 *
 * dst: String to copy data to
 * src: String to copy data from
 * size: Size of destination string
 */
size_t misc_strlcat(char *dst, const char *src, size_t size);


/*
 * Secure version of strncpy.
 *
 * dst: String to copy data to
 * src: String to copy data from
 * size: Size of destination string
 */
size_t misc_strlcpy(char *dst, const char *src, size_t size);

// --------

#endif // MISC_H_

