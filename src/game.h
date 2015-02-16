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

header *game_header;

#endif // GAME_H_

