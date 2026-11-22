/* Copyright 2020 Ian Boisvert */
/**
 * AccountsWin implements the main scrolling list of
 * account entries.
 */
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <cerrno>
#include <unistd.h>
#endif // #ifndef _WIN32

#include <algorithm>

#include "AccountDetailsDlg.h"
#include "AccountsWin.h"
#include "ChangePasswordDlg.h"
#include "ChangeDbPasswordCommand.h"
#include "ChangeDbPasswordDlg.h"
#include "MessageBox.h"
#include "PWSafeApp.h"
#include "Utils.h"
#ifdef _WIN32
#include "Win32.h"
#endif

#ifndef _WIN32
static constexpr int COMMAND_NOT_FOUND = 127;
static const char *XCLIP = "/usr/bin/xclip";

// This code adapted from
// https://stackoverflow.com/questions/9405985/linux-executing-child-process-with-piped-stdin-stdout Credit and thanks
// to @Ammo Goettsch and @eerpini
/**
 * Copy text to the clipboard using xclip.
 *
 * Default path for xclip is `/usr/bin/xclip`.
 * This can be overridden using the `NCPWSAFE_XCLIP` environment variable.
 *
 * @remark
 * This is somewhat of a hack. A better solution might be
 * to grab the code from Nicolas Dade's pwsafe and use it instead
 *
 * @returns
 * 1 if xclip executable cannot be opened
 * 2 if an error occurs creating pipes to map to child process stdin, stdout
 * 3 if fork() fails
 * 4 if DISPLAY is not set
 * 5 if writing to xclip fails or xclip exits with an error
 */
static int CopyTextToClipboardUnix(const std::string &text)
{
    constexpr unsigned READ = 0, WRITE = 1;
    struct stat sb;

    const char *env_display = getenv("DISPLAY");
    if (env_display == NULL || *env_display == 0)
    {
        // This will never work
        return 4;
    }
    const char *xclip_path = XCLIP;
    const char *env_xclip = getenv("NCPWSAFE_XCLIP");
    if (env_xclip != NULL && *env_xclip != 0)
    {
        xclip_path = env_xclip;
    }

    if ((stat(xclip_path, &sb)) == -1)
        return 1;

    int fd_stdin[2] = {-1, -1}, fd_stdout[2] = {-1, -1};
    if (pipe(fd_stdin) < 0)
    {
        return 2;
    }
    if (pipe(fd_stdout) < 0)
    {
        close(fd_stdin[READ]);
        close(fd_stdin[WRITE]);
        return 2;
    }

    int pid = fork();
    if (0 == pid)
    {
        // child continues here

        // redirect stdin
        if (dup2(fd_stdin[READ], STDIN_FILENO) == -1)
        {
            exit(COMMAND_NOT_FOUND);
        }
        // redirect stdout
        if (dup2(fd_stdout[WRITE], STDOUT_FILENO) == -1)
        {
            exit(COMMAND_NOT_FOUND);
        }

        // all these are for use by parent only
        close(fd_stdin[READ]);
        close(fd_stdin[WRITE]);
        close(fd_stdout[READ]);
        close(fd_stdout[WRITE]);

        // run child process image
        // replace this with any exec* function find easier to use ("man exec")
        // clang-format off
        const char *const args[]{
            xclip_path, 
            "-in", 
            "-selection", 
            "clipboard", 
            NULL
        };
        // clang-format on

        std::vector<const char *> env;

        char penv_display[128];
        snprintf(penv_display, 128, "DISPLAY=%s", env_display);
        env.push_back(penv_display);

        // XAUTHORITY=$HOME/.Xauthority
        // This seems to be required for X forwarding over SSH to work at all.
        // I have no idea why or when this is required exactly
        char penv_xauthority[128];
        const char *env_home = getenv("HOME");
        if (env_home != NULL && *env_home != 0)
        {
            snprintf(penv_xauthority, 128, "XAUTHORITY=%s/.Xauthority", env_home);
            if ((stat(penv_xauthority + 11, &sb)) != -1)
            {
                env.push_back(penv_xauthority);
            }
        }

        env.push_back(NULL);
        execve(xclip_path, const_cast<char **>(args), const_cast<char **>(env.data()));

        // if we get here at all, exec failed, but we are in the child
        // process, so just exit. _exit() skips atexit handlers and stdio
        // flushing, which belong to the parent
        _exit(COMMAND_NOT_FOUND);
    }
    else if (pid > 0)
    {
        // parent continues here

        // close unused file descriptors
        close(fd_stdin[READ]);
        close(fd_stdout[READ]);
        close(fd_stdout[WRITE]);

        // Ignore SIGPIPE while writing, so that a child that exits early
        // (e.g. exec failed) produces a write error instead of killing us
        struct sigaction sa_ignore{}, sa_old{};
        sa_ignore.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa_ignore, &sa_old);

        bool write_ok = true;
        for (size_t off = 0; off < text.size();)
        {
            ssize_t n = write(fd_stdin[WRITE], text.data() + off, text.size() - off);
            if (n < 0)
            {
                if (errno == EINTR) continue;
                write_ok = false;
                break;
            }
            off += static_cast<size_t>(n);
        }
        close(fd_stdin[WRITE]);

        sigaction(SIGPIPE, &sa_old, nullptr);

        // Reap the child so it does not linger as a zombie. xclip forks
        // into the background to serve the selection, so this returns
        // as soon as it has read its input
        int status = 0;
        while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {}
        if (!write_ok || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            return 5;
        }
    }
    else
    {
        // failed to create child
        close(fd_stdin[READ]);
        close(fd_stdin[WRITE]);
        close(fd_stdout[READ]);
        close(fd_stdout[WRITE]);

        return 3;
    }
    return 0;
}
#endif // #ifndef _WIN32

/** Copy to clipboard, report errors */
int CopyTextToClipboard(PWSafeApp &app, WINDOW *win, const std::string &text)
{
#ifndef _WIN32
    int result = CopyTextToClipboardUnix(text);
    if (result == 4)
    {
        MessageBox(app).Show(win, "The DISPLAY environment variable is not set.");
    }
    else if (result > 0)
    {
        MessageBox(app).Show(win, "An error occurred copying data to the clipboard");
    }
    return result;
#else
    int result = win32::CopyTextToClipboard(text);
    if (result != win32::CLIPBOARD_OK)
    {
        MessageBox(app).Show(win, "An error occurred copying data to the clipboard");
    }
    return result;
#endif
}

AccountsWin::AccountsWin(PWSafeApp &app, WINDOW *win) : app_(app), win_(win)
{
    // clang-format off
    app.GetCommandBar().Register(this, {
        {"Enter", "View", "View the account properties"}, 
        {~CBOPTS_READONLY, "^A", "Add", "Add a new account"},
        {~CBOPTS_READONLY, "^D", "Delete", "Delete an account"},
        {"^U", "Copy user", "Copy the account user name to the clipboard"},
        {"^P", "Copy password", "Copy the account password to the clipboard"},
        {~CBOPTS_READONLY, "^S", "Save and exit", "Save changes to the database and exit"},
        {"^X", "Exit", "Exit without saving changes"},
        {~CBOPTS_READONLY, "^C", "Change password", "Change the account database password"}
    });
    // clang-format on
}

void AccountsWin::SetSelection(const AccountRecord &record)
{
    list_.SelectRecord(record);
    Draw();
}

const AccountRecord *AccountsWin::GetSelection() const
{
    return list_.SelectedRecord();
}

void AccountsWin::SetPosition(const AccountList::Position &position)
{
    list_.SetPosition(position);
    Draw();
}

void AccountsWin::Rebuild()
{
    list_.Build(app_.GetDb().Records());
}

void AccountsWin::SelectUuid(const std::string &uuid)
{
    AccountRecords &records = app_.GetDb().Records();
    AccountRecords::iterator it = records.Find(FT_UUID, uuid);
    if (it != records.end())
    {
        list_.SelectRecord(*it);
    }
}

int AccountsWin::ListHeight() const
{
    return std::max(0, getmaxy(win_) - FIRST_ROW);
}

int AccountsWin::TitleColumnWidth(int width) const
{
    int longest = 0;
    for (const AccountList::Row &row : list_.Rows())
    {
        if (row.record != nullptr)
        {
            longest = std::max(longest, DisplayWidth(PrintableText(row.record->GetField(FT_TITLE, ""))));
        }
    }
    const int indent = list_.HasHeadings() ? 4 : 1;
    // At least two spaces between the title and the user name
    const int max_width = std::max(12, (width - indent) * 3 / 5);
    return std::clamp(longest + 2, 12, max_width);
}

void AccountsWin::DrawHeading(int y, const AccountList::Row &row, bool selected)
{
    const Theme &theme = app_.GetTheme();
    const int width = getmaxx(win_);
    const attr_t selection = theme.Attr(Role::SELECTION);
    auto attrs = [&](Role role) { return selected ? selection : theme.Attr(role); };

    if (selected)
    {
        mvwhline(win_, y, 0, ' ' | selection, width);
    }

    // " ▾ Banking  ──────  2 ", with at least two spaces around the rule
    const std::string count = std::to_string(row.count);
    const int count_x = width - static_cast<int>(count.size()) - 1;
    const std::string name = row.group.empty() ? "(no group)" : PrintableText(row.group.c_str());
    const std::string fitted = FitText(name, std::max(1, count_x - 2 - 3), theme.Sym(Symbol::ELLIPSIS));

    SetAttrs(win_, attrs(Role::HEADING));
    mvwaddstr(win_, y, 1, theme.Sym(row.collapsed ? Symbol::COLLAPSED : Symbol::EXPANDED));
    waddch(win_, ' ');
    waddstr(win_, fitted.c_str());

    SetAttrs(win_, selected ? selection : A_NORMAL);
    const int rule_x = 3 + DisplayWidth(fitted) + 2;
    const int rule_len = count_x - 2 - rule_x;
    if (rule_len > 0)
    {
        mvwhline(win_, y, rule_x, ACS_HLINE | attrs(Role::MUTED), rule_len);
    }
    SetAttrs(win_, attrs(Role::MUTED));
    mvwaddstr(win_, y, count_x, count.c_str());
    SetAttrs(win_, A_NORMAL);
}

void AccountsWin::DrawAccount(int y, const AccountList::Row &row, bool selected, int title_width)
{
    const Theme &theme = app_.GetTheme();
    const int width = getmaxx(win_);
    const attr_t selection = theme.Attr(Role::SELECTION);
    const char *ellipsis = theme.Sym(Symbol::ELLIPSIS);

    if (selected)
    {
        mvwhline(win_, y, 0, ' ' | selection, width);
    }

    const int indent = list_.HasHeadings() ? 4 : 1;
    const std::string title = FitText(PrintableText(row.record->GetField(FT_TITLE, "")), title_width - 2, ellipsis);
    SetAttrs(win_, selected ? selection : A_NORMAL);
    mvwaddstr(win_, y, indent, title.c_str());

    const char *user = row.record->GetField(FT_USER, "");
    const int user_x = indent + title_width;
    if (*user != '\0' && user_x < width - 1)
    {
        // The user name isn't bold in the selection
        SetAttrs(win_, selected ? selection & ~A_BOLD : theme.Attr(Role::MUTED));
        mvwaddstr(win_, y, user_x, FitText(PrintableText(user), width - user_x - 1, ellipsis).c_str());
    }
    SetAttrs(win_, A_NORMAL);
}

void AccountsWin::Draw()
{
    if (win_ == nullptr)
        return;
    werase(win_);

    const Theme &theme = app_.GetTheme();
    const int width = getmaxx(win_);
    const auto &rows = list_.Rows();
    if (rows.empty())
    {
        const char *hint = app_.GetDb().ReadOnly() ? "No accounts" : "No accounts, press ^A to add one";
        ScopedAttr muted(win_, theme, Role::MUTED);
        mvwaddstr(win_, FIRST_ROW, 1, FitText(hint, width - 2, theme.Sym(Symbol::ELLIPSIS)).c_str());
        return;
    }

    const int height = ListHeight();
    const int top = list_.ScrollTop(height);
    const int title_width = TitleColumnWidth(width);
    for (int y = 0; y < height && top + y < static_cast<int>(rows.size()); ++y)
    {
        const int index = top + y;
        const AccountList::Row &row = rows[static_cast<size_t>(index)];
        const bool selected = index == list_.Selected();
        if (row.kind == AccountList::RowKind::HEADING)
            DrawHeading(FIRST_ROW + y, row, selected);
        else
            DrawAccount(FIRST_ROW + y, row, selected, title_width);
    }
}

void AccountsWin::OnResize()
{
    Draw();
    touchwin(win_);
}

void AccountsWin::InitTUI()
{
    panel_ = new_panel(win_);

    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);

    save_cursor_ = curs_set(0);
}

void AccountsWin::EndTUI()
{
    del_panel(panel_);
    panel_ = nullptr;
    win_ = nullptr;

    curs_set(save_cursor_);
}

void AccountsWin::UpdateList(const AccountRecord old_record, const AccountRecord &new_record)
{
    // `old_record` is a copy: Save() sorts the records, so a reference to the old record
    // could refer to another account afterwards
    AccountRecords &records = app_.GetDb().Records();
    const std::string uuid = records.Save(new_record)->GetField(FT_UUID, "");
    if (uuid != old_record.GetField(FT_UUID, ""))
    {
        // Saved as a new record, remove the old one
        records.Delete(old_record);
    }

    // The records may have moved, the rows point to the wrong records
    Rebuild();
    SelectUuid(uuid);
}

/** View or edit an account entry */
DialogResult AccountsWin::ShowAccountRecord(AccountRecord &record)
{
    auto &db = app_.GetDb();
    bool read_only = db.ReadOnly();
    AccountDetailsDlg details{app_, record};

    DialogResult result = details.Show(win_, read_only);
    if (result == DialogResult::OK)
    {
        record = details.GetItem();
    }

    SetCommandBar();

    return result;
}

/** Display an account item dialog */
DialogResult AccountsWin::AddNewEntry()
{
    AccountDb &db = app_.GetDb();

    AccountRecord record;
    AccountDetailsDlg details(app_, record);

    DialogResult result = details.Show(win_);
    if (result == DialogResult::OK)
    {
        const AccountRecord &new_record = details.GetItem();
        const std::string new_uuid = db.Records().Save(new_record)->GetField(FT_UUID, "");
        Rebuild();
        SelectUuid(new_uuid);
    }

    SetCommandBar();

    return result;
}

/** Delete an account, after asking */
DialogResult AccountsWin::DeleteEntry(const AccountRecord &record)
{
    AccountDb &db = app_.GetDb();

    app_.GetCommandBar().Show(CommandBarWin::YES_NO);

    auto &records = db.Records();
    // Name the user only if another account has the same title
    bool unique = std::none_of(records.begin(), records.end(), [&record](const AccountRecord &other) {
        return &other != &record && FieldCompare(FT_TITLE, other, record);
    });
    std::string msg = std::string("Delete account ").append(record.GetField(FT_TITLE));
    if (!unique && record.GetField(FT_USER))
    {
        msg.append(" with user ").append(record.GetField(FT_USER));
    }
    msg.append("?");

    DialogResult result = MessageBox(app_).Show(win_, msg.c_str(), &YesNoKeyHandler);
    if (result == DialogResult::YES)
    {
        AccountRecords::iterator it = records.Find(FT_UUID, record.GetField(FT_UUID));
        const int index = list_.Selected();
        if (it != records.end() && records.Delete(it))
        {
            // Select the row that took the deleted account's place
            Rebuild();
            list_.SelectIndex(index);
        }
    }

    SetCommandBar();

    return result;
}

static bool YesNoCancelKeyHandler(int ch, DialogResult &result)
{
    ch = tolower(ch);
    if (ch == 'y')
    {
        result = DialogResult::YES;
        return true;
    }
    else if (ch == 'n')
    {
        result = DialogResult::NO;
        return true;
    }
    else if (ch == KEY_CTRL('C'))
    {
        result = DialogResult::CANCEL;
        return true;
    }
    return false;
}

/** Save the database to the current file. */
bool AccountsWin::Save()
{
    app_.GetCommandBar().Show({Action::YES, Action::NO, {"^X", "Cancel"}});

    bool retval = false;

    int rc = RC_FAILURE;
    while (rc != RC_SUCCESS && rc != RC_USER_CANCEL)
    {
        rc = app_.Save();
        if (rc == RC_SUCCESS)
        {
            retval = true;
        }
        else
        {
            AccountDb &db = app_.GetDb();
            std::string msg("An error occurred writing the database to file\n");
            msg.append(db.DbPathname()).append(". Retry?");
            DialogResult dr = MessageBox(app_).Show(win_, msg.c_str(), &YesNoCancelKeyHandler);
            if (dr == DialogResult::NO)
            {
                rc = RC_SUCCESS;
                retval = true;
                break;
            }
            else if (dr == DialogResult::CANCEL)
            {
                rc = RC_USER_CANCEL;
                break;
            }

            redrawwin(win_);
        }
    }

    SetCommandBar();

    return retval;
}

/** Ask for confirmation to discard changes */
bool AccountsWin::DiscardChanges()
{
    bool retval = true;

    if (app_.GetDb().IsDirty())
    {
        app_.GetCommandBar().Show(CommandBarWin::YES_NO);

        const char *msg = "The database has changed. Discard changes?";
        retval = MessageBox(app_).Show(win_, msg, &YesNoKeyHandler) == DialogResult::YES;

        redrawwin(win_);
        SetCommandBar();
    }

    return retval;
}


/** Show the password item list */
DialogResult AccountsWin::Show()
{
    SetCommandBar();

    InitTUI();
    Rebuild();
    Draw();

    update_panels();
    doupdate();

    DialogResult result;
    {
        ScopedResizeHandler resize_handler(app_, [this]() { OnResize(); });
        result = ProcessInput();
    }

    EndTUI();

    return result;
}

void AccountsWin::SetCommandBar()
{
    bool read_only = app_.GetDb().ReadOnly();
    const unsigned char opts = read_only ? CBOPTS_READONLY : ~CBOPTS_READONLY;
    app_.GetCommandBar().Show(this, opts);
}

DialogResult AccountsWin::ProcessInput()
{
    bool read_only = app_.GetDb().ReadOnly();
    DialogResult result = DialogResult::CANCEL;
    int c;
    while ((c = wgetch(win_)) != ERR)
    {
        switch (c)
        {
        case KEY_RESIZE:
        {
            app_.HandleResize();
            break;
        }
        case KEY_CTRL('A'):
        {
            if (!read_only)
            {
                AddNewEntry();
            }
            break;
        }
        case KEY_CTRL('C'):
        {
            if (!read_only)
            {
                ChangeDbPasswordDlg dialog(app_);
                if (dialog.Show(win_) == DialogResult::OK)
                {
                    const std::string &password = dialog.GetPassword();
                    const std::string &new_password = dialog.GetNewPassword();
                    app_.GetDb().Password() = password;
                    if (ChangeDbPasswordCommand{app_, new_password}.Execute() != RC_SUCCESS)
                    {
                        MessageBox(app_).Show(win_, "An error occurred changing the account database password.");
                    }
                }
            }
            break;
        }
        case KEY_CTRL('D'):
        {
            // Nothing is selected in an empty list, and a group heading has no account
            const AccountRecord *record = list_.SelectedRecord();
            if (!read_only && record != nullptr)
            {
                DeleteEntry(*record);
            }
            break;
        }
        case KEY_CTRL('S'):
        {
            if (!read_only && Save())
            {
                result = DialogResult::OK;
                goto done;
            }
            break;
        }
        case KEY_CTRL('X'):
        {
            if (read_only || DiscardChanges())
            {
                result = DialogResult::CANCEL;
                goto done;
            }
            break;
        }
        case KEY_CTRL('U'):
        case KEY_CTRL('P'):
        {
            const AccountRecord *record = list_.SelectedRecord();
            if (record != nullptr)
            {
                const std::string str = record->GetField(c == KEY_CTRL('U') ? FT_USER : FT_PASSWORD, "");
                if (CopyTextToClipboard(app_, win_, str) > 0)
                {
                    app_.GetCommandBar().Show(this);
                }
            }
            break;
        }
        case '/':
        {
            app_.DoSearch();
            app_.GetCommandBar().Show(this);
            break;
        }
        case ' ':
        {
            list_.Toggle();
            break;
        }
        case '\n':
        {
            const AccountRecord *record = list_.SelectedRecord();
            if (record == nullptr)
            {
                // A group heading, or the list is empty
                list_.Toggle();
                break;
            }
            AccountRecord copy{*record};
            if (ShowAccountRecord(copy) == DialogResult::OK && !read_only)
            {
                UpdateList(*record, copy);
            }
            break;
        }
        case KEY_HOME:
            list_.MoveFirst();
            break;
        case KEY_END:
            list_.MoveLast();
            break;
        case KEY_UP:
            list_.MoveUp();
            break;
        case KEY_DOWN:
            list_.MoveDown();
            break;
        case KEY_LEFT:
            list_.Left();
            break;
        case KEY_RIGHT:
            list_.Right();
            break;
        case KEY_PPAGE:
            list_.MovePageUp(ListHeight() - 1);
            break;
        case KEY_NPAGE:
            list_.MovePageDown(ListHeight() - 1);
            break;
        default:
            break;
        }
        Draw();
    }

done:
    return result;
}
