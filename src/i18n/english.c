/*
 * english.c
 * ---------
 *
 * English translation.
 */

#include "i18n.h"

// ---------

// Objects
const char *i18n_aliases = "aliases";
const char *i18n_choices = "choices";
const char *i18n_words = "words";

const char *i18n_entry = "Entry";
const char *i18n_room = "Room";
const char *i18n_scene = "Scene";

// ---------

// Table headers
const char *i18n_head_description = "Description";
const char *i18n_head_saves = "Saves";
const char *i18n_head_state = "State";

// ---------

// Curses TUI
const char *i18n_curses_8colorsonly = "Terminal cannot change colors, using unfancy standard colors";
const char *i18n_curses_init = "Initializing curses";
const char *i18n_curses_newtermsize = "New terminal size is";
const char *i18n_curses_quit = "Shutdown curses";
const char *i18n_curses_termresize = "Terminal resize detected";
const char *i18n_curses_termsize = "Terminal size is";
const char *i18n_curses_userinput = "User input";

// ---------

// Link matching
const char *i18n_link_linebreak = "Line break in link";
const char *i18n_link_didntmatch = "Link didn't match anything";
const char *i18n_link_nestedlink = "Nested link detected";
const char *i18n_link_openatend = "Link still open at end of description";
const char *i18n_link_notopened = "Closing an unopened link";

// ---------

// Glossary
const char *i18n_glossary_entrieslisted = "Entries listed";
const char *i18n_glossary_notfound = "Entry not found";

// ---------

// Room
const char *i18n_room_mentioned = "Room was mentioned but not visited";
const char *i18n_room_notfound = "Room not found";
const char *i18n_room_roomslisted = "Rooms listed";

// ---------

// Scene
const char *i18n_scene_choice = "Make your choice";
const char *i18n_scene_firstnotfound = "First scene doesn't exists";
const char *i18n_scene_invalidchoice = "Invalid choice";
const char *i18n_scene_listed = "Scenes listed";
const char *i18n_scene_next = "Advancing to the next scene";
const char *i18n_scene_nochoice = "No choice possible";
const char *i18n_scene_notfound= "No such scene";
const char *i18n_scene_play = "Playing scene";
const char *i18n_scene_playerschoice = "Player's choice";

// ---------

// Startscreen
const char *i18n_start_author = "Written by";
const char *i18n_start_stats = "This game has";
const char *i18n_start_welcome = "Welcome to";

const char *i18n_start_glossary = "glossary entries";
const char *i18n_start_rooms = "rooms";
const char *i18n_start_scenes = "scenes";

const char *i18n_start_help1 = "Type";
const char *i18n_start_help2 = "for help, or";
const char *i18n_start_help3 = "to start the game";

// ---------

// Endscreen
const char *i18n_end_congratulations = "Congratulations! You have successfull completed";
const char *i18n_end_glossaryentriesseen = "glossary entries seen";
const char *i18n_end_roomsvisited = "rooms visited";
const char *i18n_end_scenesplayed = "scenes played";
const char *i18n_end_stats = "Your track record is";
const char *i18n_end_statusbar = "Game is finished";

// ---------

// Input
const char *i18n_input_command = "Command";
const char *i18n_input_cmdslisted = "Commands listed";
const char *i18n_input_cmdnotfound ="Command not found";
const char *i18n_input_init = "Initializing input";
const char *i18n_input_quit = "Shutting down input";

// ---------

// Version
const char *i18n_version_buildon = "This binary was build on";
const char *i18n_version_thisis = "This is";

// ---------

// 'glossary' Command
const char *i18n_cmdglossary = "glossary";
const char *i18n_cmdglossaryhelp = "Prints a glossay entry or a list of all entries";
const char *i18n_cmdglossaryshort = "g";

// 'help' Command
const char *i18n_cmdhelp = "help";
const char *i18n_cmdhelphelp = "Prints this help";
const char *i18n_cmdhelpshort = "h";

// 'info' Command
const char *i18n_cmdinfo = "info";
const char *i18n_cmdinfohelp = "Prints the games metadata block";
const char *i18n_cmdinfoshort = "i";

// 'load' Command
const char *i18n_cmdload = "load";
const char *i18n_cmdloadhelp = "Loads a saved game";
const char *i18n_cmdloadshort = "l";

// 'next' Command
const char *i18n_cmdnext = "next";
const char *i18n_cmdnexthelp = "Advances to the next scene";
const char *i18n_cmdnextshort = "n";

// 'quit' Command
const char *i18n_cmdquit = "quit";
const char *i18n_cmdquithelp = "Saves the game and exists";
const char *i18n_cmdquitshort = "q";

// 'room' Command
const char *i18n_cmdroom = "room";
const char *i18n_cmdroomhelp = "Prints a room description or a list of all rooms";
const char *i18n_cmdroomshort = "r";

// 'save' Command
const char *i18n_cmdsave = "save";
const char *i18n_cmdsavehelp = "Saves the game";
const char *i18n_cmdsaveshort = "sa";

// 'scene' Command
const char *i18n_cmdscene = "scene";
const char *i18n_cmdscenehelp = "Replays a scene or prints a list of all scenes";
const char *i18n_cmdsceneshort = "sc";

// 'version' Command
const char *i18n_cmdversion = "version";
const char *i18n_cmdversionhelp = "Prints the engine version";
const char *i18n_cmdversionshort = "v";

// ---------

// Game
const char *i18n_game_end = "Game has ended";
const char *i18n_game_init = "Initializing game";
const char *i18n_game_quit = "Shutting down game";
const char *i18n_game_start = "Game has started";

// ---------

// Parser
const char *i18n_parser_error = "Parser error in line";
const char *i18n_parser_gamespecs = "Game specifications are";
const char *i18n_parser_glossaryentry = "Glossary entry";
const char *i18n_parser_glossarytwice = "There's already a glossary entry with name or alias";
const char *i18n_parser_linesparsed = "Lines parsed";
const char *i18n_parser_parsingfile = "Parsing game file";
const char *i18n_parser_roomtwice = "There's already a room with name or alias";
const char *i18n_parser_scenetwice = "There's already a scene with name or alias";

// ---------

// Savegames
const char *i18n_save_filedoesnexists = "Savegame doesn't exists";
const char *i18n_save_fromanothergame = "Savegame from another game";
const char *i18n_save_init = "Initializing savegames";
const char *i18n_save_listedsaves = "Savegames listed";

// ---------

// Misc
const char *i18n_from = "from";
const char *i18n_name = "Name";

