/*
 * misc.c
 * ------
 *
 * Miscellaneous global functions.
 */

#ifndef MISC_H_
#define MISC_H_

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

// --------

#endif // MISC_H_

