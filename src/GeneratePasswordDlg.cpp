/* Copyright 2022 Ian Boisvert */
#include "GeneratePasswordDlg.h"
#include <cassert>
#include "Label.h"
#include "Utils.h"

static constexpr int MAX_PASSWORD_LENGTH = 56;
static constexpr int MIN_PASSWORD_LENGTH = 4;
static constexpr int PASSWORD_LENGTH_STEP = 4;

GeneratePasswordDlg::GeneratePasswordDlg(PWSafeApp &app) : m_app(app), m_pwPolicyFlagsIndex(0), m_pwPolicyLength(12)
{
    // clang-format off
    m_app.GetCommandBar().Register(this, {
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
    m_app.GetCommandBar().Show(this);
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
    m_password = m_pwPolicy.MakePassword();
    m_passwordField.Rewrite(m_password.c_str());
}

DialogResult GeneratePasswordDlg::Show(WINDOW *parent)
{
    SetCommandBarWin();

    m_parentWin = parent;

    InitTUI("Generate Password");

    Update();

    touchwin(parent);
    update_panels();
    doupdate();

    DialogResult result = ProcessInput();

    EndTUI();

    touchwin(parent);
    update_panels();
    doupdate();

    return result;
}

void GeneratePasswordDlg::InitTUI(const std::string &title)
{
    int beg_y, beg_x, max_y, max_x;
    getbegyx(m_parentWin, beg_y, beg_x);
    getmaxyx(m_parentWin, max_y, max_x);
    int nlines = 4, ncols = MAX_PASSWORD_LENGTH;
    beg_y = (max_y + beg_y - nlines) / 2;
    beg_x = (max_x + beg_x - ncols) / 2;

    // Add 2 to each dimension for border
    m_win = newwin(nlines + 2, ncols + 4, beg_y, beg_x);
    m_panel = new_panel(m_win);

    // Enable keypad so curses interprets function keys
    keypad(m_win, TRUE);
    box(m_win, 0, 0);

    wattron(m_win, A_BOLD);
    Label::WriteJustified(m_win, /*y*/ 0, /*begin_x*/ 2, ncols + 2, title.c_str(), JUSTIFY_CENTER);
    wattroff(m_win, A_BOLD);

    Label::Write(m_win, /*y*/ 1, /*x*/ 2, "Complexity:");
    Label::Write(m_win, /*y*/ 2, /*x*/ 2, "Length:");
    const int label_begin_x = 14;
    m_pwPolicyField = Label(m_win, /*y*/ 1, label_begin_x, /*width*/ 32);
    m_pwLengthField = Label(m_win, /*y*/ 2, label_begin_x, /*width*/ 10);
    m_passwordField = Label(m_win, /*y*/ 4, /*x*/ 2, /*width*/ ncols, A_BOLD, JUSTIFY_CENTER);

    // Reset cursor for input fields
    m_saveCursor = curs_set(0);
}

void GeneratePasswordDlg::EndTUI()
{
    del_panel(m_panel);
    m_panel = nullptr;
    delwin(m_win);
    m_win = nullptr;
    curs_set(m_saveCursor);
}

/** Input driver */
DialogResult GeneratePasswordDlg::ProcessInput()
{
    DialogResult retval = DialogResult::CANCEL;
    int ch;
    while ((ch = wgetch(m_win)) != ERR)
    {
        switch (ch)
        {
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
