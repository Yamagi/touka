/*
 * This is a small log file handler. Currently only
 * two log levels are supported:
 *  - INFO:  Normal messages
 *  - ERROR: Error messages
 *
 *  When the code is build without NDEBUG the function name
 *  and the line number of the caller are printed. Also ERROR
 *  messages are echoed to stderr.
 *
 *  Before the log file handler can be used it must be
 *  initialized with initlog(). Initializing rotates the
 *  existing log files. When the program is terminated
 *  the log handler must be closed with closelog().
 */

/*
 * Logs an error message. If NDEBUG is not set,
 * the error message is echoed to stderr and the
 * name and line number of the caller are added.
 *
 * func: Name of the calling function
 * line: Line number of the calling statement
 * msg:  Message to print. 896 characters maximum
 */
void errmsg(const char *func, int line, const char *msg);

/*
 * Logs an info message. If NDEBUG is not set, the
 * error message is echoed to stderr and the name
 * and line number of the caller are added.
 *
 * func: Name of the calling function
 * line: Line number of the calling statement
 * msg:  Message to print. 896 characters maximum
 */
void infomsg(const char *func, int line, const char *msg);

/*
 * Initialize the log file. May be called only once.
 *
 * path: Directory with the log files
 * name: Name of the log file
 * seg: Number of segments to keep. Maximum is 99
 */
void initlog(const char *path, const char *name, int seg);

/*
 * Closes the log file. May be called several times.
 */
void closelog(void);

/*
 * 	Convenience macro to errmsg()
 */
#define log_err(M) errmsg(__func__, __LINE__, M)

/*
 * 	Convenience macro to infomsg()
 */
#define log_info(M) infomsg(__func__, __LINE__, M)

