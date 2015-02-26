/*
 * parser.c
 * --------
 *
 * Parses the game file. Please note that this hand
 * written implementation is not that robust, if an
 * error is detected, it just bails out.
 */

#define _WITH_GETLINE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "curses.h"
#include "game.h"
#include "log.h"
#include "util.h"
#include "data/list.h"

// --------

// What we are parsing?
static uint8_t is_header;
static uint8_t is_room;
static uint8_t is_scene;

// Lines parsed so far
static int32_t count;

// --------

/*
 * Concanates elements of a list
 * filled with strings into one
 * string.
 *
 * tokens: List to concanate
 */
static char
*parser_concat(list *tokens)
{
	char *cur;
	char *string;
	size_t len;
	size_t oldlen;

	assert(tokens);

	len = 0;
	string = NULL;

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!string)
		{
			oldlen = 0;
			len = strlen(cur) + 2;
		}
		else
		{
			oldlen = len;
			len += strlen(string) + strlen(cur) + 2;
		}

		if ((string = realloc(string, len)) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}

		memset(string + oldlen, 0, len - oldlen);

		strncat(string, cur, len);

		if (tokens->count > 0)
		{
			strncat(string, " ", len);
		}
	}

	return string;
}

/*
 * Bails out with error.
 *
 * line: Line number where the error occured
 */
static void
parser_error(void)
{
	log_error_f("Parser error in line %i of file\n", count);
	quit_error("Parser error");
}

/*
 * Tokenizes a line.
 *
 * line: Line to tokenize
 */
static list
*parser_tokenize(char *line)
{
	char *token;
	char *work;
	list *tokens;
	size_t len;
	uint32_t i;

	assert(line);

	work = line;
	tokens = list_create();

	// Strip whitespaces
	while (work[0] == ' ')
	{
		work++;
	}

	len = strlen(work);

	while (work[len] == ' ')
	{
		work[len] = '\0';
		work--;
	}

	// Strip comments
	for (i = 0; i < strlen(work); i++)
	{
		if (work[i] == '%')
		{
			work[i] = '\0';

			break;
		}
	}

	while ((token = strsep(&work, " \n")) != NULL)
	{
		if (!strlen(token))
		{
			continue;
		}

		list_push(tokens, token);
	}

	return tokens;
}

// --------

/*
 * Checks if the header was parsed successfull.
 */
static void
parser_check_header(void)
{
	if (!game_header->game)
	{
		quit_error("No game name specified\n");
	}
	else if (!game_header->author)
	{
		quit_error("No author specified\n");
	}
	else if (!game_header->date)
	{
		quit_error("PANIC: No copyright date speficied\n");
	}
	else if (!game_header->uid)
	{
		quit_error("PANIC: No UID specified\n");
	}
	else if (!game_header->first_scene)
	{
		quit_error("PANIC: No starting scene specified\n");
	}

	log_info("Game specifications are:");
	log_info_f("Game:   %s", game_header->game);
	log_info_f("Author: %s", game_header->author);
	log_info_f("Date:   %s", game_header->date);
	log_info_f("UID:    %s", game_header->uid);
}

/*
 * Parses the game header.
 *
 * tokens: Line to parse
 */
static void
parser_header(list *tokens)
{
	char *cur;

	assert(tokens);

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "#GAME:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->game = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#AUTHOR:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->author = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#DATE:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->date = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#UID:"))
		{
			if (tokens->count != 1)
			{
				parser_error();
			}

			game_header->uid = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#START:"))
		{
			if (tokens->count != 1)
			{
				parser_error();
			}

			game_header->first_scene = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (tokens->count != 0)
			{
				parser_error();
			}

			parser_check_header();
			is_header = 0;
		}
		else
		{
			parser_error();
		}
	}
}

// --------

/*
 * Checks if a room was parsed successfull
 *
 * new: Room to be checked
 */
static void
parser_check_room(game_room_s *room)
{
	assert(room);

	if (!room->name)
	{
		parser_error();
	}

	if (!room->descr)
	{
		parser_error();
	}

	if (!room->words)
	{
		parser_error();
	}
	else
	{
		if (!room->words->count)
		{
			parser_error();
		}
	}

	if (!room->aliases)
	{
		log_info_f("Room: %s (0 aliases, %i words)", room->name,
				room->words->count);
	}
	else
	{
		log_info_f("Room: %s (%i aliases, %i words)", room->name,
				room->aliases->count, room->words->count);
	}
}

/*
 * Does some sanity checks and adds the
 * room to the global room list.
 */
static void
parser_add_room(game_room_s *room)
{
	listnode *lnode;
	game_room_s *test;
	int32_t i;

	parser_check_room(room);

    if ((test = hashmap_get(game_rooms, room->name)) != NULL)
	{
		log_warn_f("There's already a room with name or alias %s", room->name);
	}

	hashmap_add(game_rooms, room->name, room, MAIN);
	game_stats->rooms_total++;

	// Aliases
	if (room->aliases)
	{
		if (room->aliases->first)
		{
			lnode = room->aliases->first;

			for (i = 0; i < room->aliases->count; i++)
			{
				if ((test = hashmap_get(game_rooms, lnode->data)) != NULL)
				{
					log_warn_f("There's already a room with name or alias %s", room->name);
				}

				hashmap_add(game_rooms, lnode->data, room, ALIAS);
				lnode = lnode->next;
			}
		}
	}
}

/*
 * Parses a room.
 *
 * tokens: Line to parse
 */
static void
parser_room(list *tokens)
{
	char *cur;
	int32_t i;
	static game_room_s *room;

	assert(tokens);

	if (!room)
	{
		if ((room = calloc(1, sizeof(game_room_s))) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}
	}

	// Empty input line
	if (!tokens->count)
	{
		if (room->words)
		{
			if (room->words->last)
			{
				if (strcmp(room->words->last->data, "\n"))
				{
					list_push(room->words, strdup("\n"));
				}
			}
		}
	}

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "#ROOM:"))
		{
			if (room->name || room->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			room->name = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#DESCR:"))
		{
			if (room->words)
			{
				parser_error();
			}

			room->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#ALIAS:"))
		{
			if (room->words)
			{
				parser_error();
			}

			if (!room->aliases)
			{
				room->aliases = list_create();
			}

			list_push(room->aliases, parser_concat(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (room->words)
			{
				if (room->words->last)
				{
					while (!strcmp(room->words->last->data, "\n"))
					{
						free(list_pop(room->words));
					}
				}
			}

			parser_add_room(room);

			is_room = 0;
			room = NULL;
		}
		else
		{
			if (!room->words)
			{
				room->words = list_create();
			}

			list_push(room->words, strdup(cur));

			for (i = 0; tokens->count > 0; i++)
			{
				list_push(room->words, strdup(list_shift(tokens)));
			}
		}
	}
}

// --------

/*
 * Checks if a scene was parsed successfull.
 *
 * new: Scene to be checked
 */
static void
parser_check_scene(game_scene_s *scene)
{
	assert(scene);

	if (!scene->name)
	{
		parser_error();
	}

	if (!scene->descr)
	{
		parser_error();
	}

	if (!scene->words)
	{
		parser_error();
	}
	else
	{
		if (!scene->words->count)
		{
			parser_error();
		}
	}

	if (!scene->next)
	{
		parser_error();
	}
	else
	{
		if (!scene->next->elements)
		{
			parser_error();
		}
		else if (scene->next->elements > 9)
		{
			parser_error();
		}
	}

	if (!scene->aliases)
	{
		log_info_f("Scene: %s (0 aliases, %i choices, %i words)",
				scene->name, scene->next->elements, scene->words->count);
	}
	else
	{
		log_info_f("Scene: %s (%i aliases, %i choices, %i words)",
				scene->name, scene->aliases->count, scene->next->elements,
				scene->words->count);
	}
}

/*
 * Does some sanity checks and adds the
 * scene to the global scene list.
 *
 * new: Scene to add
 */
static void
parser_add_scene(game_scene_s *scene)
{
	listnode *lnode;
	game_scene_s *test;
	int32_t i;

	parser_check_scene(scene);

    if ((test = hashmap_get(game_scenes, scene->name)) != NULL)
	{
		log_warn_f("There's already a scene with name or alias %s", scene->name);
	}

	hashmap_add(game_scenes, scene->name, scene, MAIN);
	game_stats->scenes_total++;

	// Aliases
	if (scene->aliases)
	{
		if (scene->aliases->first)
		{
			lnode = scene->aliases->first;

			for (i = 0; i < scene->aliases->count; i++)
			{
				if ((test = hashmap_get(game_scenes, lnode->data)) != NULL)
				{
					log_warn_f("There's already a scene with name or alias %s", scene->name);
				}

				hashmap_add(game_scenes, lnode->data, scene, ALIAS);
				lnode = lnode->next;
			}
		}
	}
}

/*
 * Parses a scene.
 *
 * tokens: Line to parse
 */
static void
parser_scene(list *tokens)
{
	char *cur;
	int32_t i;
	static game_scene_s *scene;

	assert(tokens);

	if (!scene)
	{
		if ((scene = calloc(1, sizeof(game_scene_s))) == NULL)
		{
			quit_error("Couldn't allocate memory");
		}
	}

	// Empty input line
	if (!tokens->count)
	{
		if (scene->words)
		{
			if (scene->words->last)
			{
				if (strcmp(scene->words->last->data, "\n"))
				{
					list_push(scene->words, strdup("\n"));
				}
			}
		}
	}

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "#SCENE:"))
		{
			if (scene->name || scene->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			scene->name = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#DESCR:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			scene->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#PROMPT:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			if (curses_prompt)
			{
				free(curses_prompt);
			}

            scene->prompt = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#ALIAS:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			if (!scene->aliases)
			{
				scene->aliases = list_create();
			}

			list_push(scene->aliases, parser_concat(tokens));
		}
		else if (!strcmp(cur, "#ROOM:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			scene->room = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#NEXT:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			if (!scene->next)
			{
				scene->next = darray_create();
			}

			darray_push(scene->next, parser_concat(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (scene->words)
			{
				if (scene->words->last)
				{
					while (!strcmp(scene->words->last->data, "\n"))
					{
						free(list_pop(scene->words));
					}
				}
			}

			parser_add_scene(scene);

			is_scene = 0;
			scene = NULL;
		}
		else
		{
			if (!scene->words)
			{
				scene->words = list_create();
			}

			list_push(scene->words, strdup(cur));

			for (i = 0; tokens->count > 0; i++)
			{
				list_push(scene->words, strdup(list_shift(tokens)));
			}
		}
	}
}

// --------

void
parser_game(const char *file)
{
	FILE *game;
	char *line;
	char *tmp;
	size_t linecap;
	ssize_t linelen;
	struct stat sb;
	list *tokens;

	assert(file);

	if ((stat(file, &sb)) != 0)
	{
		quit_error("Game file doesn't exists\n");
	}

	if (!S_ISREG(sb.st_mode))
	{
		quit_error("Game file ist not a regular file\n");
	}

	if ((game = fopen(file, "r")) == NULL)
	{
		quit_error("Couldn't open game file");
	}

	log_info_f("Parsing game file: %s", file);

	line = NULL;
	linecap = 0;

	// Header is always the first section
	is_header = 1;

    while ((linelen = getline(&line, &linecap, game)) > 0)
	{
		count++;

        // Line is comment
		if (line[0] == '%')
		{
			continue;
		}

		tokens = parser_tokenize(line);

		// What we are parsing?
		if (!(is_header || is_room || is_scene))
		{
			if (tokens->count > 0)
			{
				tmp = list_shift(tokens);

				if (!strcmp(tmp, "#ROOM:"))
				{
					is_room = 1;
					list_unshift(tokens, tmp);
				}
				if (!strcmp(tmp, "#SCENE:"))
				{
					is_scene = 1;
					list_unshift(tokens, tmp);
				}
				else
				{
					//parser_error();
				}
			}
			else
			{
				list_destroy(tokens, NULL);
				continue;
			}
		}

		// Header
		if (is_header)
		{
			parser_header(tokens);
		}

		// Room
		if (is_room)
		{
			parser_room(tokens);
		}

		// Scene
		if (is_scene)
		{
			parser_scene(tokens);
		}

		list_destroy(tokens, NULL);
	}

	free(line);
	log_info_f("Parsed %i lines", count);
}

