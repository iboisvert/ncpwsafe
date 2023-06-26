/* Copyright 2020 Ian Boisvert */
/**
 * AccountsWin implements the main scrolling list of
 * account entries.
 */
#ifndef _WINDOWS
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif // #ifndef _WINDOWS

#include "AccountDetailsDlg.h"
#include "AccountsWin.h"
#include "ChangePasswordDlg.h"
#include "ChangeDbPasswordCommand.h"
#include "ChangeDbPasswordDlg.h"
#include "MessageBox.h"
#include "PWSafeApp.h"
#include "Utils.h"

// TODO IMB 2022-11-21 Display page "x/y" bottom right corner

// item_new rejects an empty string for the item name
static constexpr const char *EMPTY_MENU_ITEM = " ";

#ifndef _WINDOWS
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
            exit(errno);
        }
        // redirect stdout
        if (dup2(fd_stdout[WRITE], STDOUT_FILENO) == -1)
        {
            exit(errno);
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
            XCLIP, 
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
        int result = execve(XCLIP, const_cast<char **>(args), const_cast<char **>(env.data()));

        // if we get here at all, an error occurred, but we are in the child
        // process, so just exit
        exit(result);
    }
    else if (pid > 0)
    {
        // parent continues here

        // close unused file descriptors
        close(fd_stdin[READ]);
        close(fd_stdout[READ]);
        close(fd_stdout[WRITE]);

        // Include error check here
        __attribute__((unused)) ssize_t result = write(fd_stdin[WRITE], text.c_str(), text.size());

        // done with these in this example program, you would normally keep these
        // open of course as long as you want to talk to the child
        close(fd_stdin[WRITE]);
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
#endif // #ifndef _WINDOWS

/** Copy to clipboard, report errors */
int CopyTextToClipboard(PWSafeApp &app, WINDOW *win, const std::string &text)
{
#ifndef _WINDOWS
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
    return -1;
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

ITEM *AccountsWin::FindItem(const AccountRecord &record) const
{
    ITEM *pitem = nullptr;
    auto it = std::find_if(
        menu_items_.begin(), menu_items_.end(), [&record](const ITEM *pitem)
        { return item_userptr(pitem) == &record; });
    if (it != menu_items_.end())
    {
        pitem = *it;
    }
    return pitem;
}

void AccountsWin::SetSelection(const AccountRecord &record) const
{
    ITEM *pitem = FindItem(record);
    if (pitem != nullptr)
    {
        set_current_item(menu_, pitem);
    }
}

const AccountRecord *AccountsWin::GetSelection() const
{
    AccountRecord *pcid = nullptr;
    ITEM *item = current_item(menu_);
    if (item != nullptr)
    {
        pcid = reinterpret_cast<AccountRecord *>(item_userptr(item));
    }
    return pcid;
}

static ITEM *CreateBlankMenuItem()
{
    ITEM *item = new_item(EMPTY_MENU_ITEM, 0);
    item_opts_off(item, O_SELECTABLE);
    return item;
}

static bool IsBlankMenuItem(const ITEM *item)
{
    assert(item);
    return item_userptr(item) == nullptr && item_name(item) == EMPTY_MENU_ITEM;
}

static bool IsGroupMenuItem(const ITEM *item)
{
    assert(item);
    return item_userptr(item) == nullptr && item_name(item) != EMPTY_MENU_ITEM;
}

void AccountsWin::CreateMenu()
{
    menu_items_.clear();

    auto &records = app_.GetDb().Records();
    const char *lastGroup = "";

    ITEM *item;
    int itemIndex = 0;
    // Accounts are sorted by group and title
    for (const AccountRecord &record : records)
    {
        const char *group = record.GetField(FT_GROUP, "");
        if (strcmp(lastGroup, group) != 0)
        {
            lastGroup = group;

            // Align groups on column 0
            while ((itemIndex % NCOLS) != 0)
            {
                item = CreateBlankMenuItem();
                ++itemIndex;
                menu_items_.push_back(item);
            }

            // Insert blank row before group, unless group is first menu item
            if (itemIndex > 0)
            {
                for (int i = 0; i < NCOLS; ++i)
                {
                    item = CreateBlankMenuItem();
                    ++itemIndex;
                    menu_items_.push_back(item);
                }
            }

            item = new_item(group, 0);
            ++itemIndex;
            menu_items_.push_back(item);

            // Align first account in group on column 0
            while ((itemIndex % NCOLS) != 0)
            {
                item = CreateBlankMenuItem();
                ++itemIndex;
                menu_items_.push_back(item);
            }
        }

        const char *title = record.GetField(FT_TITLE, EMPTY_MENU_ITEM);
        const char *user = record.GetField(FT_USER, EMPTY_MENU_ITEM);
        item = new_item(title, user);
        ++itemIndex;
        set_item_userptr(item, const_cast<AccountRecord *>(&record));
        menu_items_.push_back(item);
    }
    menu_items_.push_back(nullptr);

    menu_ = new_menu(menu_items_.data());
    set_menu_grey(menu_, A_NORMAL);

    set_menu_win(menu_, win_);
    int beg_y, beg_x, max_y, max_x;
    getbegyx(win_, beg_y, beg_x);
    getmaxyx(win_, max_y, max_x);
    int nlines = max_y - beg_y + 1, ncols = max_x - beg_x;
    int menuWin_nlines = nlines - 1, menuWin_ncols = ncols - 2;
    menu_win_ = derwin(win_, menuWin_nlines, menuWin_ncols, /*begin_y*/ 1, /*begin_x*/ 1);
    set_menu_sub(menu_, menu_win_);

    menu_opts_off(menu_, O_SHOWDESC);
    set_menu_mark(menu_, nullptr);
    set_menu_format(menu_, menuWin_nlines, NCOLS);

    post_menu(menu_);

    // TESTING repost with 2 cols to see if it works
    // status = unpost_menu(menu);
    // status = set_menu_format(menu, LINES - 2, cols);
    // status = post_menu(menu);
}

static void DestroyMenu(MENU *menu)
{
    ITEM **menuItems = menu_items(menu);
    if (menuItems)
    {
        for (ITEM **ppitem = menuItems; *ppitem != nullptr; ++ppitem)
        {
            ITEM *pitem = *ppitem;
            const char *name = item_name(pitem);
            if (name && name != EMPTY_STR)
            {
                delete[] name;
            }
            const char *description = item_description(pitem);
            if (description)
            {
                delete[] description;
            }
            free_item(pitem);
        }
    }
    free_menu(menu);
}

// Copied from ui/wxWidgets/MenuEditHandlers.cpp
// void PasswordSafeFrame::DoCopyPassword(AccountRecord &item)
// {
//   if (!item.IsDependent())
//     Clipboard::GetInstance()->SetData(item.GetPassword());
//   else
//   {
//     const CUUID &base = item.GetBaseUUID();
//     const std::string &passwd = m_db.GetEntry(m_db.Find(base)).GetPassword();
//     Clipboard::GetInstance()->SetData(passwd);
//   }
//   UpdateLastClipboardAction(AccountRecord::FieldType::PASSWORD);
//   UpdateAccessTime(item);
// }

void AccountsWin::InitTUI()
{
    panel_ = new_panel(win_);

    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);

    save_cursor_ = curs_set(0);
}

void AccountsWin::EndTUI()
{
    DestroyMenu(menu_);
    menu_ = nullptr;
    del_panel(panel_);
    panel_ = nullptr;
    delwin(menu_win_);
    menu_win_ = nullptr;
    win_ = nullptr;

    curs_set(save_cursor_);
}

static void NavigateUp(MENU *menu, int cols, const std::vector<ITEM *> &items)
{
    int idx = item_index(current_item(menu));
    // If menu item above current item is before start of list or a blank item,
    // select the first non-blank menu item to the left in the row above
    if (idx - cols < 0 || IsBlankMenuItem(items[idx - cols]))
    {
        idx = (idx / cols) * cols - cols;
        while (idx > -1)
        {
            if (!IsBlankMenuItem(items[idx]))
            {
                set_current_item(menu, items[idx]);
                break;
            }
            idx -= cols;
        }
    }
    else
    {
        set_current_item(menu, items[idx - cols]);
    }
}

static void NavigateDown(MENU *menu, int cols, const std::vector<ITEM *> &items)
{
    int idx = item_index(current_item(menu));
    int count = item_count(menu);
    // If menu item below current item is past the end of list or a blank item,
    // select the first non-blank menu item in the row below
    if (idx + cols > count || IsBlankMenuItem(items[idx + cols]))
    {
        idx = (idx / cols) * cols + cols;
        while (idx < count)
        {
            if (!IsBlankMenuItem(items[idx]))
            {
                set_current_item(menu, items[idx]);
                break;
            }
            ++idx;
        }
    }
    else
    {
        set_current_item(menu, items[idx + cols]);
    }
}

static void NavigateRight(MENU *menu, int /*cols*/, const std::vector<ITEM *> &items)
{
    int idx = item_index(current_item(menu));
    int count = item_count(menu);
    while (++idx < count)
    {
        if (!IsBlankMenuItem(items[idx]))
        {
            set_current_item(menu, items[idx]);
            break;
        }
    }
}

static void NavigateLeft(MENU *menu, int /*cols*/, const std::vector<ITEM *> &items)
{
    int idx = item_index(current_item(menu));
    while (--idx > -1)
    {
        if (!IsBlankMenuItem(items[idx]))
        {
            set_current_item(menu, items[idx]);
            break;
        }
    }
}

static void NavigatePageUp(MENU *menu, int cols, const std::vector<ITEM *> &items)
{
    menu_driver(menu, REQ_SCR_UPAGE);
    int idx = item_index(current_item(menu));
    int count = item_count(menu);
    if (IsBlankMenuItem(items[idx]))
    {
        // Move down rows until a non-blank menu item is found
        idx += cols;
        while (idx < count)
        {
            if (!IsBlankMenuItem(items[idx]))
            {
                set_current_item(menu, items[idx]);
                break;
            }
            idx += cols;
        }
    }
}

static void NavigatePageDown(MENU *menu, int cols, const std::vector<ITEM *> &items)
{
    menu_driver(menu, REQ_SCR_DPAGE);
    int idx = item_index(current_item(menu));
    if (IsBlankMenuItem(items[idx]))
    {
        // Move up rows until a non-blank menu item is found
        idx -= cols;
        while (idx > -1)
        {
            if (!IsBlankMenuItem(items[idx]))
            {
                set_current_item(menu, items[idx]);
                break;
            }
            idx -= cols;
        }
    }
}

static bool EqualMenuData(const AccountRecord &a, const AccountRecord &b)
{
    return FieldCompare(FT_GROUP, a, b)
        && FieldCompare(FT_TITLE, a, b)
        && FieldCompare(FT_USER, a, b);
}

void AccountsWin::UpdateMenu(const AccountRecord &old_record, const AccountRecord &new_record)
{
    if (!EqualMenuData(old_record, new_record))
    {
        AccountRecords &records = app_.GetDb().Records();
        if (records.Add(new_record))
        {
            records.Delete(old_record);

            DestroyMenu(menu_);
            CreateMenu();

            // Reset selection
            SetSelection(new_record);
        }
    }
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
        db.Records().Add(new_record);

        DestroyMenu(menu_);
        CreateMenu();

        // Reset selection
        const std::string &new_uuid = new_record.GetField(FT_UUID);
        auto it = std::find_if(menu_items_.begin(), menu_items_.end(), [&new_uuid](const ITEM *pitem) { 
            return item_userptr(pitem) && reinterpret_cast<AccountRecord *>(item_userptr(pitem))->GetField(FT_UUID) == new_uuid; 
        });
        assert(it != menu_items_.end());
        if (it != menu_items_.end())
        {
            set_current_item(menu_, *it);
        }
    }

    SetCommandBar();

    return result;
}

/** Display an account item dialog */
DialogResult AccountsWin::DeleteEntry(const ITEM *pitem)
{
    AccountDb &db = app_.GetDb();

    const AccountRecord *prec = reinterpret_cast<AccountRecord *>(item_userptr(pitem));
    assert(prec != nullptr);
    if (!prec)
    {
        return DialogResult::NO;
    }

    app_.GetCommandBar().Show(CommandBarWin::YES_NO);

    auto &records = db.Records();
    bool unique = std::all_of(records.begin(), records.end(), [prec](const AccountRecord &record) { 
        return FieldCompare(FT_TITLE, record, *prec) || &record == prec; 
    });
    std::string msg = std::string("Delete account ").append(prec->GetField(FT_TITLE));
    if (!unique && prec->GetField(FT_USER))
    {
        msg.append(" with user ").append(prec->GetField(FT_USER));
    }
    msg.append("?");

    DialogResult result = MessageBox(app_).Show(win_, msg.c_str(), &YesNoKeyHandler);
    if (result == DialogResult::YES)
    {
        AccountRecords &records = db.Records();
        AccountRecords::iterator it = records.Find(FT_UUID, prec->GetField(FT_UUID));
        if (it != records.end() && records.Delete(it))
        {
            DestroyMenu(menu_);
            CreateMenu();
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

    app_.GetCommandBar().Show(CommandBarWin::YES_NO);

    if (app_.GetDb().IsDirty())
    {
        const char *msg = "The database has changed. Discard changes?";
        retval = MessageBox(app_).Show(win_, msg, &YesNoKeyHandler) == DialogResult::YES;

        redrawwin(win_);
    }

    SetCommandBar();

    return retval;
}

/** Show the password item list */
DialogResult AccountsWin::Show()
{
    SetCommandBar();

    // TODO check through items for items that have same group and title
    // if matching items found, enable display of username

    InitTUI();
    werase(win_);

    CreateMenu();

    update_panels();
    doupdate();

    DialogResult result = ProcessInput();

    EndTUI();

    win_ = nullptr;

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
            if (!read_only)
            {
                ITEM *item = current_item(menu_);
                DeleteEntry(item);
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
        {
            ITEM *item = current_item(menu_);
            if (!IsGroupMenuItem(item))
            {
                const AccountRecord *record = GetAccountRecordFromMenuItem(item);
                const std::string &str = record->GetField(FT_USER);
                if (CopyTextToClipboard(app_, win_, str) > 0)
                {
                    app_.GetCommandBar().Show(this);
                }
            }
            break;
        }
        case KEY_CTRL('P'):
        {
            ITEM *item = current_item(menu_);
            if (!IsGroupMenuItem(item))
            {
                const AccountRecord *record = GetAccountRecordFromMenuItem(item);
                const std::string &str = record->GetField(FT_PASSWORD);
                if (CopyTextToClipboard(app_, win_, str) > 0)
                {
                    app_.GetCommandBar().Show(this);
                }
            }
            break;
        }
        case '/':
        {
            using std::placeholders::_1;
            app_.DoSearch();
            app_.GetCommandBar().Show(this);
            break;
        }
            // case 'a': {
            //     // Check for Group/Username/Title uniqueness
            //     // if (m_db.Find(m_Item.GetGroup(), m_Item.GetTitle(), m_Item.GetUser()) !=
            //     //     m_Core.GetEntryEndIter())
            //     // {
            //     //   wxMessageDialog msg(
            //     //       this,
            //     //       _("An entry or shortcut with the same Group, Title and Username already exists."),
            //     //       _("Error"),
            //     //       wxOK | wxICON_ERROR);
            //     //   msg.ShowModal();
            //     //   return;
            //     // }
            //     break;
            // }

        case '\n':
        {
            ITEM *item = current_item(menu_);
            assert(!IsBlankMenuItem(item));
            if (IsGroupMenuItem(item))
            {
                // DisplayGroup(*record);
            }
            else
            {
                const AccountRecord *record = GetAccountRecordFromMenuItem(item);
                assert(record != nullptr);
                AccountRecord copy{*record};
                if (ShowAccountRecord(copy) == DialogResult::OK && !read_only)
                {
                    UpdateMenu(*record, copy);
                }
            }

            break;
        }

        case KEY_HOME:
        {
            menu_driver(menu_, REQ_FIRST_ITEM);
            break;
        }

        case KEY_END:
        {
            menu_driver(menu_, REQ_LAST_ITEM);
            break;
        }

        case KEY_UP:
        {
            NavigateUp(menu_, NCOLS, menu_items_);
            break;
        }

        case KEY_DOWN:
        {
            NavigateDown(menu_, NCOLS, menu_items_);
            break;
        }

        case KEY_RIGHT:
        {
            NavigateRight(menu_, NCOLS, menu_items_);
            break;
        }

        case KEY_LEFT:
        {
            NavigateLeft(menu_, NCOLS, menu_items_);
            break;
        }

        case KEY_PPAGE:
        {
            NavigatePageUp(menu_, NCOLS, menu_items_);
            break;
        }

        case KEY_NPAGE:
        {
            NavigatePageDown(menu_, NCOLS, menu_items_);
            break;
        }

        default:
            break;
        }
    }

done:
    return result;
}
