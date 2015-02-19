/*
 * this is the "heard" of our engine, as it runs
 * the actual game.
 */

#ifndef GAME_H_
#define GAME_H_

#include "data/hashmap.h"
#include "data/list.h"

// --------

/*
 * Header of the game file.
 */
typedef struct
{
	const char *game;
	const char *author;
	const char *date;
	const char *uid;
	const char *start;
} header;

// Parsed game header
extern header *game_header;

/*
 * Represents one room.
 */
typedef struct
{
	const char *name;
	const char *descr;
	list *aliases;
	list *words;
	int8_t mentioned;
	int8_t seen;
} room;

// Rooms
extern hashmap *game_rooms;

/*
 * Represents one scene.
 */
typedef struct
{
	const char *name;
	const char *descr;
	const char *room;
	list *aliases;
	list *words;
	darray *next;
	int8_t visited;
} scene;

extern hashmap *game_scenes;

// --------

/*
 * Initializes the game.
 */
void game_init(const char *file);

/*
 * Shuts the game down.
 */
void game_quit(void);

/*
 * Prints a room description into
 * the text window. If an debug
 * build, it's name and all aliases
 * are printed too.
 */
void game_room_describe(const char *key);

/*
 * Prints a list of all rooms.
 */
void game_rooms_list(void);

/*
 * Advances to the next scene. Either by
 * users choice or to the only one. When
 * the requested choice is not possible,
 * -1 is returned.
 *
 * choice: Option to choose
 */
int32_t game_scene_next(uint32_t choice);

/*
 * Plays the current scene.
 */
void game_scene_play(void);

#endif // GAME_H_

