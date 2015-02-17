/*
 * game.c:
 *  - Game Initialization
 *  - Game logic
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "parser.h"
#include "util.h"
#include "data/hashmap.h"
#include "data/list.h"

// --------

// Game header
header *game_header;

// Rooms
hashmap *game_rooms;

// --------

/*
 * Callback to destroy the room hashmap.
 *
 * data: Room to destroy
 */
static void
game_room_callback(void *data)
{
	room *r;

	assert(data);

    r = data;

	if (r->name)
	{
		free((char *)r->name);
	}

	if (r->aliases)
	{
		list_destroy(r->aliases, NULL);
	}

	if (r->words)
	{
		list_destroy(r->words, NULL);
	}

	free(r);
}

// --------

void
game_init(const char *file)
{
	assert(file);

	if (!game_header)
	{
		if ((game_header = calloc(1, sizeof(header))) == NULL)
		{
			perror("PANIC: Couldn't allocate memory");
			quit_error();
		}
	}

	if (!game_rooms)
	{
		game_rooms = hashmap_create(128);
	}

	parser_game(file);
}

void
game_quit(void)
{
	if (game_header)
	{
		free((char *)game_header->game);
		free((char *)game_header->author);
		free((char *)game_header->date);
		free((char *)game_header->uid);

		free(game_header);
		game_header = NULL;
	}

	if (game_rooms)
	{
		hashmap_destroy(game_rooms, game_room_callback);
	}
}

