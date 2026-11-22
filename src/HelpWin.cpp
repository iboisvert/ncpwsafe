/* Copyright 2026 Ian Boisvert */
#include "HelpWin.h"
#include <algorithm>
#include "PWSafeApp.h"
#include "Utils.h"

static constexpr const char *TITLE = " Keys ";
/** Columns between the border and the text, and between the key and its description */
static constexpr int MARGIN = 2;

static int KeyColumnWidth(const std::vector<Action> &actions)
{
    int width = 0;
    for (const Action &action : actions)
    {
        width = std::max(width, DisplayWidth(action.m_key));
    }
    return width;
}

/** The description of an action, or its name if it has none */
static const std::string &Description(const Action &action)
{
    return action.m_description.empty() ? action.m_name : action.m_description;
}

WINDOW *HelpWin::NewWindow() const
{
    int text_width = DisplayWidth(TITLE);
    const int key_width = KeyColumnWidth(actions_);
    for (const Action &action : actions_)
    {
        text_width = std::max(text_width, key_width + MARGIN + DisplayWidth(Description(action)));
    }

    int maxy, maxx, begy, begx;
    getmaxyx(parent_win_, maxy, maxx);
    getbegyx(parent_win_, begy, begx);
    const int nlines = std::min(static_cast<int>(actions_.size()) + 4, std::max(1, maxy));
    const int ncols = std::min(text_width + 2 * MARGIN + 2, std::max(1, maxx));
    const int y = begy + std::max(0, (maxy - nlines) / 2);
    const int x = begx + std::max(0, (maxx - ncols) / 2);
    return newwin(nlines, ncols, y, x);
}

void HelpWin::Draw()
{
    const Theme &theme = app_.GetTheme();
    const char *ellipsis = theme.Sym(Symbol::ELLIPSIS);
    werase(win_);
    box(win_, 0, 0);
    {
        ScopedAttr title(win_, theme, Role::TITLE);
        mvwaddstr(win_, 0, std::max(1, (getmaxx(win_) - DisplayWidth(TITLE)) / 2), TITLE);
    }

    const int key_width = KeyColumnWidth(actions_);
    const int desc_x = 1 + MARGIN + key_width + MARGIN;
    const int desc_width = getmaxx(win_) - desc_x - MARGIN;
    const int nrows = getmaxy(win_) - 4;
    for (int i = 0; i < static_cast<int>(actions_.size()) && i < nrows; ++i)
    {
        const Action &action = actions_[static_cast<size_t>(i)];
        const int y = 2 + i;
        {
            ScopedAttr key(win_, theme, Role::KEY);
            mvwaddstr(win_, y, 1 + MARGIN, action.m_key.c_str());
        }
        if (desc_width > 0)
        {
            mvwaddstr(win_, y, desc_x, FitText(Description(action), desc_width, ellipsis).c_str());
        }
    }
}

void HelpWin::OnResize()
{
    WINDOW *win = NewWindow();
    if (win == nullptr)
    {
        return;
    }
    // Replacing the window keeps the panel's position in the panel stack
    replace_panel(panel_, win);
    delwin(win_);
    win_ = win;
    Draw();
}

void HelpWin::Show(WINDOW *parent)
{
    parent_win_ = parent;
    win_ = NewWindow();
    if (win_ == nullptr)
    {
        return;
    }
    panel_ = new_panel(win_);
    keypad(win_, TRUE);
    const int save_cursor = curs_set(0);
    Draw();

    app_.GetCommandBar().Show({{"Esc", "Close"}});
    update_panels();
    doupdate();

    {
        ScopedResizeHandler resize_handler(app_, [this]() { OnResize(); });
        int c;
        while ((c = wgetch(win_)) != ERR)
        {
            if (c == KEY_RESIZE)
            {
                app_.HandleResize();
                continue;
            }
            break;
        }
    }

    del_panel(panel_);
    panel_ = nullptr;
    delwin(win_);
    win_ = nullptr;
    curs_set(save_cursor);
    touchwin(parent);
    update_panels();
    doupdate();
}
