/* Copyright 2022 Ian Boisvert */
#include "GeneratePasswordDlg.h"
#include <cassert>
#include "Label.h"
#include "Utils.h"

static constexpr int MAX_PASSWORD_LENGTH = 56;
static constexpr int MIN_PASSWORD_LENGTH = 4;
static constexpr int PASSWORD_LENGTH_STEP = 4;

GeneratePasswordDlg::GeneratePasswordDlg(PWSafeApp &app) : app_(app), m_pwPolicyFlagsIndex(0), m_pwPolicyLength(12)
{
    // clang-format off
    app_.GetCommandBar().Register(this, {
        {"^S", "Save and close", "Save the new password and close"},
        {"^X", "Cancel", "Cancel changing password"}, 
        {"Enter", "Generate", "Generate a new password"},
        {"<,>", "Length", "Change the password length"},
        {"Space", "Complexity", "Change the complexity of the generated password. The generated password may contain symbols, characters, and digits"}
    });
    // clang-format on
}

void GeneratePasswordDlg::SetCommandBarWin()
{
    app_.GetCommandBar().Show(this);
}

void GeneratePasswordDlg::Update()
{
    UpdatePWPolicy();
    UpdatePWLength();
    UpdatePassword();
}

void GeneratePasswordDlg::UpdatePWPolicy()
{
    m_pwPolicy = PasswordPolicy{static_cast<PasswordPolicy::Composition>(m_pwPolicyFlagsIndex), m_pwPolicyLength};
    m_pwPolicyField.Rewrite(m_pwPolicy.GetName());
}

void GeneratePasswordDlg::UpdatePWLength()
{
    char buf[4];
    snprintf(buf, 4, "%zd", m_pwPolicyLength);
    m_pwLengthField.Rewrite(buf);
}

void GeneratePasswordDlg::UpdatePassword()
{
    password_ = m_pwPolicy.MakePassword();
    m_passwordField.Rewrite(password_.c_str());
}

DialogResult GeneratePasswordDlg::Show(WINDOW *parent)
{
    SetCommandBarWin();

    parent_win_ = parent;

    InitTUI("Generate Password");

    Update();

    touchwin(parent);
    update_panels();
    doupdate();

    DialogResult result;
    {
        ScopedResizeHandler resize_handler(app_, [this]() { OnResize(); });
        result = ProcessInput();
    }

    EndTUI();

    touchwin(parent);
    update_panels();
    doupdate();

    return result;
}

void GeneratePasswordDlg::InitTUI(const std::string &title)
{
    title_ = title;

    win_ = NewWindow();
    panel_ = new_panel(win_);

    Draw();

    // Reset cursor for input fields
    save_cursor_ = curs_set(0);
}

static constexpr int DLG_NLINES = 4, DLG_NCOLS = MAX_PASSWORD_LENGTH;

WINDOW *GeneratePasswordDlg::NewWindow() const
{
    int beg_y, beg_x, max_y, max_x;
    getbegyx(parent_win_, beg_y, beg_x);
    getmaxyx(parent_win_, max_y, max_x);
    beg_y = std::max(0, (max_y + beg_y - DLG_NLINES) / 2);
    beg_x = std::max(0, (max_x + beg_x - DLG_NCOLS) / 2);

    // Add 2 to each dimension for border
    return newwin(DLG_NLINES + 2, DLG_NCOLS + 4, beg_y, beg_x);
}

void GeneratePasswordDlg::Draw()
{
    const int ncols = DLG_NCOLS;

    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);
    box(win_, 0, 0);

    const Theme &theme = app_.GetTheme();
    const std::string title = " " + title_ + " ";
    Label::Write(win_, /*y*/ 0, /*begin_x*/ 2, ncols + 2, title.c_str(), theme.Attr(Role::TITLE), JUSTIFY_CENTER);

    Label::Write(win_, /*y*/ 1, /*x*/ 2, /*width*/ 0, "Complexity:", theme.Attr(Role::LABEL));
    Label::Write(win_, /*y*/ 2, /*x*/ 2, /*width*/ 0, "Length:", theme.Attr(Role::LABEL));
    const int label_begin_x = 14;
    m_pwPolicyField = Label(win_, /*y*/ 1, label_begin_x, /*width*/ 32);
    m_pwLengthField = Label(win_, /*y*/ 2, label_begin_x, /*width*/ 10);
    m_passwordField = Label(win_, /*y*/ 4, /*x*/ 2, /*width*/ ncols, theme.Attr(Role::SECRET), JUSTIFY_CENTER);
}

void GeneratePasswordDlg::OnResize()
{
    WINDOW *win = NewWindow();
    if (win == nullptr)
    {
        // The dialog doesn't fit on the screen, leave it as ncurses truncated it.
        // It will be laid out again when the terminal is made large enough.
        return;
    }

    // Replacing the window keeps the panel's position in the panel stack
    replace_panel(panel_, win);
    delwin(win_);
    win_ = win;

    Draw();

    // Redraw the current values without generating a new password
    m_pwPolicyField.Rewrite(m_pwPolicy.GetName());
    UpdatePWLength();
    m_passwordField.Rewrite(password_.c_str());
}

void GeneratePasswordDlg::EndTUI()
{
    del_panel(panel_);
    panel_ = nullptr;
    delwin(win_);
    win_ = nullptr;
    curs_set(save_cursor_);
}

/** Input driver */
DialogResult GeneratePasswordDlg::ProcessInput()
{
    DialogResult retval = DialogResult::CANCEL;
    int ch;
    while ((ch = ReadKey(win_)) != ERR)
    {
        switch (ch)
        {
        case KEY_RESIZE: {
            app_.HandleResize();
            break;
        }
        case KEY_CTRL('X'): {
            goto done;
        }
        case KEY_CTRL('S'): {
            retval = DialogResult::OK;
            goto done;
        }
        case '.':
        case '>': {
            if (m_pwPolicyLength <= MAX_PASSWORD_LENGTH - PASSWORD_LENGTH_STEP)
            {
                m_pwPolicyLength += PASSWORD_LENGTH_STEP;
                Update();
            }
            break;
        }
        case ',':
        case '<': {
            if (m_pwPolicyLength > MIN_PASSWORD_LENGTH)
            {
                m_pwPolicyLength -= PASSWORD_LENGTH_STEP;
                Update();
            }
            break;
        }
        case ' ': {
            m_pwPolicyFlagsIndex = (m_pwPolicyFlagsIndex + 1) % PasswordPolicy::Composition::COUNT;
            UpdatePWPolicy();
            UpdatePassword();
            break;
        }
        case '\n':
            UpdatePassword();
            break;
        }

        // update_panels();
        // doupdate();
    }
done:
    return retval;
}
