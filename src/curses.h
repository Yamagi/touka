/*
 * Text User Interface written with ncurses. This is a
 * rather simple approach with only three windows and
 * no global refresh cycle.
 */

/*
 * Highlight state:
 *  - COLOR_NORM: White on black
 *  - COLOR_HIGH: Green on Black
 */
#define COLOR_NORM 0
#define COLOR_HIGH 1

/*
 * Initializes ncurses.
 */
void curses_init(void);

/*
 * Processes user key strokes, combines them
 * into a buffer and sends the buffer up into
 * input frontend. This function blocks, until
 * user input was received.
 *
 * prompt: Prompt to display
 */
void curses_input(const char *prompt);

/*
 * Shuts ncurses down.
 */
void curses_quit(void);

/*
 * Prints a string into the main window.
 *
 * string: Text to print
 */
void curses_text(int highlight, const char *fmt, ...);
