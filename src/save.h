#ifndef SAVE_H_
#define SAVE_H_

/*
 * Initializes the savegame system.
 *
 * homedir: Home directory of the engine
 */
void save_init(const char *homedir);

/*
 * Saves the game.
 *
 * name: Name of the savegame
 */
void save_write(char *name);

#endif // SAVE_H_

