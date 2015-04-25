/*
 * main.h
 * ------
 *
 * Global macros. Either for configuration
 * or implementation.
 */

#ifndef MAIN_H_
#define MAIN_H_

// --------

#include <stdint.h>

// --------

// Applicaction name
#define APPNAME "Touka"

// Program Author
#define AUTHOR "Yamagi Burmeister"

// Game file
#define GAMEFILE ""

// Max. entries in command history
#define HISTSIZE 512

// Home directory
#define HOMEDIR ".touka"

// Number of characters the input line scrolls
#define HSCROLLOFF 5

// Size of input buffer
#define INPUTBUF 512

// Log directory
#define LOGDIR "log"

// Log name
#define LOGNAME "log"

// Number of log segments to keep
#define LOGNUM 15

// Version number
#define VERSION "1.0"

// Number of lines in the scrollback buffer
#define SCROLLBACK 512

// Length of the status line
#define STATUSBAR 128
// Number of lines the text window scrolls
#define VSCROLLOFF 5

// Copyright year
#define YEAR "2015"

// --------

// Bool false
#ifndef FALSE
#define FALSE 0
#endif

// Bool true
#ifndef TRUE
#define TRUE 1
#endif

// Datatype for bool
typedef uint8_t boolean;

// --------

#endif // MAIN_H_
