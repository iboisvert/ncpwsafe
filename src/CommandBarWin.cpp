/* Copyright 2020 Ian Boisvert */
#include "PWSafeApp.h"
#include "CommandBarWin.h"
#include <algorithm>
#include "HelpWin.h"
#include "Utils.h"

const Action Action::YES{"Y", "Yes"};
const Action Action::NO{"N", "No"};
const Action CommandBarWin::MORE{"^G", "More"};

const std::vector<Action> CommandBarWin::YES_NO{Action::YES, Action::NO};

void CommandBarWin::Show(void *p, int opts)
{
    last_actions_ = actions_.at(p);
    last_opts_ = opts;
    CommandBarWin::ShowActions(app_, win_, last_actions_, last_opts_);
}

/** Update the command bar with the give actions */
void CommandBarWin::Show(std::vector<Action> actions)
{
    last_actions_ = std::move(actions);
    last_opts_ = -1;
    CommandBarWin::ShowActions(app_, win_, last_actions_, last_opts_);
}

void CommandBarWin::Redraw()
{
    CommandBarWin::DrawActions(app_, win_, last_actions_, last_opts_);
}

void CommandBarWin::ShowActions(const PWSafeApp &app, WINDOW *win, const std::vector<Action> &actions, int opts)
{
    DrawActions(app, win, actions, opts);
    wrefresh(win);
}

/** Columns taken by "KEY Name" */
static int ActionWidth(const Action &action)
{
    return DisplayWidth(action.m_key) + /*space*/ 1 + DisplayWidth(action.m_name);
}

static void DrawAction(const Theme &theme, WINDOW *win, const Action &action)
{
    {
        ScopedAttr key(win, theme, Role::KEY);
        waddstr(win, action.m_key.c_str());
    }
    waddch(win, ' ');
    waddstr(win, action.m_name.c_str());
}

void CommandBarWin::DrawActions(const PWSafeApp &app, WINDOW *win, const std::vector<Action> &actions, int opts)
{
    const Theme &theme = app.GetTheme();
    wattrset(win, theme.Attr(Role::KEY_DESC));

    const int beg_x = getbegx(win), max_x = getmaxx(win);
    constexpr int SPACER = 2;

    std::vector<const Action *> visible;
    int total = 0;
    for (const Action &action : actions)
    {
        if ((opts & action.m_mask) == 0)
            continue;
        total += (visible.empty() ? 0 : SPACER) + ActionWidth(action);
        visible.push_back(&action);
    }
    // When they don't all fit, end with "^G More", which lists them all (see ShowHelp())
    const bool truncated = beg_x + total > max_x;
    const int limit = truncated ? max_x - SPACER - ActionWidth(MORE) : max_x;

    wmove(win, /*y*/ 0, beg_x);
    int col = beg_x, spacer = 0;
    for (const Action *action : visible)
    {
        const int len = spacer + ActionWidth(*action);
        if (col + len > limit)
            break;
        for (int j = 0; j < spacer; ++j)
            waddch(win, ' ');
        DrawAction(theme, win, *action);
        spacer = SPACER;
        col += len;
    }
    if (truncated && col + spacer + ActionWidth(MORE) <= max_x)
    {
        for (int j = 0; j < spacer; ++j)
            waddch(win, ' ');
        DrawAction(theme, win, MORE);
        col += spacer + ActionWidth(MORE);
        spacer = SPACER;
    }
    for (int j = 0; j < spacer && col + j < max_x; ++j)
        waddch(win, ' ');

    whline(win, ' ', std::max(0, max_x - getcurx(win)));
    wattrset(win, A_NORMAL);
}

void CommandBarWin::ShowHelp(WINDOW *parent)
{
    // Showing the help replaces the command bar, put it back afterwards
    const std::vector<Action> actions = last_actions_;
    const int opts = last_opts_;

    std::vector<Action> visible;
    for (const Action &action : actions)
    {
        if ((opts & action.m_mask) != 0)
            visible.push_back(action);
    }
    HelpWin(app_, std::move(visible)).Show(parent);

    last_actions_ = actions;
    last_opts_ = opts;
    ShowActions(app_, win_, last_actions_, last_opts_);
}
