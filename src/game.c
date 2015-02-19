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

// Scenes
hashmap *game_scenes;

// Current scene
scene *current_scene;

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

	if (r->descr)
	{
		free((char *)r->descr);
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

/*
 * Callback to destroy the scene hashmap.
 *
 * data: Scene to destroy
 */
static void
game_scene_callback(void *data)
{
	scene *s;

	assert(data);

	s = data;

	if (s->name)
	{
		free((char *)s->name);
	}

	if (s->descr)
	{
		free((char *)s->descr);
	}

	if (s->room)
	{
		free((char *)s->room);
	}

    if (s->aliases)
	{
		list_destroy(s->aliases, NULL);
	}

	if (s->words)
	{
		list_destroy(s->words, NULL);
	}

	if (s->next)
	{
		darray_destroy(s->next, NULL);
	}

	free(s);
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

	if (!game_scenes)
	{
		game_scenes = hashmap_create(128);
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
		free((char *)game_header->start);

		free(game_header);
		game_header = NULL;
	}

	if (game_rooms)
	{
		hashmap_destroy(game_rooms, game_room_callback);
	}

	if (game_scenes)
	{
		hashmap_destroy(game_scenes, game_scene_callback);
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

#else

	// Only the debug build shows not visited rooms

	if (r->mentioned && !r->seen)
	{
		curses_text(COLOR_NORM, "Room %s was mentioned but not visited\n", key);

		return;
	}

	if (!r->seen)
	{
		curses_text(COLOR_NORM, "No such room: %s\n", key);

		return;
	}

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

void
game_rooms_list(void)
{
	int32_t i;
	int32_t len;
	list *data;
	listnode *node;
	room *cur;

	data = hashmap_to_list(game_rooms);
	len = 0;

	if (data)
	{
		if (data->first)
		{
			node = data->first;

			while (node)
			{
				cur = node->data;

#ifdef NDEBUG
				if (!cur->seen && !cur->mentioned)
				{
					node = node->next;

					continue;
				}
#endif

				if (strlen(cur->name) > len)
				{
					len = strlen(cur->name);
				}

				node = node->next;
			}
		}
	}

	if (len < strlen("Name"))
	{
		len = strlen("Name");
	}

	curses_text(COLOR_NORM, "%-*s %-*s %s\n", len + 1, "Name", 6, "State", "Description");
	curses_text(COLOR_NORM, "%-*s %-*s %s\n", len + 1, "----", 6, "-----", "-----------");

	for (i = 0; i <= data->count; i++)
	{
		cur = list_shift(data);

#ifdef NDEBUG
		if (!cur->seen && !cur->mentioned)
		{
			continue;
		}
#endif

		curses_text(COLOR_NORM, "%-*s", len + 2, cur->name);

        if (cur->seen)
		{
			curses_text(COLOR_NORM, "%-*s", 7, "S");
		}
		else if (cur->mentioned)
		{
			curses_text(COLOR_NORM, "%-*s", 7, "M");
		}
		else
		{
			curses_text(COLOR_NORM, "%-*s", 7, "-");
		}

		curses_text(COLOR_NORM, "%s\n", cur->descr);
	}

	list_destroy(data, NULL);
}

int32_t
game_scene_next(uint32_t choice)
{
	scene *tmp;

	if (!current_scene)
	{
		if ((current_scene = hashmap_get(game_scenes, game_header->start)) == NULL)
		{
			fprintf(stderr, "PANIC: First scene %s doesn't exists\n", game_header->start);
			quit_error();
		}
	}
	else
	{
		if (choice)
		{
			if (current_scene->next->elements == 1)
			{
				curses_text(COLOR_NORM, "No choice possible");

				return -1;
			}
			if (choice > current_scene->next->elements)
			{
				curses_text(COLOR_NORM, "Invalid choice");

				return -1;
			}

			// TODO: End
			tmp = hashmap_get(game_scenes, darray_get(current_scene->next, choice - 1));

			if (!tmp)
			{
				fprintf(stderr, "PANIC: Scene %s doesn't exists\n", game_header->start);
				quit_error();
			}
			else
			{
				current_scene = tmp;
			}
		}
		else
		{
			if (current_scene->next->elements > 1)
			{
				curses_text(COLOR_NORM, "Make your choice\n");

				return -1;
			}

			// TODO: End
			tmp = hashmap_get(game_scenes, darray_get(current_scene->next, 0));

			if (!tmp)
			{
				fprintf(stderr, "PANIC: Scene %s doesn't exists\n", game_header->start);
				quit_error();
			}
			else
			{
				current_scene = tmp;
			}
		}
	}

	return 0;
}

void
game_scene_play(void)
{
	int32_t i;
	listnode *lnode;

	lnode = current_scene->words->first;

	for (i = 0; i < current_scene->words->count; i++)
	{
		if (!strcmp(lnode->data, "\n") || i == current_scene->words->count - 1)
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

