/*
 * This is the game file parser. Calling parser_game()
 * parses the given file and fills the data structures
 * used by game.c. If an error is detected, the parser
 * bails out and prints the number of the broken line.
 */

#ifndef PARSER_H_
#define PARSER_H_

// ---------

/*
 * Parses a game file. Must be called after game_init()
 * since it fills the games internal data structures.
 * The functions checks if the given file exists and
 * bails out if it doesn't.
 *
 * file: File to parse
 */
void parser_game(const char *file);

// ---------

#endif // PARSER_H_

