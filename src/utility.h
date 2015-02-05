/*
 * A collection of various more or less
 * usefull utility functions, that don't
 * fit anywhere else.
 */

/*
 * Create a directory with all intermediate
 * directories as required.
 *
 * dir: Directory to create.
 */
void recursive_mkdir(const char *dir);

/*
 * Terminates the program and returns
 * the status code 'ret'.
 *
 * ret: Status code to return
 */
void quit(int ret);
