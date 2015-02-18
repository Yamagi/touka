/*
 * parser.c:
 *  - Parses the game file:
 *    - Header
 */

#define _WITH_GETLINE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
	int32_t len;
	int32_t oldlen;

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
			perror("PANIC: Couldn't allocate memory");
			quit_error();
		}

		memset(string + oldlen, 0, len - oldlen);

		strlcat(string, cur, len);

		if (tokens->count > 0)
		{
			strlcat(string, " ", len);
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
	fprintf(stderr, "Parser error in line %i\n", count);
	quit_error();
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
		fprintf(stderr, "PANIC: No game name specified\n");
		quit_error();
	}
	else if (!game_header->author)
	{
		fprintf(stderr, "PANIC: No author specified\n");
		quit_error();
	}
	else if (!game_header->date)
	{
		fprintf(stderr, "PANIC: No copyright date speficied\n");
		quit_error();
	}
	else if (!game_header->uid)
	{
		fprintf(stderr, "PANIC: No UID speicifid\n");
		quit_error();
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
parser_check_room(room *new)
{
	assert(new);

	if (!new->name)
	{
		parser_error();
	}

	if (!new->descr)
	{
		parser_error();
	}

	if (!new->words)
	{
		parser_error();
	}
	else
	{
		if (!new->words->count)
		{
			parser_error();
		}
	}

	if (!new->aliases)
	{
		log_info_f("Room: %s (0 aliases, %i words)", new->name,
				new->words->count);
	}
	else
	{
		log_info_f("Room: %s (%i aliases, %i words)", new->name,
				new->aliases->count, new->words->count);
	}
}

/*
 * Does some sanity checks and adds the
 * room to the global room list.
 */
static void
parser_add_room(room *new)
{
	listnode *lnode;
	room *test;
	int32_t i;

	parser_check_room(new);

    if ((test = hashmap_get(game_rooms, new->name)) != NULL)
	{
		log_warn_f("There's already a room with name or alias %s", new->name);
	}

	hashmap_add(game_rooms, new->name, new, MAIN);

	// Aliases
	if (new->aliases)
	{
		if (new->aliases->first)
		{
			lnode = new->aliases->first;

			for (i = 0; i < new->aliases->count; i++)
			{
				if ((test = hashmap_get(game_rooms, lnode->data)) != NULL)
				{
					log_warn_f("There's already a room with name or alias %s", new->name);
				}

				hashmap_add(game_rooms, lnode->data, new, ALIAS);
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
	static room *new;

	assert(tokens);

	if (!new)
	{
		if ((new = calloc(1, sizeof(room))) == NULL)
		{
			perror("PANIC: Couldn't allocate memory");
			quit_error();
		}
	}

	// Empty input line
	if (!tokens->count)
	{
		if (new->words)
		{
			if (new->words->last)
			{
				if (strcmp(new->words->last->data, "\n"))
				{
					list_push(new->words, strdup("\n"));
				}
			}
		}
	}

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "#ROOM:"))
		{
			if (new->name || new->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			new->name = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#DESCR:"))
		{
			if (new->words)
			{
				parser_error();
			}

			new->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#ALIAS:"))
		{
			if (new->words)
			{
				parser_error();
			}

			if (!new->aliases)
			{
				new->aliases = list_create();
			}

			list_push(new->aliases, parser_concat(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (new->words)
			{
				if (new->words->last)
				{
					while (!strcmp(new->words->last->data, "\n"))
					{
						free(list_pop(new->words));
					}
				}
			}

			parser_add_room(new);

			is_room = 0;
			new = NULL;
		}
		else
		{
			if (!new->words)
			{
				new->words = list_create();
			}

			list_push(new->words, strdup(cur));

			for (i = 0; tokens->count > 0; i++)
			{
				list_push(new->words, strdup(list_shift(tokens)));
			}
		}
	}
}

// --------

/*
 * Checks if a scene was parsed successfull.
 *
 * new: Room to be checked
 */
static void
parser_check_scene(scene *new)
{
	assert(new);

	if (!new->name)
	{
		parser_error();
	}

	if (!new->descr)
	{
		parser_error();
	}

	if (!new->words)
	{
		parser_error();
	}
	else
	{
		if (!new->words->count)
		{
			parser_error();
		}
	}

	if (!new->next)
	{
		parser_error();
	}
	else
	{
		if (!new->next->elements)
		{
			parser_error();
		}
	}

	if (!new->aliases)
	{
		log_info_f("Scene: %s (0 aliases, %i choices, %i words)",
				new->name, new->next->elements, new->words->count);
	}
	else
	{
		log_info_f("Scene: %s (%i aliases, %i choices, %i words)",
				new->name, new->aliases->count, new->next->elements,
				new->words->count);
	}
}

/*
 * Does some sanity checks and adds the
 * scene to the global scene list.
 *
 * new: Scene to add
 */
static void
parser_add_scene(scene *new)
{
	listnode *lnode;
	scene *test;
	int32_t i;

	parser_check_scene(new);

    if ((test = hashmap_get(game_scenes, new->name)) != NULL)
	{
		log_warn_f("There's already a scene with name or alias %s", new->name);
	}

	hashmap_add(game_scenes, new->name, new, MAIN);

	// Aliases
	if (new->aliases)
	{
		if (new->aliases->first)
		{
			lnode = new->aliases->first;

			for (i = 0; i < new->aliases->count; i++)
			{
				if ((test = hashmap_get(game_scenes, lnode->data)) != NULL)
				{
					log_warn_f("There's already a scene with name or alias %s", new->name);
				}

				hashmap_add(game_scenes, lnode->data, new, ALIAS);
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
	static scene *new;

	assert(tokens);

	if (!new)
	{
		if ((new = calloc(1, sizeof(scene))) == NULL)
		{
			perror("PANIC: Couldn't allocate memory");
			quit_error();
		}
	}

	// Empty input line
	if (!tokens->count)
	{
		if (new->words)
		{
			if (new->words->last)
			{
				if (strcmp(new->words->last->data, "\n"))
				{
					list_push(new->words, strdup("\n"));
				}
			}
		}
	}

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "#SCENE:"))
		{
			if (new->name || new->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			new->name = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#DESCR:"))
		{
			if (new->words)
			{
				parser_error();
			}

			new->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "#ALIAS:"))
		{
			if (new->words)
			{
				parser_error();
			}

			if (!new->aliases)
			{
				new->aliases = list_create();
			}

			list_push(new->aliases, parser_concat(tokens));
		}
		else if (!strcmp(cur, "#ROOM:"))
		{
			if (new->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			new->room = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "#NEXT:"))
		{
			if (new->words)
			{
				parser_error();
			}

			if (!new->next)
			{
				new->next = darray_create();
			}

			darray_push(new->next, parser_concat(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (new->words)
			{
				if (new->words->last)
				{
					while (!strcmp(new->words->last->data, "\n"))
					{
						free(list_pop(new->words));
					}
				}
			}

			parser_add_scene(new);

			is_scene = 0;
			new = NULL;
		}
		else
		{
			if (!new->words)
			{
				new->words = list_create();
			}

			list_push(new->words, strdup(cur));

			for (i = 0; tokens->count > 0; i++)
			{
				list_push(new->words, strdup(list_shift(tokens)));
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
		printf("PANIC: %s doesn't exists\n", file);
		quit_error();
	}

	if (!S_ISREG(sb.st_mode))
	{
		printf("PANIC: %s ist not a regular file\n", file);
		quit_error();
	}

	if ((game = fopen(file, "r")) == NULL)
	{
		perror("PANIC: Couldn't open game file");
		quit_error();
	}

	line = NULL;
	linecap = 0;

	// Header is always the first section
	is_header = 1;

    while ((linelen = getline(&line, &linecap, game)) > 0)
	{
		count++;
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
}

