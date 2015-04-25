cmake_minimum_required(VERSION 3.1)
project(touka)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Werror -pedantic")

find_library(CURSES ncursesw)

set (LIBRARIES
    ${CURSES})

set(SOURCE_FILES
    src/data/darray.c
    src/data/hashmap.c
    src/data/list.c
    src/i18n/english.c
    src/curses.c
    src/game.c
    src/input.c
    src/log.c
    src/main.c
    src/misc.c
    src/parser.c
    src/quit.c
    src/save.c)

set(HEADERS
    src/i18n/i18n.h)

add_executable(touka ${SOURCE_FILES} ${HEADERS})
target_link_libraries(touka ${LIBRARIES})
