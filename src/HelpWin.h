/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_HELPWIN_H
#define HAVE_HELPWIN_H

#include <vector>
#include "libncurses.h"
#include "CommandBarWin.h"

class PWSafeApp;

/**
 * The ^G help overlay: every key of the current context with its description,
 * including the ones that don't fit in the command bar. Any key closes it.
 */
class HelpWin
{
public:
    /** @param actions The actions to list, already filtered for the context */
    HelpWin(PWSafeApp &app, std::vector<Action> actions) : app_(app), actions_(std::move(actions))
    {
    }

    /** Show the help over `parent` until a key is pressed */
    void Show(WINDOW *parent);

private:
    /** A window sized for the actions, centred on the parent; `nullptr` if it can't be created */
    WINDOW *NewWindow() const;
    void Draw();
    void OnResize();

    PWSafeApp &app_;
    std::vector<Action> actions_;
    WINDOW *parent_win_ = nullptr;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
};

#endif  // #ifndef HAVE_HELPWIN_H
