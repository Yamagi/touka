/*
 * Input processing and sub commands. This code is
 * mostly responsible for calling the approriate
 * functions according to user input.
 */

#ifndef INPUT_H_
#define INPUT_H_

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

#endif // INPUT_H_

