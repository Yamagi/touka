/*
 * i18n.h
 * ------
 *
 * Declarations of translation strings. The strings
 * are defined in the corresponding translation file.
 */

#ifndef I18N_H_
#define I18N_H_

// ---------

// Objects
extern const char *i18n_aliases;
extern const char *i18n_choices;
extern const char *i18n_words;

extern const char *i18n_entry;
extern const char *i18n_room;
extern const char *i18n_scene;

// ---------

// Table headers
extern const char *i18n_head_attribute;
extern const char *i18n_head_description;
extern const char *i18n_head_saves;
extern const char *i18n_head_state;
extern const char *i18n_head_value;

// ---------

// Curses TUI
extern const char *i18n_curses_8colorsonly;
extern const char *i18n_curses_init;
extern const char *i18n_curses_newtermsize;
extern const char *i18n_curses_quit;
extern const char *i18n_curses_termresize;
extern const char *i18n_curses_termsize;
extern const char *i18n_curses_userinput;

// ---------

// Link matching
extern const char *i18n_link_didntmatch;
extern const char *i18n_link_linebreak;
extern const char *i18n_link_nestedlink;
extern const char *i18n_link_notopened;
extern const char *i18n_link_openatend;

// ---------

// Glossary
extern const char *i18n_glossary_entrieslisted;
extern const char *i18n_glossary_notfound;

// ---------

// Room
extern const char *i18n_room_mentioned;
extern const char *i18n_room_notfound;
extern const char *i18n_room_roomslisted;

// ---------

// Scene
extern const char *i18n_scene_choice;
extern const char *i18n_scene_firstnotfound;
extern const char *i18n_scene_invalidchoice;
extern const char *i18n_scene_listed;
extern const char *i18n_scene_next;
extern const char *i18n_scene_nochoice;
extern const char *i18n_scene_notfound;
extern const char *i18n_scene_play;
extern const char *i18n_scene_playerschoice;

// ---------

// Startscreen
extern const char *i18n_start_author;
extern const char *i18n_start_stats;
extern const char *i18n_start_welcome;

extern const char *i18n_start_glossary;
extern const char *i18n_start_rooms;
extern const char *i18n_start_scenes;

extern const char *i18n_start_help1;
extern const char *i18n_start_help2;
extern const char *i18n_start_help3;

// ---------

// Endscreen
extern const char *i18n_end_congratulations;
extern const char *i18n_end_glossaryentriesseen;
extern const char *i18n_end_roomsvisited;
extern const char *i18n_end_scenesplayed;
extern const char *i18n_end_stats;
extern const char *i18n_end_statusbar;

// ---------

// Input
extern const char *i18n_input_command;
extern const char *i18n_input_cmdnotfound;
extern const char *i18n_input_cmdslisted;
extern const char *i18n_input_init;
extern const char *i18n_input_quit;

// ---------

// Version
extern const char *i18n_version_buildon;
extern const char *i18n_version_thisis;

// ---------

// 'glossary' Command
extern const char *i18n_cmdglossary;
extern const char *i18n_cmdglossaryhelp;
extern const char *i18n_cmdglossaryshort;

// 'help' Command
extern const char *i18n_cmdhelp;
extern const char *i18n_cmdhelphelp;
extern const char *i18n_cmdhelpshort;

// 'info' Command
extern const char *i18n_cmdinfo;
extern const char *i18n_cmdinfohelp;
extern const char *i18n_cmdinfoshort;

// 'load' Command
extern const char *i18n_cmdload;
extern const char *i18n_cmdloadhelp;
extern const char *i18n_cmdloadshort;

// 'next' Command
extern const char *i18n_cmdnext;
extern const char *i18n_cmdnexthelp;
extern const char *i18n_cmdnextshort;

// 'quit' Command
extern const char *i18n_cmdquit;
extern const char *i18n_cmdquithelp;
extern const char *i18n_cmdquitshort;

// 'room' Command
extern const char *i18n_cmdroom;
extern const char *i18n_cmdroomhelp;
extern const char *i18n_cmdroomshort;

// 'save' Command
extern const char *i18n_cmdsave;
extern const char *i18n_cmdsavehelp;
extern const char *i18n_cmdsaveshort;

// 'scene' Command
extern const char *i18n_cmdscene;
extern const char *i18n_cmdscenehelp;
extern const char *i18n_cmdsceneshort;

// 'version' Command
extern const char *i18n_cmdversion;
extern const char *i18n_cmdversionhelp;
extern const char *i18n_cmdversionshort;

// ---------

// Game
extern const char *i18n_game_end;
extern const char *i18n_game_init;
extern const char *i18n_game_quit;
extern const char *i18n_game_start;

// ---------

// Parser
extern const char *i18n_parser_error;
extern const char *i18n_parser_gamespecs;
extern const char *i18n_parser_glossaryentry;
extern const char *i18n_parser_glossarytwice;
extern const char *i18n_parser_linesparsed;
extern const char *i18n_parser_parsingfile;
extern const char *i18n_parser_roomtwice;
extern const char *i18n_parser_scenetwice;

// ---------

// Savegames
extern const char *i18n_save_filedoesnexists;
extern const char *i18n_save_fromanothergame;
extern const char *i18n_save_init;
extern const char *i18n_save_listedsaves;

// ---------

// Misc
extern const char *i18n_from;
extern const char *i18n_name;

// --------

// Header
extern const char *i18n_info_author;
extern const char *i18n_info_date;
extern const char *i18n_info_game;
extern const char *i18n_info_uid;

// --------

// Errorcodes
extern const char *i18n_error_brokensave;
extern const char *i18n_error_couldntclosefile;
extern const char *i18n_error_couldntcreatedir;
extern const char *i18n_error_couldntloadhistory;
extern const char *i18n_error_couldloadsave;
extern const char *i18n_error_couldntopendir;
extern const char *i18n_error_couldntopenfile;
extern const char *i18n_error_couldntrotatelogs;
extern const char *i18n_error_couldntsavehistory;
extern const char *i18n_error_couldntwritelogmsg;
extern const char *i18n_error_filenotexist;
extern const char *i18n_error_firstscenenotfound;
extern const char *i18n_error_invgameheader;
extern const char *i18n_error_localtime;
extern const char *i18n_error_notadir;
extern const char *i18n_error_notafile;
extern const char *i18n_error_outofmem;
extern const char *i18n_error_parsererr;
extern const char *i18n_error_roomnotfound;
extern const char *i18n_error_scenenotfound;
extern const char *i18n_error_unkownlogtype;

// --------

#endif // I18N_H_

