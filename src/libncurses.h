/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_LIBNCURSES_H
#define HAVE_LIBNCURSES_H

#define NCURSES_WIDECHAR 1
#include <form.h>
#include <menu.h>
#include <ncurses.h>
#include <panel.h>

// curses #defines OK...
#ifdef OK
#undef OK
#endif
/** The curses OK return value, the macro is undefined above */
constexpr int CURSES_OK = 0;

#endif //#ifndef HAVE_LIBNCURSES_H
