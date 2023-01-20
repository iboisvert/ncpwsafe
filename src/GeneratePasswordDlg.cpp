/* Copyright 2022 Ian Boisvert */
#include "GeneratePasswordDlg.h"
#include "Label.h"
#include "Utils.h"
#include "core/PolicyManager.h"

static constexpr int MAX_PASSWORD_LENGTH = 56;
static constexpr int MIN_PASSWORD_LENGTH = 4;
static constexpr int PASSWORD_LENGTH_STEP = 4;

struct PWPolicyFlags
{
    unsigned flags;
    const char *name;
};

// clang-format off
static PWPolicyFlags PW_POLICY[PW_POLICY_COUNT]{
    {0xf000, "alpha/digit/symbol"},
    {0xe000, "alpha/digit"},
    {0xf400, "easy-to-read alpha/digit/symbol"},
    {0x2000, "digits only"},
    {0x0800, "hex digits only"},
};
// clang-format on

unsigned int GetPolicyFlags(size_t policyIndex)
{
    assert(policyIndex < PW_POLICY_COUNT);
    return PW_POLICY[policyIndex].flags;
}

const char *GetPolicyName(size_t policyIndex)
{
    assert(policyIndex < PW_POLICY_COUNT);
    return PW_POLICY[policyIndex].name;
}

PWPolicy CreatePolicy(unsigned flags, size_t length)
{
    wchar_t buf[20];
    swprintf(buf, 20, L"%4x%3zx000000000000", flags, length);
    return PWPolicy(buf);
}

GeneratePasswordDlg::GeneratePasswordDlg(PWSafeApp &app) : m_app(app), m_pwPolicyFlagsIndex(0), m_pwPolicyLength(12)
{
    // clang-format off
    m_app.GetCommandBar().Register(this, {
        {L"^S", L"Save and close", L"Save the new password and close"},
        {L"^X", L"Cancel", L"Cancel changing password"}, 
        {L"Enter", L"Generate", L"Generate a new password"},
        {L"<,>", L"Length", L"Change the password length"},
        {L"Space", L"Complexity", L"Change the complexity of the generated password. The generated password may contain symbols, characters, and digits"}
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
    m_pwPolicy = CreatePolicy(PW_POLICY[m_pwPolicyFlagsIndex].flags, m_pwPolicyLength);
    m_pwPolicyField.Rewrite(PW_POLICY[m_pwPolicyFlagsIndex].name);
}

void GeneratePasswordDlg::UpdatePWLength()
{
    char buf[4];
    snprintf(buf, 4, "%d", m_pwPolicyLength);
    m_pwLengthField.Rewrite(buf);
}

void GeneratePasswordDlg::UpdatePassword()
{
    m_password = m_pwPolicy.MakeRandomPassword();
    m_passwordField.Rewrite(m_password.c_str());
}

DialogResult GeneratePasswordDlg::Show(WINDOW *parent)
{
    PolicyManager pm(m_app.GetCore());
    m_pwPolicy = pm.GetDefaultPolicy();
    StringX str = m_pwPolicy.GetDisplayString();
    StringX str2 = m_pwPolicy;

    SetCommandBarWin();

    m_parentWin = parent;

    InitTUI(L"Generate Password");

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

void GeneratePasswordDlg::InitTUI(const stringT &title)
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
            m_pwPolicyFlagsIndex = (m_pwPolicyFlagsIndex + 1) % PW_POLICY_COUNT;
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
