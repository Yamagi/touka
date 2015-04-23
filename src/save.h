/*
 * Code to save the game state into a file and
 * load it back at a later time. All available
 * savegame files can be listed, too.
 *
 * Since the save games are written in plain
 * text and parsed at load, they shouled be
 * portable between engine versions and even
 * operating systems or platforms.
 */

#ifndef SAVE_H_
#define SAVE_H_

// --------

#include <stdint.h>

// --------

/*
 * Initializes the savegame system.
 *
 * homedir: Home directory of the engine
 */
void save_init(const char *homedir);

/*
 * Prints a list of all available savegames.
 */
void save_list(void);

/*
 * Load a savegame. If successfull 0 is
 * returned, otherwise -1.
 *
 * name: Name of the savegame
 */
boolean save_read(char *name);

/*
 * Saves the game.
 *
 * name: Name of the savegame
 */
void save_write(char *name);

// --------

#endif // SAVE_H_
