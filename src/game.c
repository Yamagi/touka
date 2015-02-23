/*
 * game.c
 * ------
 *
 * This file runs the game. The upperst level
 * is a scene, which is composed of a room and
 * a description.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "curses.h"
#include "game.h"
#include "log.h"
#include "main.h"
#include "parser.h"
#include "util.h"
#include "data/hashmap.h"
#include "data/list.h"

// --------

// Game header
game_header_s *game_header;

// Rooms
hashmap *game_rooms;

// Scenes
hashmap *game_scenes;

// Current scene
game_scene_s *current_scene;

// Has the game ended?
static boolean game_end;

// --------

/*
 * Callback destroy a room.
 *
 * data: Room to destroy
 */
static void
game_room_destroy_callback(void *data)
{
	game_room_s *room;

	assert(data);

    room = data;

	if (room->name)
	{
		free((char *)room->name);
	}

	if (room->descr)
	{
		free((char *)room->descr);
	}

	if (room->aliases)
	{
		list_destroy(room->aliases, NULL);
	}

	if (room->words)
	{
		list_destroy(room->words, NULL);
	}

	free(room);
}

void
game_room_describe(const char *key)
{
	uint16_t i;
	listnode *lnode;
	game_room_s *room;

	assert(key);

	if ((room = hashmap_get(game_rooms, key)) == NULL)
	{
		// Room doesn't exists
		curses_text(COLOR_NORM, "No such room: %s\n", key);

		return;
	}

#ifndef NDEBUG
	/* When an debug build, print the room name
	   and all it's aliases above the description. */

	curses_text(COLOR_HIGH, "%s", room->name);

	if (room->aliases)
	{
		if (room->aliases->first)
		{
			lnode = room->aliases->first;
		}

		curses_text(COLOR_NORM, " (");

		for (i = 0; i < room->aliases->count; i++)
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

	if (room->mentioned && !room->visited)
	{
		curses_text(COLOR_NORM, "Room %s was mentioned but not visited\n", key);

		return;
	}

	if (!room->visited)
	{
		curses_text(COLOR_NORM, "No such room: %s\n", key);

		return;
	}

#endif // NDEBUG

	// Print description
	lnode = room->words->first;

	for (i = 0; i < room->words->count; i++)
	{
		if (!strcmp(lnode->data, "\n") || i == room->words->count - 1)
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
	uint16_t i;
	size_t len;
	list *data;
	listnode *node;
	game_room_s *room;

	data = hashmap_to_list(game_rooms);
	len = 0;

	if (data)
	{
		if (data->first)
		{
			node = data->first;

			while (node)
			{
				room = node->data;

#ifdef NDEBUG
				if (!room->visited && !room->mentioned)
				{
					node = node->next;

					continue;
				}
#endif

				if (strlen(room->name) > len)
				{
					len = strlen(room->name);
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
		room = list_shift(data);

#ifdef NDEBUG
		if (!room->visited && !room->mentioned)
		{
			continue;
		}
#endif

		curses_text(COLOR_NORM, "%-*s", len + 2, room->name);

        if (room->visited)
		{
			curses_text(COLOR_NORM, "%-*s", 7, "S");
		}
		else if (room->mentioned)
		{
			curses_text(COLOR_NORM, "%-*s", 7, "M");
		}
		else
		{
			curses_text(COLOR_NORM, "%-*s", 7, "-");
		}

		curses_text(COLOR_NORM, "%s\n", room->descr);
	}

	list_destroy(data, NULL);
	log_info_f("Listed %i rooms", i);
}

// --------

/*
 * Callback to destroy a scene.
 *
 * data: Scene to destroy
 */
static void
game_scene_destroy_callback(void *data)
{
	game_scene_s *scene;

	assert(data);

	scene = data;

	if (scene->name)
	{
		free((char *)scene->name);
	}

	if (scene->descr)
	{
		free((char *)scene->descr);
	}

	if (scene->room)
	{
		free((char *)scene->room);
	}

    if (scene->aliases)
	{
		list_destroy(scene->aliases, NULL);
	}

	if (scene->words)
	{
		list_destroy(scene->words, NULL);
	}

	if (scene->next)
	{
		darray_destroy(scene->next, NULL);
	}

	free(scene);
}

/*
 * Prints a nice endscreen.
 */
static void
game_scene_endscreen(void)
{
	log_info("Game has ended");
	curses_text(COLOR_NORM, "End-Screen Placeholder\n\n");
}

/*
 * prints a nice startscreen.
 */
static void
game_scene_startscreen(void)
{
	log_info("Game has started");
	curses_text(COLOR_NORM, "Start-Screen Placeholder\n\n");
}

void
game_scene_list(void)
{
	uint16_t i;
	size_t len_scene, len_room;
	list *data;
	listnode *node;
	game_scene_s *scene;

	data = hashmap_to_list(game_scenes);
	len_scene = 0;
	len_room = 0;

	if (data)
	{
		if (data->first)
		{
			node = data->first;

			while (node)
			{
				scene = node->data;

#ifdef NDEBUG
				if (!scene->visited)
				{
					node = node->next;

					continue;
				}
#endif

				if (strlen(scene->name) > len_scene)
				{
					len_scene = strlen(scene->name);
				}

				if (strlen(scene->room) > len_room)
				{
					len_room = strlen(scene->room);
				}

				node = node->next;
			}
		}
	}

	if (len_scene < strlen("Name"))
	{
		len_scene = strlen("Name");
	}

	curses_text(COLOR_NORM, "%-*s %-*s %s\n", len_scene + 1, "Name", len_room + 1, "Room", "Description");
	curses_text(COLOR_NORM, "%-*s %-*s %s\n", len_scene + 1, "----", len_room + 1, "----", "-----------");

	for (i = 0; i <= data->count; i++)
	{
		scene = list_shift(data);

#ifdef NDEBUG
		if (!scene->visited)
		{
			continue;
		}
#endif

		curses_text(COLOR_NORM, "%-*s", len_scene + 2, scene->name);
		curses_text(COLOR_NORM, "%-*s", len_room + 2, scene->room);
		curses_text(COLOR_NORM, "%s\n", scene->descr);
	}

	list_destroy(data, NULL);
	log_info_f("Listed %i scenes", i);
}

boolean
game_scene_next(uint8_t choice)
{
	char *key;
	game_scene_s *scene;

	if (choice)
	{
		log_info_f("Advancing to the next scene. Player's choice: %i", choice);
	}
	else
	{
		log_info("Advancing to the next scene");
	}

	if (!current_scene)
	{
		if ((current_scene = hashmap_get(game_scenes, game_header->first_scene)) == NULL)
		{
			log_error_f("First scene %i doesn't exists\n", game_header->first_scene);
			quit_error("First scene doesn't exists\n");
		}
	}
	else
	{
		if (choice)
		{
			if (current_scene->next->elements == 1)
			{
				curses_text(COLOR_NORM, "No choice possible");

				return FALSE;
			}
			if (choice > current_scene->next->elements)
			{
				curses_text(COLOR_NORM, "Invalid choice");

				return FALSE;
			}

			key = darray_get(current_scene->next, choice - 1);

			if (!strcmp(key, "END"))
			{
				current_scene = NULL;
				game_end = 1;

				return TRUE;
			}

			if ((scene = hashmap_get(game_scenes, key)) == NULL)
			{
				log_error_f("Scene %s doesn't exists\n", game_header->first_scene);
				quit_error("A Scene doesn't exists\n");
			}
			else
			{
				current_scene = scene;
			}
		}
		else
		{
			if (current_scene->next->elements > 1)
			{
				curses_text(COLOR_NORM, "Make your choice\n");

				return FALSE;
			}

			key = darray_get(current_scene->next, 0);

			if (!strcmp(key, "END"))
			{
				current_scene = NULL;
				game_end = 1;

				return TRUE;
			}

			if ((scene = hashmap_get(game_scenes, key)) == NULL)
			{
				log_error_f("Scene %s doesn't exists\n", game_header->first_scene);
				quit_error("A Scene doesn't exists\n");
			}
			else
			{
				current_scene = scene;
			}
		}
	}

	return TRUE;
}

void
game_scene_play(const char *key)
{
	uint16_t i;
	listnode *lnode;
	game_room_s *room;
	game_scene_s *scene;

	if (key)
	{
		if ((scene = hashmap_get(game_scenes, key)) == NULL)
		{
			curses_text(COLOR_NORM, "No such room: %s\n", key);

			return;
		}

#ifdef NDEBUG
		if (!scene->visited)
		{
			curses_text(COLOR_NORM, "No such room: %s\n", key);

			return;
		}
#endif
	}
	else
	{
		scene = current_scene;
	}

	if (!game_end && !scene)
	{
		game_scene_startscreen();

		return;
	}

	if (game_end)
	{
		game_scene_endscreen();

		return;
	}

	log_info_f("Playing scene %s", current_scene->name);

	// Mark scene as visited
	scene->visited = 1;

	// Mark room as seen
	if ((room = hashmap_get(game_rooms, scene->room)) == NULL)
	{
		log_error_f("Room %s doesn't exist", scene->room);
		quit_error("Room doesn't exist");
	}

	room->mentioned= 1;
	room->visited = 1;

	// Print description
	lnode = scene->words->first;

	for (i = 0; i < scene->words->count; i++)
	{
		if (!strcmp(lnode->data, "\n") || i == scene->words->count - 1)
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

// --------

void
game_init(const char *file)
{
	assert(file);

    log_info("Initializing game");

	if (!game_header)
	{
		if ((game_header = calloc(1, sizeof(game_header_s))) == NULL)
		{
			quit_error("PANIC: Couldn't allocate memory");
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
	log_info("Shutting down game");

	if (game_header)
	{
		free((char *)game_header->game);
		free((char *)game_header->author);
		free((char *)game_header->date);
		free((char *)game_header->uid);
		free((char *)game_header->first_scene);

		free(game_header);
		game_header = NULL;
	}

	if (game_rooms)
	{
		hashmap_destroy(game_rooms, game_room_destroy_callback);
	}

	if (game_scenes)
	{
		hashmap_destroy(game_scenes, game_scene_destroy_callback);
	}
}

