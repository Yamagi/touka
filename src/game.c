/*
 * game.c
 * ------
 *
 * This file runs the game. The upperst level
 * is a scene, which is composed of a room and
 * a description. The description can have
 * links
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

// Glossary
hashmap *game_glossary;

// Rooms
hashmap *game_rooms;

// Scenes
hashmap *game_scenes;

// Statistics
game_stats_s *game_stats;

// Current scene
game_scene_s *current_scene;

// Has the game ended?
boolean game_end;

// --------

/*
 * Removes the link markers around the
 * given link, matches it against all
 * known objects and returns the color
 * desriping the object. Of no object
 * is found, COLOR_NORM is returned.
 *
 * Please note that the matcher uses a
 * "first match serves" approach, e.g.
 * if a match is found it shadows all
 * other objects.
 *
 * link: Link to match.
 */
static uint32_t
game_match_link(char *link)
{
	game_glossary_s *glossary;
	game_room_s *room;

	uint32_t i;

	assert(link);

	// Remove | at the end
	link[strlen(link) - 1] = '\0';

	// Remove | at the beginning
	for (i = 0; i < strlen(link); i++)
	{
		link[i] = link[i + 1];
	}

	// Glossary
	if ((glossary = hashmap_get(game_glossary, link)) != NULL)
	{
		if (!glossary->mentioned)
		{
			glossary->mentioned = 1;
		}

		return COLOR_GLOSSARY;
	}

	// Room
	if ((room = hashmap_get(game_rooms, link)) != NULL)
	{
		if (!room->mentioned)
		{
			room->mentioned = 1;
		}

		return COLOR_ROOM;
	}

	log_warn_f("Link %s didn't match anything", link);

	return COLOR_NORM;
}

/*
 * Prints a description. Link detection
 * is performed and the links are matched
 * by game_link_match(). Links are printed
 * in a specific color.
 *
 * words: List with words to print
 */
static void
game_print_description(list *words)
{
	char *cur;
	char *link;
	char tmp[3];
	listnode *node;
	size_t len, oldlen;
	uint32_t color;

	assert (words);

	link = NULL;
	node = words->first;
	memset(tmp, 0, sizeof(tmp));

	while (node)
	{
		cur = node->data;

		// Word starts a link
		if (!strncmp(cur, "|", 1))
		{
			if (link)
			{
				log_error("Nested link detected");

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(COLOR_NORM, cur);
				}
				else
				{
					curses_text(COLOR_NORM, "%s ", cur);
				}

				node = node->next;
				continue;
			}

			if (!strncmp(&cur[strlen(cur) - 1], "|", 1)
					|| !strncmp(&cur[strlen(cur) - 2], "|", 1))
			{
				link = strdup(cur);

				if (!strncmp(&link[strlen(link) - 2], "|", 1))
				{
					stpncpy(tmp, &link[strlen(link) - 1], sizeof(tmp));
					link[strlen(link) - 1] = '\0';
				}

				color = game_match_link(link);

				curses_text(color, link);

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(color, tmp);
				}
				else
				{
					curses_text(color, "%s ", tmp);
				}

				free(link);
				link = NULL;
				memset(tmp, 0, sizeof(tmp));

				node = node->next;
				continue;
			}

			oldlen = 0;
			len = strlen(cur) + 2;

			if ((link = calloc(1, len)) == 0)
			{
				quit_error("Couldn't allocate memory");
			}

			strncat(link, cur, len);
			strncat(link, " ", len);

			node = node->next;
			continue;
		}

		// Word ends a link
		if (!strncmp(&cur[strlen(cur) - 1], "|", 1)
				|| !strncmp(&cur[strlen(cur) - 2], "|", 1))
		{
			if (!link)
			{
				log_error("Closing an unopened link");

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(COLOR_NORM, cur);
				}
				else
				{
					curses_text(COLOR_NORM, "%s ", cur);
				}

				node = node->next;
				continue;
			}

			oldlen = len;
			len = strlen(cur) + len + 2;

			if ((link = realloc(link, len)) == NULL)
			{
				quit_error("Couldn't allocate memory");
			}

			memset(link + len, 0, len - oldlen);
			strncat(link, cur, len);

			if (!strncmp(&link[strlen(link) - 2], "|", 1))
			{
				stpncpy(tmp, &link[strlen(link) - 1], sizeof(tmp));
				link[strlen(link) - 1] = '\0';
			}

			color = game_match_link(link);

			curses_text(color, link);

			if (!node->next || !strcmp(cur, "\n"))
			{
				curses_text(color, tmp);
			}
			else
			{
				curses_text(color, "%s ", tmp);
			}

			free(link);
			link = NULL;
			memset(tmp, 0, sizeof(tmp));

			node = node->next;
			continue;
		}

		// Word is part of a link
		if (link)
		{
			if (!strcmp(cur, "\n"))
			{
				log_error("Line break in link");

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(COLOR_NORM, "%s ", link);
					curses_text(COLOR_NORM, cur);
				}
				else
				{
					curses_text(COLOR_NORM, "%s ", link);
					curses_text(COLOR_NORM, "%s ", cur);
				}

				free(link);
				link = NULL;

				node = node->next;
				continue;
			}

			oldlen = len;
			len = strlen(cur) + len + 2;

			if ((link = realloc(link, len)) == NULL)
			{
				quit_error("Couldn't allocate memory");
			}

			memset(link + len, 0, len - oldlen);
			strncat(link, cur, len);
			strncat(link, " ", len);

            node = node->next;
			continue;
		}

		// Normal word
		if (!node->next || !strcmp(cur, "\n"))
		{
			curses_text(COLOR_NORM, cur);
		}
		else
		{
			curses_text(COLOR_NORM, "%s ", cur);
		}

		node = node->next;
	}

	// Link still open
	if (link)
	{
		log_error("Link still open at end of description");
	}

	curses_text(COLOR_NORM, "\n");
}


// --------

/*
 * Callback to destroy a glossary entry.
 *
 * data: Glossary to destroy
 */
static void
game_glossary_destroy_callback(void *data)
{
	game_glossary_s *entry;

	assert(data);

	entry = data;

	if (entry->name)
	{
		free((char *)entry->name);
	}

	if (entry->descr)
	{
		free((char *)entry->descr);
	}

	if (entry->aliases)
	{
		list_destroy(entry->aliases, NULL);
	}

	if (entry->words)
	{
		list_destroy(entry->words, NULL);
	}

	free(entry);
}

void
game_glossary_list(void)
{
	game_glossary_s *entry;
	list *data;
	listnode *node;
	size_t len_entry;
	uint32_t i;

    data = hashmap_to_list(game_glossary);
	len_entry = 0;

	if (data)
	{
		if (data->first)
		{
			node = data->first;

			while (node)
			{
				entry = node->data;

#ifdef NDEBUG
				if (!entry->mentioned)
				{
					node = node->next;

					continue;
				}
#endif

				if (strlen(entry->name) > len_entry)
				{
					len_entry = strlen(entry->name);
				}

				node = node->next;
			}
		}
	}

	if (len_entry < strlen("Entry"))
	{
		len_entry = strlen("Entry");
	}

	curses_text(COLOR_NORM, "%-*s %s\n", len_entry + 2, "Entry", "Description");
	curses_text(COLOR_NORM, "%-*s %s\n", len_entry + 2, "-----", "-----------");

	for (i = 0; i <= data->count; i++)
	{
		entry = list_shift(data);

#ifdef NDEBUG
		if (!entry->mentioned)
		{
			continue;
		}
#endif

		curses_text(COLOR_NORM, "%-*s %s\n", len_entry + 2, entry->name, entry->descr);
	}

	list_destroy(data, NULL);
	log_info_f("Listed %i glossary entries", i);
}

void
game_glossary_print(const char *key)
{
	uint16_t i;
	listnode *lnode;
	game_glossary_s *entry;

	assert(key);

	if ((entry = hashmap_get(game_glossary, key)) == NULL)
	{
		curses_text(COLOR_NORM, "No such glossary entry: %s\n", key);

		return;
	}

#ifndef NDEBUG
	curses_text(COLOR_HIGH, "%s", entry->name);

	if (entry->aliases)
	{
		if (entry->aliases->first)
		{
			lnode = entry->aliases->first;
		}

		curses_text(COLOR_NORM, " (");

		for (i = 0; i < entry->aliases->count; i++)
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
#endif

#ifdef NDEBUG
	if (!entry->mentioned)
	{
		curses_text(COLOR_NORM, "No such glossary entry: %s\n", key);

		return;
	}
#endif

	game_print_description(entry->words);
}

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
	game_print_description(room->words);
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

	if (scene->prompt)
	{
		free((char *)scene->prompt);
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

	curses_text(COLOR_NORM, "Congratulations! You have successfull completed ");
	curses_text(COLOR_HIGH, "%s", game_header->game);
	curses_text(COLOR_NORM, ".\n\n");

	curses_text(COLOR_NORM, "Your track record is:\n");
	curses_text(COLOR_NORM, " - %i from %i rooms visited\n", game_stats->rooms_visited,
			game_stats->rooms_total);
	curses_text(COLOR_NORM, " - %i from %i scenes played\n\n", game_stats->scenes_visited,
			game_stats->scenes_total);

	curses_status("Game is finished");

	// Set prompt
	if (game_header->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(game_header->prompt) + 3)) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}

		sprintf(curses_prompt, "%s: ", game_header->prompt);
	}
}

/*
 * prints a nice startscreen.
 */
static void
game_scene_startscreen(void)
{
	log_info("Game has started");

	curses_text(COLOR_NORM, "Welcome to ");
	curses_text(COLOR_HIGH, "%s\n", game_header->game);
	curses_text(COLOR_NORM, "Written by %s\n\n", game_header->author);

	curses_text(COLOR_NORM, "This game has:\n");
	curses_text(COLOR_NORM, " - %i rooms\n", game_stats->rooms_total);
	curses_text(COLOR_NORM, " - %i scenes\n\n", game_stats->scenes_total);

	curses_text(COLOR_NORM, "Type ");
	curses_text(COLOR_HIGH, "help ");
	curses_text(COLOR_NORM, "for help, or ");
	curses_text(COLOR_HIGH, "next ");
	curses_text(COLOR_NORM, "to start the game.\n\n");

	curses_status("Welcome to %s", game_header->game);

	// Set prompt
	if (game_header->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(game_header->prompt) + 3)) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}

		sprintf(curses_prompt, "%s: ", game_header->prompt);
	}
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
	game_room_s *room;
	game_scene_s *scene;

	if (key)
	{
		if ((scene = hashmap_get(game_scenes, key)) == NULL)
		{
			curses_text(COLOR_NORM, "No such scene: %s\n", key);

			return;
		}

#ifdef NDEBUG
		if (!scene->visited)
		{
			curses_text(COLOR_NORM, "No such scene: %s\n", key);

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
	if (!scene->visited)
	{
		scene->visited = TRUE;
		game_stats->scenes_visited++;
	}

	// Mark room as seen
	if ((room = hashmap_get(game_rooms, scene->room)) == NULL)
	{
		log_error_f("Room %s doesn't exist", scene->room);
		quit_error("Room doesn't exist");
	}

	if (!room->visited)
	{
		room->mentioned = TRUE;
		room->visited = TRUE;
		game_stats->rooms_visited++;
	}

	// Set statusbar
	curses_status("Scene: %i/%i || Room: %s", game_stats->scenes_visited,
			game_stats->scenes_total, room->descr);

	// Set prompt
	if (scene->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(scene->prompt) + 3)) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}

		sprintf(curses_prompt, "%s: ", scene->prompt);
	}

	// Print description
	game_print_description(scene->words);
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
			quit_error("Couldn't allocate memory");
		}
	}

	if (!game_stats)
	{
		if ((game_stats = calloc(1, sizeof(game_stats_s))) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}
	}

	if (!game_glossary)
	{
		game_glossary = hashmap_create(128);
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

	if (game_stats)
	{
		free(game_stats);
		game_stats = NULL;
	}

	if (game_glossary)
	{
		hashmap_destroy(game_glossary, game_glossary_destroy_callback);
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

