/* Copyright 2020 Ian Boisvert */

#include <algorithm>
#include "MessageBox.h"
#include "Utils.h"

bool DefaultMessageBoxKeyHandler(int ch, DialogResult &result)
{
    result = DialogResult::OK;
    return ch == '\n';
}

/** Space between the window edge and the message, including the border */
static constexpr int MARGIN = 2;

/**
 * Wrap the message to fit in `parent`, and cut it to fit if it has more lines than
 * `parent` has room for
 */
static std::vector<std::string> LayoutMsg(const WINDOW *parent, const std::string &msg)
{
    int maxy, maxx;
    getmaxyx(parent, maxy, maxx);
    std::vector<std::string> lines = WrapText(msg, std::max(1, maxx - 2 * MARGIN));
    size_t max_lines = std::max(1, maxy - 2 * MARGIN);
    if (lines.size() > max_lines)
    {
        lines.resize(max_lines);
    }
    return lines;
}

/** Print the lines of the message */
static void PrintMessage(WINDOW *win, int y, int x, const std::vector<std::string> &lines)
{
    for (const std::string &line : lines)
    {
        mvwaddstr(win, y++, x, line.c_str());
    }
}

/** Screen position of a window of size `lines` x `cols` centered on `parent` */
static void ScaleMsgBox(const WINDOW *parent, const int &lines, const int &cols, int &y, int &x)
{
    int begy, begx, maxy, maxx;
    getbegyx(parent, begy, begx);
    getmaxyx(parent, maxy, maxx);
    y = begy + (maxy - lines) / 2;
    x = begx + (maxx - cols) / 2;
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

    touchwin(parent);
    update_panels();
    doupdate();

    DialogResult retval;
    {
        ScopedResizeHandler resize_handler(app_, [this]() { OnResize(); });
        retval = ProcessInput(handler);
    }

    EndTUI();

    touchwin(parent);
    update_panels();
    doupdate();

    return retval;
}

void MessageBox::InitTUI(WINDOW *parent, const std::string &msg)
{
    parent_win_ = parent;
    msg_ = msg;

    lines_ = LayoutMsg(parent_win_, msg_);
    win_ = NewWindow(lines_);
    panel_ = new_panel(win_);

    Draw();

    // Reset cursor for input fields
    save_cursor_ = curs_set(0);
}

WINDOW *MessageBox::NewWindow(const std::vector<std::string> &lines) const
{
    size_t cols = 0;
    for (const std::string &line : lines)
    {
        cols = std::max(cols, Utf8Length(line));
    }
    // The message is laid out to fit the parent, but the margins may not fit a very small one
    int maxy, maxx;
    getmaxyx(parent_win_, maxy, maxx);
    int nlines = std::min(static_cast<int>(lines.size()) + 2 * MARGIN, std::max(1, maxy));
    int ncols = std::min(static_cast<int>(cols) + 2 * MARGIN, std::max(1, maxx));

    int y, x;
    ScaleMsgBox(parent_win_, nlines, ncols, y, x);
    return newwin(nlines, ncols, std::max(0, y), std::max(0, x));
}

void MessageBox::Draw()
{
    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);
    box(win_, 0, 0);

    PrintMessage(win_, MARGIN, MARGIN, lines_);
}

void MessageBox::OnResize()
{
    std::vector<std::string> lines = LayoutMsg(parent_win_, msg_);
    WINDOW *win = NewWindow(lines);
    if (win == nullptr)
    {
        // The message box can't be created, leave it as ncurses truncated it.
        // It will be laid out again when the terminal is resized.
        return;
    }
    lines_ = std::move(lines);

    // Replacing the window keeps the panel's position in the panel stack
    replace_panel(panel_, win);
    delwin(win_);
    win_ = win;

    Draw();
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
    if (win_ == nullptr)
    {
        // The window couldn't be created, ReadKey() would return ERR at once
        return result;
    }
    int c;
    while ((c = ReadKey(win_)) != ERR)
    {
        if (c == KEY_RESIZE)
        {
            app_.HandleResize();
            continue;
        }

        if (handler(c, result))
            break;
    }
    return result;
}
