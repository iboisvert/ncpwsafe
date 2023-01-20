/* Copyright 2022 Ian Boisvert */
#pragma once

#define NCURSES_WIDECHAR 1
#include <ncursesw/form.h>
#include <ncursesw/menu.h>
#include <ncursesw/ncurses.h>
#include <ncursesw/panel.h>

// curses #defines OK...
#ifdef OK
#undef OK
#endif
