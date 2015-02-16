/*
 * this is the "heard" of our engine, as it runs
 * the actual game.
 */

#ifndef GAME_H_
#define GAME_H_

#include "data/hashmap.h"
#include "data/list.h"

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
extern header *game_header;

/*
 * Represents one room.
 */
typedef struct
{
	const char *name;
	list *aliases;
	list *words;
} room;

// Rooms
extern hashmap *game_rooms;

// --------

/*
 * Initializes the game.
 */
void game_init(const char *file);

/*
 * Shuts the game down.
 */
void game_quit(void);

#endif // GAME_H_

