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

// --------

header *game_header;

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

