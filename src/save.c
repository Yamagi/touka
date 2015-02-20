#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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

	snprintf(savedir, sizeof(savedir), "%s/%s/%s", homedir, game_header->uid, "save");

 	if ((stat(savedir, &sb)) == 0)
	{
		if (!S_ISDIR(sb.st_mode))
		{
			printf("PANIC: %s is not a directory\n", savedir);
			exit(1);
		}
	}
	else
	{
		util_rmkdir(savedir);
	}
}

void
save_write(char *name)
{
    FILE *save;
	char savefile[PATH_MAX];
	char savename[PATH_MAX];
	list *tmp;
	room *r;
	scene *s;

	assert(name);
	assert(savedir);

	// Construct name
	if (strlen(name) > strlen(".sav"))
	{
		if (!strcmp(&name[strlen(name) - strlen(".sav")], ".sav"))
		{
			strlcpy(savename, name, sizeof(savename));
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
		perror("PANIC: Couldn't open file");
		quit_error();
	}

	// Write metadata
	fwrite("#UID: ", strlen("#UID: "), 1, save);
	fwrite(game_header->uid, strlen(game_header->uid), 1, save);
	fwrite("\n\n ", strlen("\n\n"), 1, save);

	// Write mentioned rooms
	fwrite("#ROOMS_MENTIONED:\n", strlen("#ROOMS_MENTIONED:\n"), 1, save);

	tmp = hashmap_to_list(game_rooms); 

	while (tmp->count)
	{
		r = list_shift(tmp);

		if (r->mentioned)
		{
			fwrite(r->name, strlen(r->name), 1, save);
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
		r = list_shift(tmp);

		if (r->seen)
		{
			fwrite(r->name, strlen(r->name), 1, save);
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
		s = list_shift(tmp);

		if (s->visited)
		{
			fwrite(s->name, strlen(s->name), 1, save);
			fwrite("\n", strlen("\n"), 1, save);
		}
	}

	fwrite("\n", strlen("\n"), 1, save);
	list_destroy(tmp, NULL);

	// Close
	fflush(save);
	fclose(save);
}
