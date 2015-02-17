/*
 * game.c:
 *  - Game Initialization
 *  - Game logic
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "curses.h"
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

void
game_room_describe(const char *key)
{
	int32_t i;
	listnode *lnode;
	room *r;

	assert(key);

	if ((r = hashmap_get(game_rooms, key)) == NULL)
	{
		// Room doesn't exists
		curses_text(COLOR_NORM, "No such room: %s\n", key);
		return;
	}

#ifndef NDEBUG
	/* When an debug build, print the room name
	   and all it's aliases above the description. */

	curses_text(COLOR_HIGH, "%s", r->name);

	if (r->aliases)
	{
		if (r->aliases->first)
		{
			lnode = r->aliases->first;
		}

		curses_text(COLOR_NORM, " (");

		for (i = 0; i < r->aliases->count; i++)
		{
			if (i)
			{
				curses_text(COLOR_NORM, ", ");
			}

			curses_text(COLOR_HIGH, lnode->data);
			lnode = lnode->next;
		}

		curses_text(COLOR_NORM, ")");
	}

	curses_text(COLOR_NORM, ":\n");

#endif // NDEBUG

	// Print description
	lnode = r->words->first;

	for (i = 0; i < r->words->count; i++)
	{
		if (!strcmp(lnode->data, "\n") || i == r->words->count - 1)
		{
			curses_text(COLOR_NORM, lnode->data);
		}
		else
		{
			curses_text(COLOR_NORM, "%s ", lnode->data);
		}

		lnode = lnode->next;
	}

	curses_text(COLOR_NORM, "\n");

}
