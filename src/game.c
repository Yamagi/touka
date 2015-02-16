/*
 * game.c:
 *  - Game Initialization
 *  - Game logic
 */

#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "util.h"

// --------

void
game_init(void)
{
	if (!game_header)
	{
		if ((game_header = calloc(1, sizeof(game_header))) == NULL)
		{
			perror("PANIC: Couldn't allocate memory");
			quit_error();
		}
	}
}

