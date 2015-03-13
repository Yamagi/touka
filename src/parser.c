/*
 * parser.c
 * --------
 *
 * Parses the game file. Please note that this hand
 * written implementation is not that robust if you
 * might expect. if an * error is detected, it just
 * bails out.
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
#include "quit.h"

#include "data/list.h"
#include "i18n/i18n.h"

// --------

// What we are parsing?
static boolean is_glossary;
static boolean is_header;
static boolean is_room;
static boolean is_scene;

// Lines parsed so far
static int32_t count;

// --------

/*********************************************************************
 *                                                                   *
 *                        Support Functions                          *
 *                                                                   *
 *********************************************************************/

/*
 * Concanates elements of a linked list
 * filled with strings into one string.
 * The string is allocated with malloc(),
 * so the caller need to free() it.
 *
 * tokens: List to concanate
 */
static char
*parser_concat(list *tokens)
{
	char *cur;
	char *string;
	size_t len, oldlen;

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
			quit_error(POUTOFMEM);
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
 * This function prints a more or less
 * helpfull error message and bails out.
 */
static void
parser_error(void)
{
	log_error_f("Parser error in line %i\n", count);

	quit_error(PPARSERERR);
}

/*
 * Tokenize a line into a linked list with
 * the single words. If the first character
 * is the comment symbol (for us the line
 * doesn't exists and should be skipped)
 * NULL is returned. Otherwise the list.
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
	uint16_t i;

	assert(line);

	// Line is comment
	if (line[0] == '#')
	{
		return NULL;
	}

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
		if (work[i] == '#')
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

/*********************************************************************
 *                                                                   *
 *                       Game Header Parsing                         *
 *                                                                   *
 *********************************************************************/

/*
 * Checks if the  game header was parsed successfull.
 */
static void
parser_check_header(void)
{
	if (!game_header->game)
	{
		log_error("No game name specified");
		quit_error(PINVGAMEHEADER);
	}
	else if (!game_header->author)
	{
		log_error("No author specified");
		quit_error(PINVGAMEHEADER);
	}
	else if (!game_header->date)
	{
		log_error("No copyright date speficied");
		quit_error(PINVGAMEHEADER);
	}
	else if (!game_header->uid)
	{
		log_error("No UID specified");
		quit_error(PINVGAMEHEADER);
	}
	else if (!game_header->first_scene)
	{
		log_error("No starting scene specified");
		quit_error(PINVGAMEHEADER);
	}

	log_info_f("%s:", i18n_parser_gamespecs);
	log_info_f("%s: %s", i18n_info_game, game_header->game);
	log_info_f("%s: %s", i18n_info_author, game_header->author);
	log_info_f("%s: %s", i18n_info_date, game_header->date);
	log_info_f("%s: %s", i18n_info_uid, game_header->uid);
}

/*
 * Parses the game header into the global struct
 * 'game_header'. Called for every line of the
 * header.
 *
 * tokens: Tokenized line to parse
 */
static void
parser_header(list *tokens)
{
	char *cur;

	assert(tokens);

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "%GAME:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->game = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%AUTHOR:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->author = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%DATE:"))
		{
			if (tokens->count < 1)
			{
				parser_error();
			}

			game_header->date = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%UID:"))
		{
			if (tokens->count != 1)
			{
				parser_error();
			}

			game_header->uid = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "%START:"))
		{
			if (tokens->count != 1)
			{
				parser_error();
			}

			game_header->first_scene = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "%PROMPT:"))
		{
			if (curses_prompt)
			{
				free(curses_prompt);
			}

			game_header->prompt = parser_concat(tokens);

		}
		else if (!strcmp(cur, "----"))
		{
			if (tokens->count != 0)
			{
				parser_error();
			}

			parser_check_header();
			is_header = FALSE;
		}
		else
		{
			parser_error();
		}
	}
}

// --------

/*********************************************************************
 *                                                                   *
 *                         Glossar Parsing                           *
 *                                                                   *
 *********************************************************************/

/*
 * Checks if a glossary entry was parsed successfull.
 * If not, the parser bails out with an error.
 *
 * entry: Entry to be checked
 */
static void
parser_check_glossary(game_glossary_s *entry)
{
	assert(entry);

	if (!entry->name)
	{
		parser_error();
	}

	if (!entry->descr)
	{
		parser_error();
	}

	if (!entry->words)
	{
		parser_error();
	}
	else
	{
		if (!entry->words->count)
		{
			parser_error();
		}
	}

	if (!entry->aliases)
	{
		log_info_f("%s: %s (0 %s, %i %s)", i18n_parser_glossaryentry,
				entry->name, i18n_aliases, entry->words->count, i18n_words);
	}
	else
	{
		log_info_f("%s: %s (%i %s, %i %s)", i18n_parser_glossaryentry,
				entry->name, entry->aliases->count, i18n_aliases, entry->words->count, i18n_words);
	}
}

/*
 * Adds an entry to the global glossary. If an
 * entry with the same name or alias is already
 * present, a warning is logged.
 *
 * entry: Entry to add
 */
static void
parser_add_glossary(game_glossary_s *entry)
{
	listnode *lnode;
	game_glossary_s *test;
	uint16_t i;

	parser_check_glossary(entry);

	if ((test = hashmap_get(game_glossary, entry->name)) != NULL)
	{
		log_warn_f("%s %s", i18n_parser_glossarytwice, entry->name);
	}

	hashmap_add(game_glossary, entry->name, entry, FALSE);
	game_stats->glossary_total++;

	// Aliases
	if (entry->aliases)
	{
		if (entry->aliases->first)
		{
			lnode = entry->aliases->first;

			for (i = 0; i  < entry->aliases->count; i++)
			{
				if ((test = hashmap_get(game_glossary, lnode->data)) != NULL)
				{
					log_warn_f("%s %s", i18n_parser_glossarytwice,  entry->name);
				}
			}

			hashmap_add(game_glossary, lnode->data, entry, TRUE);
			lnode = lnode->next;
		}
	}
}

/*
 * Parses a glossary entry into a 'game_glossary_s' struct.
 * If necessary the struct is created. This function is
 * called for every line of the entry and if it's complete
 * it's automatically added to the global 'game_glossar'
 * hashmap.
 *
 * tokens: Tokenized line to parse
 */
static void
parser_glossary(list *tokens)
{
	char *cur;
	static game_glossary_s *entry;
	uint16_t i;

	assert(tokens);

	if (!entry)
	{
		if ((entry = calloc(1, sizeof(game_glossary_s))) == NULL)
		{
			quit_error(POUTOFMEM);
		}
	}

	// Empty input line
	if (!tokens->count)
	{
		if (entry->words)
		{
			if (entry->words->last)
			{
				if (strcmp(entry->words->last->data, "\n"))
				{
					list_push(entry->words, strdup("\n"));
				}
			}
		}
	}

	while (tokens->count > 0)
	{
		cur = list_shift(tokens);

		if (!strcmp(cur, "%GLOSSARY:"))
		{
			if (entry->name || entry->words)
			{
				parser_error();
			}

			if (tokens->count != 1)
			{
				parser_error();
			}

			entry->name = strdup(list_shift(tokens));
		}
		else if (!strcmp(cur, "%DESCR:"))
		{
			if (entry->words)
			{
				parser_error();
			}

			entry->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%ALIAS:"))
		{
			if (entry->words)
			{
				parser_error();
			}

			if (!entry->aliases)
			{
				entry->aliases = list_create();
			}

			list_push(entry->aliases, parser_concat(tokens));
		}
		else if (!strcmp(cur, "----"))
		{
			if (entry->words)
			{
				if (entry->words->last)
				{
					while (!strcmp(entry->words->last->data, "\n"))
					{
						free(list_pop(entry->words));
					}
				}
			}

			parser_add_glossary(entry);

			is_glossary = FALSE;
			entry = NULL;
		}
		else
		{
			if (!entry->words)
			{
				entry->words = list_create();
			}

			list_push(entry->words, strdup(cur));

			for (i = 0; tokens->count; i++)
			{
				list_push(entry->words, strdup(list_shift(tokens)));
			}
		}
	}
}

// --------

/*********************************************************************
 *                                                                   *
 *                           Room Parsing                            *
 *                                                                   *
 *********************************************************************/

/*
 * Checks if a room was parsed successfull.
 * If not the parser bails out with an error.
 *
 * room: Room to be checked
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
		log_info_f("%s: %s (0 %s, %i %s)", i18n_room, room->name,
				i18n_aliases, room->words->count, i18n_words);
	}
	else
	{
		log_info_f("%s: %s (%i %s, %i %s)", i18n_room, room->name,
				room->aliases->count, i18n_aliases, room->words->count,
				i18n_words);
	}
}

/*
 * Add the room to the global 'game_rooms' hashmap.
 * If there's already an entry with the same name or
 * alias a warning is logged.
 *
 * room: Room to add
 */
static void
parser_add_room(game_room_s *room)
{
	game_room_s *test;
	listnode *lnode;
	uint16_t i;

	parser_check_room(room);

    if ((test = hashmap_get(game_rooms, room->name)) != NULL)
	{
		log_warn_f("%s %s", i18n_parser_roomtwice, room->name);
	}

	hashmap_add(game_rooms, room->name, room, FALSE);
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
					log_warn_f("%s %s", i18n_parser_roomtwice, room->name);
				}

				hashmap_add(game_rooms, lnode->data, room, TRUE);
				lnode = lnode->next;
			}
		}
	}
}

/*
 * Parses a room into a 'game_room_s' struct. The
 * struct is created if necessary. This function
 * is called for every line of the room. When the
 * room is completly parsed, it's automatically
 * added to the global 'game_rooms' hashmap.
 *
 * tokens: Tokenized line to parse
 */
static void
parser_room(list *tokens)
{
	char *cur;
	static game_room_s *room;
	uint16_t i;

	assert(tokens);

	if (!room)
	{
		if ((room = calloc(1, sizeof(game_room_s))) == NULL)
		{
			quit_error(POUTOFMEM);
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

		if (!strcmp(cur, "%ROOM:"))
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
		else if (!strcmp(cur, "%DESCR:"))
		{
			if (room->words)
			{
				parser_error();
			}

			room->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%ALIAS:"))
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

			is_room = FALSE;
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

/*********************************************************************
 *                                                                   *
 *                          Scene Parsing                            *
 *                                                                   *
 *********************************************************************/

/*
 * Checks if a scene was parsed successfull.
 * If not, the parser bails out.
 *
 * scene: Scene to be checked
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
		log_info_f("%s: %s (0 %s, %i %s, %i %s)", i18n_scene, scene->name,
				i18n_aliases, scene->next->elements, i18n_choices, scene->words->count,
				i18n_words);
	}
	else
	{
		log_info_f("%s: %s (%i %s, %i %s, %i %s)", i18n_scene, scene->name,
				scene->aliases->count, i18n_aliases, scene->next->elements,
				i18n_choices, scene->words->count, i18n_words);
	}
}

/*
 * Adds a new scene to the global 'game_scenes' hashmap.
 * If an entry with the same name or alias is already
 * present, a warnig is logged.
 *
 * scene: Scene to add
 */
static void
parser_add_scene(game_scene_s *scene)
{
	game_scene_s *test;
	listnode *lnode;
	uint16_t i;

	parser_check_scene(scene);

    if ((test = hashmap_get(game_scenes, scene->name)) != NULL)
	{
		log_warn_f("%s %s", i18n_parser_scenetwice, scene->name);
	}

	hashmap_add(game_scenes, scene->name, scene, FALSE);
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
					log_warn_f("%s %s", i18n_parser_scenetwice, scene->name);
				}

				hashmap_add(game_scenes, lnode->data, scene, TRUE);
				lnode = lnode->next;
			}
		}
	}
}

/*
 * Parses a scene into a 'game_scene_s' struct. If
 * necessary the struct is created. This function is
 * called for every line of the scene. After it was
 * parsed it's added the global 'game_scenes' hashmap
 * automatically.
 *
 * tokens: Tokenized line to parse
 */
static void
parser_scene(list *tokens)
{
	char *cur;
	static game_scene_s *scene;
	uint16_t i;

	assert(tokens);

	if (!scene)
	{
		if ((scene = calloc(1, sizeof(game_scene_s))) == NULL)
		{
			quit_error(POUTOFMEM);
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

		if (!strcmp(cur, "%SCENE:"))
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
		else if (!strcmp(cur, "%DESCR:"))
		{
			if (scene->words)
			{
				parser_error();
			}

			scene->descr = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%PROMPT:"))
		{
			if (scene->words)
			{
				parser_error();
			}

            scene->prompt = parser_concat(tokens);
		}
		else if (!strcmp(cur, "%ALIAS:"))
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
		else if (!strcmp(cur, "%ROOM:"))
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
		else if (!strcmp(cur, "%NEXT:"))
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

			is_scene = FALSE;
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

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

void
parser_game(const char *file)
{
	FILE *game;
	char *line;
	char *tmp;
	list *tokens;
	size_t linecap;
	ssize_t linelen;
	struct stat sb;

	assert(file);

	if ((stat(file, &sb)) != 0)
	{
		quit_error(PFILENOTEXIST);
	}

	if (!S_ISREG(sb.st_mode))
	{
		quit_error(PNOTAFILE);
	}

	if ((game = fopen(file, "r")) == NULL)
	{
		quit_error(PCOULDNTOPENFILE);
	}

	log_info_f("%s: %s", i18n_parser_parsingfile, file);

	line = NULL;
	linecap = 0;

	// Header is always the first section
	is_header = TRUE;

    while ((linelen = getline(&line, &linecap, game)) > 0)
	{
		count++;

		if ((tokens = parser_tokenize(line)) == NULL)
		{
			continue;
		}

		// What we are parsing?
		if (!(is_glossary || is_header || is_room || is_scene))
		{
			if (tokens->count > 0)
			{
				tmp = list_shift(tokens);

				if (!strcmp(tmp, "%GLOSSARY:"))
				{
					is_glossary = TRUE;
					list_unshift(tokens, tmp);
				}
				else if (!strcmp(tmp, "%ROOM:"))
				{
					is_room = TRUE;
					list_unshift(tokens, tmp);
				}
				else if (!strcmp(tmp, "%SCENE:"))
				{
					is_scene = TRUE;
					list_unshift(tokens, tmp);
				}
				else
				{
					parser_error();
				}
			}
			else
			{
				list_destroy(tokens, NULL);
				continue;
			}
		}

		// Glossary
		if (is_glossary)
		{
			parser_glossary(tokens);
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
	log_info_f("%s: %i", i18n_parser_linesparsed, count);
}

