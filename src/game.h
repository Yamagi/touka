/*
 * this is the "heard" of our engine, as it runs
 * the actual game.
 */

#ifndef GAME_H_
#define GAME_H_

/*
 * Header of the game file.
 */
typedef struct
{
	const char *game;
	const char *author;
	const char *date;
	const char *uid;
} header;

// Parsed game header
header *game_header;

#endif // GAME_H_

