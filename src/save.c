/*
 * save.c
 * ------
 *
 * Code to save the game state into a file and
 * load it back at a later time.
 */

#define _WITH_GETLINE

#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "curses.h"
#include "game.h"
#include "log.h"
#include "main.h"
#include "util.h"

#include "data/hashmap.h"
#include "data/list.h"

// --------

static char savedir[PATH_MAX];

// --------

/*********************************************************************
 *                                                                   *
 *                        Support Functions                          *
 *                                                                   *
 *********************************************************************/

/*
 * Resets the global state, as if the engine
 * was just started and the game loaded.
 */
static void
save_reset_state(void)
{
	game_glossary_s *glossary;
	game_room_s *room;
	game_scene_s *scene;
	list *tmp;

	// Global state
	current_scene = NULL;
	game_end = FALSE;

	// Glossary
	tmp = hashmap_to_list(game_glossary);

	while (tmp->count)
	{
		glossary = list_shift(tmp);

		if (glossary->mentioned)
		{
			glossary->mentioned = FALSE;
		}
	}

	game_stats->glossary_mentioned = 0;
	free(tmp);

	// Rooms
	tmp = hashmap_to_list(game_rooms);

	while (tmp->count)
	{
		room = list_shift(tmp);

		if (room->mentioned)
		{
			room->mentioned = FALSE;
		}

		if (room->visited)
		{
			room->visited = FALSE;
		}
	}

	game_stats->rooms_visited = 0;
	free(tmp);

	// Scenes
	tmp = hashmap_to_list(game_scenes);

	while (tmp->count)
	{
		scene = list_shift(tmp);

		if (scene->visited)
		{
			scene->visited = FALSE;
		}
	}

	game_stats->scenes_visited = 0;
	free(tmp);
}

// --------

/*********************************************************************
 *                                                                   *
 *                          Public Interface                         *
 *                                                                   *
 *********************************************************************/

void
save_init(const char *homedir)
{
	struct stat sb;

	assert(game_header);
	assert(homedir);

	log_info("Initializing savegames");
	snprintf(savedir, sizeof(savedir), "%s/%s/%s", homedir, "save", game_header->uid);

	if ((stat(savedir, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			quit_error("Savedir is not a directory\n");
			exit(1);
		}
	}
	else
	{
		util_rmkdir(savedir);
	}
}

void
save_list(void)
{
	DIR *dir;
	char buf[PATH_MAX];
	struct dirent *cur;
	struct stat sb;
	uint16_t count;

	if ((dir = opendir(savedir)) == NULL)
	{
		quit_error("Couldn't open directory");
	}

	count = 0;

	curses_text(COLOR_NORM, "Saves:\n");
	curses_text(COLOR_NORM, "------\n");

	while ((cur = readdir(dir)) != NULL)
	{
		snprintf(buf, sizeof(buf), "%s/%s", savedir, cur->d_name);
		stat(buf, &sb);

		if (S_ISREG(sb.st_mode))
		{
			if (strlen(cur->d_name) > strlen(".sav"))
			{
				if (!strcmp(&cur->d_name[strlen(cur->d_name) - strlen(".sav")], ".sav"))
				{
					snprintf(buf, strlen(cur->d_name) - strlen(".sav") + 1, "%s", cur->d_name);
					curses_text(COLOR_NORM, "%s\n", buf);
					count++;
				}
			}
		}
	}

	log_info_f("Listed %i savegames", count);
}

int32_t
save_read(char *name)
{
	FILE *save;
	boolean glossary_mentioned;
	boolean header;
	boolean rooms_mentioned;
	boolean rooms_seen;
	boolean scenes_visited;
	game_glossary_s *glossary;
	char *cur;
	char *line;
	char savefile[PATH_MAX];
	char savename[PATH_MAX];
	char *token;
	game_room_s *room;
	game_scene_s *scene;
	size_t linecap;
	ssize_t linelen;
	struct stat sb;

	assert(name);
	assert(savedir);

	// Construct name
	if (strlen(name) > strlen(".sav"))
	{
		if (!strcmp(&name[strlen(name) - strlen(".sav")], ".sav"))
		{
			strncpy(savename, name, sizeof(savename));
		}
		else
		{
			snprintf(savename, sizeof(savename), "%s.sav", name);
		}
	}
	else
	{
		snprintf(savename, sizeof(savename), "%s.sav", name);
	}

	snprintf(savefile, sizeof(savefile), "%s/%s", savedir, savename);
	log_info_f("Loading game from %s", savefile);

	// Existing?
	if ((stat(savefile, &sb)) == 0)
	{
		if (!S_ISREG(sb.st_mode))
		{
			curses_text(COLOR_NORM, "Savegame %s doesn't exists\n", name);

			return -1;
		}
	}
	else
	{
			curses_text(COLOR_NORM, "Savegame %s doesn't exists\n", name);

			return -1;
	}

	// Load it
	glossary_mentioned = FALSE;
	header = TRUE;
	rooms_mentioned = FALSE;
	rooms_seen = FALSE;
	scenes_visited = FALSE;

	line = NULL;
	linecap = 0;

	current_scene = NULL;

	if ((save = fopen(savefile, "r")) == NULL)
	{
		quit_error("Couldn't load savegame");
	}

	// Reset state
	save_reset_state();

    while ((linelen = getline(&line, &linecap, save)) > 0)
	{
		cur = line;

		// Empty line ends block
		if (linelen == 1)
		{
			glossary_mentioned = FALSE;
			header = FALSE;
			rooms_mentioned = FALSE;
			rooms_seen = FALSE;
			scenes_visited = FALSE;

			continue;
		}

		// Remove newline character
		cur[strlen(cur) - 1] = '\0';

		// New block
		if (!(glossary_mentioned || header || rooms_mentioned || rooms_seen || scenes_visited))
		{
			if (!strncmp(cur, "#GLOSSARY_MENTIONED:", strlen("GLOSSARY_MENTIONED:")))
			{
				glossary_mentioned = TRUE;
			}

			if (!strncmp(cur, "#ROOMS_MENTIONED:", strlen("#ROOMS_MENTIONED:")))
			{
				rooms_mentioned = TRUE;
			}

			if (!strncmp(cur, "#ROOMS_SEEN:", strlen("#ROOMS_SEEN:")))
			{
				rooms_seen = TRUE;
			}

			if (!strncmp(cur, "#SCENES_VISITED:", strlen("#SCENES_VISITED:")))
			{
				scenes_visited = TRUE;
			}

			continue;
		}

		// Glossary
		if (glossary_mentioned)
		{
			if ((glossary = hashmap_get(game_glossary, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			glossary->mentioned = TRUE;
		}

		// Header
		if (header)
		{
			token = strsep(&cur, " ");

			if (!strcmp(token, "#UID:"))
			{
				if (strcmp(cur, game_header->uid))
				{
					curses_text(COLOR_NORM, "Savegame from another game\n");

					return -1;
				}
			}

			if (!strcmp(token, "#CURSCENE:"))
			{
				if ((scene = hashmap_get(game_scenes, cur)) == NULL)
				{
					quit_error("Savegame is broken\n");
				}

				current_scene = scene;
			}

			if (!strcmp(token, "#GAMEEND"))
			{
				game_end = TRUE;
			}
		}

		// Rooms mentioned
		if (rooms_mentioned)
		{
			if ((room = hashmap_get(game_rooms, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			room->mentioned = TRUE;
		}

		// Rooms visited
		if (rooms_seen)
		{
			if ((room = hashmap_get(game_rooms, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			room->visited = TRUE;
		}

		// Scenes visited
		if (scenes_visited)
		{
			if ((scene = hashmap_get(game_scenes, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			scene->visited = TRUE;
		}
	}

	free(line);

	return 0;
}

void
save_write(char *name)
{
    FILE *save;
	char savefile[PATH_MAX];
	char savename[PATH_MAX];
	game_glossary_s *glossary;
	game_room_s *room;
	game_scene_s *scene;
	list *tmp;

	assert(name);
	assert(savedir);

	// Construct name
	if (strlen(name) > strlen(".sav"))
	{
		if (!strcmp(&name[strlen(name) - strlen(".sav")], ".sav"))
		{
			strncpy(savename, name, sizeof(savename));
		}
		else
		{
			snprintf(savename, sizeof(savename), "%s.sav", name);
		}
	}
	else
	{
		snprintf(savename, sizeof(savename), "%s.sav", name);
	}

	snprintf(savefile, sizeof(savefile), "%s/%s", savedir, savename);
	log_info_f("Saving game to %s", savefile);

	// Open file
	if ((save = fopen(savefile, "w")) == NULL)
	{
		quit_error("Couldn't open file");
	}

	// Write metadata
	fwrite("#UID: ", strlen("#UID: "), 1, save);
	fwrite(game_header->uid, strlen(game_header->uid), 1, save);
	fwrite("\n ", strlen("\n"), 1, save);

	if (current_scene)
	{
		fwrite("#CURSCENE: ", strlen("#CURSCENE: "), 1, save);
		fwrite(current_scene->name, strlen(current_scene->name), 1, save);
		fwrite("\n ", strlen("\n"), 1, save);
	}

	if (game_end)
	{
		fwrite("#GAMEEND\n", strlen("#GAMEEND\n"), 1, save);
	}

	fwrite("\n ", strlen("\n"), 1, save);

	// Write mentioned glossar entries
	fwrite("#GLOSSAR_MENTIONED:\n", strlen("#GLOSSAR_MENTIONED:\n"), 1, save);

	tmp = hashmap_to_list(game_glossary);

	while (tmp->count)
	{
		glossary = list_shift(tmp);

		if (glossary->mentioned)
		{
			fwrite(glossary->name, strlen(glossary->name), 1, save);
			fwrite("\n", strlen("\n"), 1, save);
		}
	}

	fwrite("\n", strlen("\n"), 1, save);
	list_destroy(tmp, NULL);

	// Write mentioned rooms
	fwrite("#ROOMS_MENTIONED:\n", strlen("#ROOMS_MENTIONED:\n"), 1, save);

	tmp = hashmap_to_list(game_rooms);

	while (tmp->count)
	{
		room = list_shift(tmp);

		if (room->mentioned)
		{
			fwrite(room->name, strlen(room->name), 1, save);
			fwrite("\n", strlen("\n"), 1, save);
		}
	}

	fwrite("\n", strlen("\n"), 1, save);
	list_destroy(tmp, NULL);

	// Write seen rooms
	fwrite("#ROOMS_SEEN:\n", strlen("#ROOMS_SEEN:\n"), 1, save);

	tmp = hashmap_to_list(game_rooms);

	while (tmp->count)
	{
		room = list_shift(tmp);

		if (room->visited)
		{
			fwrite(room->name, strlen(room->name), 1, save);
			fwrite("\n", strlen("\n"), 1, save);
		}
	}

	fwrite("\n", strlen("\n"), 1, save);
	list_destroy(tmp, NULL);

	// Write visited scenes
	fwrite("#SCENES_VISITED:\n", strlen("#SCENES_VISITED:\n"), 1, save);

	tmp = hashmap_to_list(game_scenes);

	while (tmp->count)
	{
		scene = list_shift(tmp);

		if (scene->visited)
		{
			fwrite(scene->name, strlen(scene->name), 1, save);
			fwrite("\n", strlen("\n"), 1, save);
		}
	}

	fwrite("\n", strlen("\n"), 1, save);
	list_destroy(tmp, NULL);

	// Close
	fflush(save);
	fclose(save);
}

