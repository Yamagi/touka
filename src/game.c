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
}
