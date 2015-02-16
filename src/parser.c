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

// We are parsing the header
static uint8_t is_header;

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
		strlcat(string, " ", len);
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
	log_info_f("Name:   %s", game_header->game);
	log_info_f("Author: %s", game_header->author);
	log_info_f("Date:   %s", game_header->date);
	log_info_f("UID:    %s", game_header->uid);
}

/*
 * Parses the game header.
 *
 * line: Line to parse
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

			game_header->uid = list_shift(tokens);
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

void
parser_game(const char *file)
{
	FILE *game;
	char *line;
	size_t linecap;
	ssize_t linelen;
	struct stat sb;
	list *tokens;

	assert(file);

	if ((stat(file, &sb)) != 0)
	{
		if (!S_ISREG(sb.st_mode))
		{
			printf("PANIC: %s ist not a regular file\n", file);
			quit_error();
		}
		else
		{
			printf("PANIC: %s doesn't exists\n", file);
			quit_error();
		}
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

		// List's empty -> empty line
		if (!tokens->count)
		{
			list_destroy(tokens, NULL);
			continue;
		}

		// Header
		if (is_header)
		{
			parser_header(tokens);
		}

		list_destroy(tokens, NULL);
	}

	free(line);
}

