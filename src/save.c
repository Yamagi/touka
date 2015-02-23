/*
 * save.c
 * ------
 *
 * Code to save the game state into a file and
 * load it back at a later time. Also available
 * savegame files can be listed.
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
#include "data/hashmap.h"
#include "data/list.h"
#include "log.h"
#include "main.h"
#include "util.h"

// --------

static char savedir[PATH_MAX];

// --------

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
			quit_error("Save dir is not a directory\n");
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
	char *cur;
	char *line;
	char savefile[PATH_MAX];
	char savename[PATH_MAX];
	char *token;
	int8_t header;
	int8_t rooms_mentioned;
	int8_t rooms_seen;
	int8_t scenes_visited;
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
	header = 1;
	rooms_mentioned = 0;
	rooms_seen = 0;
	scenes_visited = 0;

	line = NULL;
	linecap = 0;

	current_scene = NULL;

	if ((save = fopen(savefile, "r")) == NULL)
	{
		quit_error("Couldn't load savegame");
	}

    while ((linelen = getline(&line, &linecap, save)) > 0)
	{
		cur = line;

		// Empty cur ends block
		if (linelen == 1)
		{
			header = 0;
			rooms_mentioned = 0;
			rooms_seen = 0;
			scenes_visited = 0;

			continue;
		}

		// Remove newcur
		cur[strlen(cur) - 1] = '\0';

		// New block
		if (!(header || rooms_mentioned || rooms_seen || scenes_visited))
		{
			if (!strncmp(cur, "#ROOMS_MENTIONED:", strlen("#ROOMS_MENTIONED:")))
			{
				rooms_mentioned = 1;
			}

			if (!strncmp(cur, "#ROOMS_SEEN:", strlen("#ROOMS_SEEN:")))
			{
				rooms_seen = 1;
			}

			if (!strncmp(cur, "#SCENES_VISITED:", strlen("#SCENES_VISITED:")))
			{
				scenes_visited = 1;
			}

			continue;
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
		}

		// Rooms mentioned
		if (rooms_mentioned)
		{
			if ((room = hashmap_get(game_rooms, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			room->mentioned = 1;
		}

		// Rooms visited
		if (rooms_seen)
		{
			if ((room = hashmap_get(game_rooms, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			room->visited = 1;
		}

		// Scenes visited
		if (scenes_visited)
		{
			if ((scene = hashmap_get(game_scenes, cur)) == NULL)
			{
				quit_error("Savegame is broken\n");
			}

			scene->visited = 1;
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
	list *tmp;
	game_room_s *room;
	game_scene_s *scene;

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

	// open file
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

	fwrite("\n ", strlen("\n"), 1, save);

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

