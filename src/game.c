/*
 * game.c
 * ------
 *
 * This file runs the game. The upperst level
 * is a scene, which is composed of a room and
 * a description. Further objects are a small
 * glossary and links.
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
#include "quit.h"

#include "data/hashmap.h"
#include "data/list.h"
#include "i18n/i18n.h"

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

/*********************************************************************
 *                                                                   *
 *                        Support Functions                          *
 *                                                                   *
 *********************************************************************/

/*
 * Link handling. The link must be passed with
 * links markers (|) around it. They're removed
 * and the link is matched against all objects.
 * If a match is found, the corresponding color
 * is returned. If there's no match, TINT_NORM
 * is returned.
 *
 * Please note that the matcher uses a "first
 * match serves" approach, e.g.  if a match is
 * found it shadows all other objects.
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
			game_stats->glossary_mentioned++;
		}

		return TINT_GLOSSARY;
	}

	// Room
	if ((room = hashmap_get(game_rooms, link)) != NULL)
	{
		if (!room->mentioned)
		{
			room->mentioned = 1;
		}

		return TINT_ROOM;
	}

	log_warn_f("%s: %s", i18n_link_didntmatch, link);

	return TINT_NORM;
}

/*
 * Prints a description. Link detection is performed
 * and the links are matched by game_link_match().
 * Links are printed in a specific color.
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
				log_error(i18n_link_nestedlink);

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(TINT_NORM, cur);
				}
				else
				{
					curses_text(TINT_NORM, "%s ", cur);
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
				quit_error(POUTOFMEM);
			}

			strncat(link, cur, len);
			strncat(link, " ", len);

			node = node->next;
			continue;
		}

		// Word ends a link
		if (strlen(cur) >= 3)
		{
			if (!strncmp(&cur[strlen(cur) - 1], "|", 1)
					|| !strncmp(&cur[strlen(cur) - 2], "|", 1))
			{
				if (!link)
				{
					log_error(i18n_link_notopened);

					if (!node->next || !strcmp(cur, "\n"))
					{
						curses_text(TINT_NORM, cur);
					}
					else
					{
						curses_text(TINT_NORM, "%s ", cur);
					}

					node = node->next;
					continue;
				}

				oldlen = len;
				len = strlen(cur) + len + 2;

				if ((link = realloc(link, len)) == NULL)
				{
					quit_error(POUTOFMEM);
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
		}

		// Word is part of a link
		if (link)
		{
			if (!strcmp(cur, "\n"))
			{
				log_error(i18n_link_linebreak);

				if (!node->next || !strcmp(cur, "\n"))
				{
					curses_text(TINT_NORM, "%s ", link);
					curses_text(TINT_NORM, cur);
				}
				else
				{
					curses_text(TINT_NORM, "%s ", link);
					curses_text(TINT_NORM, "%s ", cur);
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
				quit_error(POUTOFMEM);
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
			curses_text(TINT_NORM, cur);
		}
		else
		{
			curses_text(TINT_NORM, "%s ", cur);
		}

		node = node->next;
	}

	// Link still open
	if (link)
	{
		log_error(i18n_link_openatend);
	}

	curses_text(TINT_NORM, "\n");
}


// --------

/*********************************************************************
 *                                                                   *
 *                        Glossary Functions                         *
 *                                                                   *
 *********************************************************************/

/*
 * Callback to destroy an instance of 'game_glossary_s'.
 * To be passed to hashmap_destroy() or the like.
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

/*
 * Callback function to qsort for the glossary.
 */
int32_t
game_glossary_sort_callback(const void *msg1, const void *msg2)
{
	const listnode *l1;
	const listnode *l2;
	game_glossary_s *g1;
	game_glossary_s *g2;
	int32_t ret;

	l1 = *(const listnode **)msg1;
	l2 = *(const listnode **)msg2;

	g1 = (game_glossary_s *)l1->data;
	g2 = (game_glossary_s *)l2->data;

	ret = strcmp(g1->name, g2->name);

	return ret;
}

void
game_glossary_list(void)
{
	game_glossary_s *entry;
	list *data;
	listnode *node;
	size_t len_entry;
	uint16_t i;

    data = hashmap_to_list(game_glossary);
	list_sort(data, game_glossary_sort_callback);
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

	if (len_entry < strlen(i18n_entry))
	{
		len_entry = strlen(i18n_entry);
	}

	curses_text(TINT_NORM, "%-*s %s\n", len_entry + 2, i18n_entry, i18n_head_description);

	for (i = 0; i < strlen(i18n_entry); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "%-*s", len_entry + 2 - strlen(i18n_entry) + 1, " ");

	for (i = 0; i < strlen(i18n_head_description); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "\n");

	for (i = 0; i <= data->count; i++)
	{
		entry = list_shift(data);

#ifdef NDEBUG
		if (!entry->mentioned)
		{
			continue;
		}
#endif

		curses_text(TINT_NORM, "%-*s %s\n", len_entry + 2, entry->name, entry->descr);
	}

	list_destroy(data, NULL);
	log_info_f("%s: %i", i18n_glossary_entrieslisted, i);
}

void
game_glossary_print(const char *key)
{
	game_glossary_s *entry;
	listnode *lnode;
	uint16_t i;

	assert(key);

	if ((entry = hashmap_get(game_glossary, key)) == NULL)
	{
		curses_text(TINT_NORM, "%s: %s\n", i18n_glossary_notfound, key);

		return;
	}

#ifndef NDEBUG
	curses_text(TINT_HIGH, "%s", entry->name);

	if (entry->aliases)
	{
		if (entry->aliases->first)
		{
			lnode = entry->aliases->first;
		}

		curses_text(TINT_NORM, " (");

		for (i = 0; i < entry->aliases->count; i++)
		{
			if (i)
			{
				curses_text(TINT_NORM, ", ");
			}

			curses_text(TINT_HIGH, lnode->data);
			lnode = lnode->next;
		}

		curses_text(TINT_NORM, ")");
	}

	curses_text(TINT_NORM, ":\n");

#else

	if (!entry->mentioned)
	{
		curses_text(TINT_NORM, "%s: %s\n", i18n_glossary_notfound, key);

		return;
	}
#endif // NDEBUG

	game_print_description(entry->words);
}

// --------

/*********************************************************************
 *                                                                   *
 *                          Room Functions                           *
 *                                                                   *
 *********************************************************************/

/*
 * Callback to destroy an instance of 'game_room_s'.
 * To be passed to hashmap_destroy() or the like.
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

/*
 * Callback function to qsort for the rooms.
 */
int32_t
game_room_sort_callback(const void *msg1, const void *msg2)
{
	const listnode *l1;
	const listnode *l2;
	game_room_s *r1;
	game_room_s *r2;
	int32_t ret;

	l1 = *(const listnode **)msg1;
	l2 = *(const listnode **)msg2;

	r1 = (game_room_s *)l1->data;
	r2 = (game_room_s *)l2->data;

	ret = strcmp(r1->name, r2->name);

	return ret;
}

void
game_room_describe(const char *key)
{
	game_room_s *room;
	listnode *lnode;
	uint16_t i;

	assert(key);

	if ((room = hashmap_get(game_rooms, key)) == NULL)
	{
		curses_text(TINT_NORM, "%s\n", i18n_room_notfound, key);

		return;
	}

#ifndef NDEBUG
	curses_text(TINT_HIGH, "%s", room->name);

	if (room->aliases)
	{
		if (room->aliases->first)
		{
			lnode = room->aliases->first;
		}

		curses_text(TINT_NORM, " (");

		for (i = 0; i < room->aliases->count; i++)
		{
			if (i)
			{
				curses_text(TINT_NORM, ", ");
			}

			curses_text(TINT_HIGH, lnode->data);
			lnode = lnode->next;
		}

		curses_text(TINT_NORM, ")");
	}

	curses_text(TINT_NORM, ":\n");

#else

	if (room->mentioned && !room->visited)
	{
		curses_text(TINT_NORM, "%s\n", i18n_room_mentioned);

		return;
	}

	if (!room->visited)
	{
		curses_text(TINT_NORM, "%s\n", i18n_room_notfound, key);

		return;
	}
#endif // NDEBUG

	// Print description
	game_print_description(room->words);
}

void
game_rooms_list(void)
{
	game_room_s *room;
	list *data;
	listnode *node;
	size_t len;
	uint16_t i;

	data = hashmap_to_list(game_rooms);
	list_sort(data, game_room_sort_callback);
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

	if (len < strlen(i18n_name))
	{
		len = strlen(i18n_name);
	}

	curses_text(TINT_NORM, "%-*s %-*s %s\n", len + 1, i18n_name, strlen(i18n_head_state)
			+ 1, i18n_head_state, i18n_head_description);

	for (i = 0; i < strlen(i18n_name); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "%-*s", len + 2 - strlen(i18n_name), " ");

	for (i = 0; i < strlen(i18n_head_state); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "%-*s", 2, " ");

	for (i = 0; i < strlen(i18n_head_description); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "\n");

	for (i = 0; i <= data->count; i++)
	{
		room = list_shift(data);

#ifdef NDEBUG
		if (!room->visited && !room->mentioned)
		{
			continue;
		}
#endif

		curses_text(TINT_NORM, "%-*s", len + 2, room->name);

        if (room->visited)
		{
			curses_text(TINT_NORM, "%-*s", strlen(i18n_head_state) + 2, "S");
		}
		else if (room->mentioned)
		{
			curses_text(TINT_NORM, "%-*s", strlen(i18n_head_state) + 2, "M");
		}
		else
		{
			curses_text(TINT_NORM, "%-*s", strlen(i18n_head_state) + 2, "-");
		}

		curses_text(TINT_NORM, "%s\n", room->descr);
	}

	list_destroy(data, NULL);
	log_info_f("%s: %i", i18n_room_roomslisted, i);
}

// --------

/*********************************************************************
 *                                                                   *
 *                         Scene Functions                           *
 *                                                                   *
 *********************************************************************/

/*
 * Callback to destroy an instance of 'game_scene_s'.
 * To be passed to hashmap_destroy() or the like.
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
 * Callback function to qsort for the scenes.
 */
int32_t
game_scene_sort_callback(const void *msg1, const void *msg2)
{
	const listnode *l1;
	const listnode *l2;
	game_room_s *r1;
	game_room_s *r2;
	int32_t ret;

	l1 = *(const listnode **)msg1;
	l2 = *(const listnode **)msg2;

	r1 = (game_room_s *)l1->data;
	r2 = (game_room_s *)l2->data;

	ret = strcmp(r1->name, r2->name);

	return ret;
}

/*
 * Prints a nice endscreen with the statistics.
 * The prompt is reset to game_header->prompt.
 */
static void
game_scene_endscreen(void)
{
	log_info(i18n_game_end);

	curses_text(TINT_NORM, "%s ", i18n_end_congratulations);
	curses_text(TINT_HIGH, "%s", game_header->game);
	curses_text(TINT_NORM, ".\n\n");

	curses_text(TINT_NORM, "%s:\n", i18n_end_stats);
	curses_text(TINT_NORM, " - %i %s %i %s\n", game_stats->glossary_mentioned,
			i18n_from, game_stats->glossary_total, i18n_end_glossaryentriesseen);
	curses_text(TINT_NORM, " - %i %s %i %s\n", game_stats->rooms_visited,
			i18n_from, game_stats->rooms_total, i18n_end_roomsvisited);
	curses_text(TINT_NORM, " - %i %s %i %s\n\n", game_stats->scenes_visited,
			i18n_from, game_stats->scenes_total, i18n_end_scenesplayed);

	curses_status(i18n_end_statusbar);

	// Set prompt
	if (game_header->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(game_header->prompt) + 3)) == NULL)
		{
			quit_error(POUTOFMEM);
		}

		sprintf(curses_prompt, "%s: ", game_header->prompt);
	}
}

/*
 * Prints a nice startscreen including statistics
 * and a nice hint to start the game or how to get
 * help. The prompt is set to game_header->prompt.
 */
static void
game_scene_startscreen(void)
{
	log_info(i18n_game_start);

	curses_text(TINT_NORM, "%s ", i18n_start_welcome);
	curses_text(TINT_HIGH, "%s\n", game_header->game);
	curses_text(TINT_NORM, "%s %s\n\n", i18n_start_author, game_header->author);

	curses_text(TINT_NORM, "%s:\n", i18n_start_stats);
	curses_text(TINT_NORM, " - %i %s\n", game_stats->glossary_total, i18n_glossary_entrieslisted);
	curses_text(TINT_NORM, " - %i %s\n", game_stats->rooms_total, i18n_start_rooms);
	curses_text(TINT_NORM, " - %i %s\n\n", game_stats->scenes_total, i18n_start_scenes);

	curses_text(TINT_NORM, "%s ", i18n_start_help1);
	curses_text(TINT_HIGH, "%s ", i18n_cmdhelp);
	curses_text(TINT_NORM, "%s ", i18n_start_help2);
	curses_text(TINT_HIGH, "%s ", i18n_cmdnext);
	curses_text(TINT_NORM, "%s.\n\n", i18n_start_help3);

	curses_status("%s %s", i18n_start_welcome, game_header->game);

	// Set prompt
	if (game_header->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(game_header->prompt) + 3)) == NULL)
		{
			quit_error(POUTOFMEM);
		}

		sprintf(curses_prompt, "%s: ", game_header->prompt);
	}
}

void
game_scene_list(void)
{
	game_scene_s *scene;
	list *data;
	listnode *node;
	size_t len_scene, len_room;
	uint16_t i;

	data = hashmap_to_list(game_scenes);
	list_sort(data, game_scene_sort_callback);
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

	if (len_scene < strlen(i18n_name))
	{
		len_scene = strlen(i18n_name);
	}

	curses_text(TINT_NORM, "%-*s %-*s %s\n", len_scene + 1, i18n_name, len_room + 1,
			i18n_room, i18n_head_description);

	for (i = 0; i < strlen(i18n_name); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "%-*s", len_scene + 2 - strlen(i18n_name), " ");

	for (i = 0; i < strlen(i18n_room); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "%-*s", len_room + 2 - strlen(i18n_room), " ");

	for (i = 0; i < strlen(i18n_head_description); i++)
	{
		curses_text(TINT_NORM, "-");
	}

	curses_text(TINT_NORM, "\n");

	for (i = 0; i <= data->count; i++)
	{
		scene = list_shift(data);

#ifdef NDEBUG
		if (!scene->visited)
		{
			continue;
		}
#endif

		curses_text(TINT_NORM, "%-*s", len_scene + 2, scene->name);
		curses_text(TINT_NORM, "%-*s", len_room + 2, scene->room);
		curses_text(TINT_NORM, "%s\n", scene->descr);
	}

	list_destroy(data, NULL);
	log_info_f("%s: %i", i18n_scene_listed, i);
}

boolean
game_scene_next(uint8_t choice)
{
	char *key;
	game_scene_s *scene;

	if (choice)
	{
		log_info_f("%s. %s: %i", i18n_scene_next, i18n_scene_playerschoice, choice);
	}
	else
	{
		log_info(i18n_scene_next);
	}

	if (!current_scene)
	{
		if ((current_scene = hashmap_get(game_scenes, game_header->first_scene)) == NULL)
		{
			log_error_f("%s: %i\n", i18n_scene_firstnotfound,  game_header->first_scene);
			quit_error(PFIRSTSCENENOTFOUND);
		}
	}
	else
	{
		if (choice)
		{
			if (current_scene->next->elements == 1)
			{
				curses_text(TINT_NORM, i18n_scene_nochoice);

				return FALSE;
			}
			if (choice > current_scene->next->elements)
			{
				curses_text(TINT_NORM, i18n_scene_invalidchoice);

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
				log_error_f("%s: %s\n", i18n_scene_notfound, game_header->first_scene);
				quit_error(PSCENENOTFOUND);
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
				curses_text(TINT_NORM, "%s\n", i18n_scene_choice);

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
				log_error_f("%s: %s\n", i18n_scene_notfound, game_header->first_scene);
				quit_error(PSCENENOTFOUND);
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
			curses_text(TINT_NORM, "%s: %s\n", i18n_scene_notfound, key);

			return;
		}

#ifdef NDEBUG
		if (!scene->visited)
		{
			curses_text(TINT_NORM, "%s: %s\n", i18n_scene_notfound, key);

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

	log_info_f("%s %s", i18n_scene_play, current_scene->name);

	// Mark scene as visited
	if (!scene->visited)
	{
		scene->visited = TRUE;
		game_stats->scenes_visited++;
	}

	// Mark room as visited
	if ((room = hashmap_get(game_rooms, scene->room)) == NULL)
	{
		log_error_f("%s: %s", i18n_room_notfound, scene->room);
		quit_error(PROOMNOTFOUND);
	}

	if (!room->visited)
	{
		room->mentioned = TRUE;
		room->visited = TRUE;
		game_stats->rooms_visited++;
	}

	// Set statusbar
	curses_status("%s: %i/%i || %s: %s", i18n_scene, game_stats->scenes_visited,
			game_stats->scenes_total, i18n_room, room->descr);

	// Set prompt
	if (scene->prompt)
	{
		if (curses_prompt)
		{
			free(curses_prompt);
		}

        if ((curses_prompt = malloc(strlen(scene->prompt) + 3)) == NULL)
		{
			quit_error(POUTOFMEM);
		}

		sprintf(curses_prompt, "%s: ", scene->prompt);
	}

	// Print description
	game_print_description(scene->words);
}

// --------

/*********************************************************************
 *                                                                   *
 *                    Initialization Functions                       *
 *                                                                   *
 *********************************************************************/

void
game_init(const char *file)
{
	assert(file);

    log_info(i18n_game_init);

	if (!game_header)
	{
		if ((game_header = calloc(1, sizeof(game_header_s))) == NULL)
		{
			quit_error(POUTOFMEM);
		}
	}

	if (!game_stats)
	{
		if ((game_stats = calloc(1, sizeof(game_stats_s))) == NULL)
		{
			quit_error(POUTOFMEM);
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
	log_info(i18n_game_quit);

	if (game_header)
	{
		free((char *)game_header->game);
		free((char *)game_header->author);
		free((char *)game_header->date);
		free((char *)game_header->uid);
		free((char *)game_header->first_scene);

		if (game_header->prompt)
		{
			free((char *)game_header->prompt);
		}

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

