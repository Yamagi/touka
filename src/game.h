/*
 * game.h
 * ------
 *
 * This is the "heart" of our engine, as it runs
 * the actual game. The upperst level are scenes,
 * which are composed of rooms and a description.
 */

#ifndef GAME_H_
#define GAME_H_

#include "main.h"
#include "data/hashmap.h"
#include "data/list.h"

// --------

/*
 * Header of a game file.
 */
typedef struct
{
	const char *author;
	const char *date;
	const char *first_scene;
	const char *game;
	const char *uid;
} game_header_s;

// Parsed game header
extern game_header_s *game_header;

/*
 * Represents one glossary entry.
 */
typedef struct
{
	boolean mentioned;
	const char *descr;
	const char *name;
	list *aliases;
	list *words;
} game_glossary_s;

// Glossary
extern hashmap *game_glossary;

/*
 * Represents one room.
 */
typedef struct
{
	boolean mentioned;
	boolean visited;
	const char *descr;
	const char *name;
	list *aliases;
	list *words;
} game_room_s;

// Rooms
extern hashmap *game_rooms;

/*
 * Represents one scene.
 */
typedef struct
{
	boolean visited;
	const char *descr;
	const char *name;
	const char *prompt;
	const char *room;
	darray *next;
	list *aliases;
	list *words;
} game_scene_s;

extern hashmap *game_scenes;
extern game_scene_s *current_scene;

/*
 * Statistics.
 */
typedef struct
{
	uint32_t rooms_total;
	uint32_t rooms_visited;
	uint32_t scenes_total;
	uint32_t scenes_visited;
} game_stats_s;

extern game_stats_s *game_stats;

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
 * false is returned.
 *
 * choice: Option to choose
 */
boolean game_scene_next(uint8_t choice);

/*
 * Plays the current scene.
 */
void game_scene_play(const char *key);

/*
 * Prints a list of all scenes.
 */
void game_scene_list(void);

// --------

#endif // GAME_H_

