#ifndef SAVE_H_
#define SAVE_H_

#include <stdint.h>

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
uint32_t save_read(char *name);

/*
 * Saves the game.
 *
 * name: Name of the savegame
 */
void save_write(char *name);

#endif // SAVE_H_

