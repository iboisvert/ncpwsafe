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

#endif //#ifndef HAVE_LIBNCURSES_H
