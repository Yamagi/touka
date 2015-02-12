/*
 * Input processing and sub commands. This code is
 * mostly responsible for calling the approriate
 * functions according to user input.
 *
 * Additionally a simple(!) history and simple tab
 * completions are provided. History and completions
 * are mutual exklusiv, starting one resets the other.
 */

#ifndef INPUT_H_
#define INPUT_H_

/*
 * Returns the next string in the history.
 * If no string is found NULL is returned.
 */
char *input_history_next(void);

/*
 * Returns the previous string in the history.
 * Is no string is found NULL is returned.
 */
char *input_history_prev(void);

/*
 * Resets the history state.
 */
void input_history_reset(void);

/*
 * Returns all matching completions for
 * the input string.
 *
 * msg: What the user has typed so far
 */
char *input_complete(char *msg);

/*
 * Resets the completion state.
 */
void input_complete_reset(void);

/*
 * Initializes the input subsystem.
 * The main purpose of this function
 * is to register our input cmds.
 */
void input_init(void);

/*
 * Process user input.
 *
 * input: User supplied string
 */
void input_process(char *input);

/*
 * Shuts the input subsystem down.
 */
void input_quit(void);

#endif // INPUT_H_

