/*
 * log.h
 * -----
 *
 * This is a small log file handler. Currently only three log
 * levels are supported:
 *  - LOG_INFO:  Normal messages
 *  - LOG_WARN:  Warnings
 *  - LOG_ERROR: Error messages
 *
 *  When the code is build without NDEBUG the function name
 *  and the line number of the caller are printed. Also ERROR
 *  messages are echoed to stderr.
 *
 *  Before the log file handler can be used it must be
 *  initialized by calling initlog(). Initializing the handler
 *  rotates the existing log files. When the program is terminated
 *  the log handler must be closed with closelog().
 */

#ifndef LOG_H_
#define LOG_H_

// --------

#include <stdint.h>

// --------

/*
 * Possible log states
 */
typedef enum
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
} logtype;

// --------

/*
 * Logs an message. If NDEBUG is not set and type is
 * LOG_ERROR, the error message is echoed to stderr.
 * All messages will have the calling function and
 * line added.
 *
 * type: Type if message
 * func: Calling function
 * line: Line of caller
 * fmt:  Print format
 */
void log_insert(logtype type, const char *func, int32_t line, const char *fmt, ...);

/*
 * Initialize the log file. May be called only once.
 *
 * path: Directory with the log files
 * name: Name of the log file
 * seg: Number of segments to keep. Maximum is 99
 */
void log_init(const char *path, const char *name, int16_t seg);

/*
 * Closes the log file. May be called several times.
 */
void log_close(void);

// --------

/*
 * 	Convenience macros for infos
 */
#define log_info(F) log_insert(LOG_INFO, __func__, __LINE__, F)
#define log_info_f(F, ...) log_insert(LOG_INFO, __func__, __LINE__, F, __VA_ARGS__)

/*
 * 	Convenience macro for warnings
 */
#define log_warn(F) log_insert(LOG_WARN, __func__, __LINE__, F)
#define log_warn_f(F, ...) log_insert(LOG_WARN, __func__, __LINE__, F, __VA_ARGS__)

/*
 * 	Convenience macro for errors
 */
#define log_error(F) log_insert(LOG_ERROR, __func__, __LINE__, F)
#define log_error_f(F, ...) log_insert(LOG_ERROR, __func__, __LINE__, F, __VA_ARGS__)

// --------

#endif // LOG_H_
