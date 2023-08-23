/* Copyright 2020 Ian Boisvert */
#include "PWSafeApp.h"
#include "CommandBarWin.h"

const Action Action::YES{"Y", "Yes"};
const Action Action::NO{"N", "No"};

const std::vector<Action> CommandBarWin::YES_NO{Action::YES, Action::NO};

void CommandBarWin::Show(void *p, int opts)
{
    CommandBarWin::ShowActions(app_, win_, actions_.at(p), opts);
}

/** Update the command bar with the give actions */
void CommandBarWin::Show(std::vector<Action> actions)
{
    CommandBarWin::ShowActions(app_, win_, actions, /*opts*/ (int)-1);
}

static const char *STATUS_READ_ONLY = "RO";

void CommandBarWin::ShowActions(const PWSafeApp &app, WINDOW *win, const std::vector<Action> &actions, int opts)
{
    wattron(win, A_REVERSE/* | A_DIM*/);

    const bool read_only = app.GetDb().ReadOnly();
    int status_len = read_only ? 2 : 0;
    int beg_x = getbegx(win), max_x = getmaxx(win) - status_len;

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
        wattron(win, A_BOLD);
        waddstr(win, action.m_key.c_str());
        wattroff(win, A_BOLD);
        waddch(win, ' ');
        waddstr(win, action.m_name.c_str());

        spacer = 2;
        col += len;
    }
    for (int j = 0; j < spacer; ++j)
        waddch(win, ' ');

    whline(win, ' ', max_x-col);
    if (read_only)
    {
        mvwaddstr(win, /*y*/0, max_x, STATUS_READ_ONLY);
    }

    wrefresh(win);
}