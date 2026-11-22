/* Copyright 2020 Ian Boisvert */
#include "PWSafeApp.h"
#include "CommandBarWin.h"

const Action Action::YES{"Y", "Yes"};
const Action Action::NO{"N", "No"};

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

void CommandBarWin::DrawActions(const PWSafeApp &app, WINDOW *win, const std::vector<Action> &actions, int opts)
{
    const Theme &theme = app.GetTheme();
    wattrset(win, theme.Attr(Role::KEY_DESC));

    int beg_x = getbegx(win), max_x = getmaxx(win);

    wmove(win, /*y*/ 0, beg_x);

    int col = beg_x, spacer = 0;
    for (size_t i = 0; i < actions.size(); ++i)
    {
        const Action &action = actions[i];

        if ((opts & action.m_mask) == 0)
            continue;

        int len = spacer + action.m_key.size() + /*space*/ 1 + action.m_name.size();
        if (col + len > max_x)
            break;

        for (int j = 0; j < spacer; ++j)
            waddch(win, ' ');
        {
            ScopedAttr key(win, theme, Role::KEY);
            waddstr(win, action.m_key.c_str());
        }
        waddch(win, ' ');
        waddstr(win, action.m_name.c_str());

        spacer = 2;
        col += len;
    }
    for (int j = 0; j < spacer; ++j)
        waddch(win, ' ');

    whline(win, ' ', max_x-col);
    wattrset(win, A_NORMAL);
}