/* Copyright 2020 Ian Boisvert */

#include <algorithm>
#include "MessageBox.h"
#include "Utils.h"

bool DefaultMessageBoxKeyHandler(int ch, DialogResult &result)
{
    result = DialogResult::OK;
    return ch == '\n';
}

static void SizeMsg(const std::string &msg, int &lines, int &cols)
{
    lines = 0;
    int idx;
    size_t maxcol = 0, pos = 0;
    while ((idx = msg.find(L'\n', pos)) != -1)
    {
        ++lines;
        maxcol = std::max(maxcol, idx - pos);
        pos = idx + 1;
    }
    if (msg.length() > pos)
    {
        ++lines;
        maxcol = std::max(maxcol, msg.length() - pos);
    }
    cols = maxcol;
}

/** Print a message with embedded newlines */
static int PrintMessage(WINDOW *win, int y, int x, const std::string &msg)
{
    int code = 0;
    int idx;
    size_t pos = 0;
    while ((idx = msg.find(L'\n', pos)) != -1)
    {
        code = mvwaddnstr(win, y, x, msg.c_str() + pos, idx - pos);
        if (code != 0)
            return code;
        ++y;
        pos = idx + 1;
    }
    if (msg.length() > pos)
    {
        code = mvwaddnstr(win, y, x, msg.c_str() + pos, msg.length() - pos);
    }
    return code;
}

static void ScaleMsgBox(const WINDOW *parent, const int &lines, const int &cols, int &y, int &x)
{
    int maxy, maxx;
    getmaxyx(parent, maxy, maxx);
    y = (maxy - lines) / 2;
    x = (maxx - cols) / 2;
}

/** Show the message box */
DialogResult MessageBox::Show(WINDOW *parent, const std::string &msg, KeyHandler handler)
{
    if (handler == &DefaultMessageBoxKeyHandler)
    {
        app_.GetCommandBar().Show({
            {"Enter", "Close"}
        });
    }

    InitTUI(parent, msg);

    PrintMessage(win_, 2, 2, msg.c_str());

    touchwin(parent);
    update_panels();
    doupdate();

    DialogResult retval = ProcessInput(handler);

    EndTUI();

    touchwin(parent);
    update_panels();
    doupdate();

    return retval;
}

void MessageBox::InitTUI(WINDOW *parent, const std::string &msg)
{
    int lines, cols, y, x;
    SizeMsg(msg, lines, cols);
    lines += 2;
    cols += 2;
    ScaleMsgBox(parent, lines, cols, y, x);

    win_ = newwin(lines + 2, cols + 2, y, x);
    panel_ = new_panel(win_);
    
    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);
    box(win_, 0, 0);

    // Reset cursor for input fields
    save_cursor_ = curs_set(0);
}

void MessageBox::EndTUI()
{
    del_panel(panel_);
    panel_ = nullptr;
    delwin(win_);
    win_ = nullptr;

    curs_set(save_cursor_);
}

/** Input driver */
DialogResult MessageBox::ProcessInput(KeyHandler handler)
{
    DialogResult result = DialogResult::CANCEL;
    int c;
    while ((c = wgetch(win_)) != ERR)
    {
        if (handler(c, result))
            break;
    }
    return result;
}
