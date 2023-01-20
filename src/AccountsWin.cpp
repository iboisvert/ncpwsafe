/* Copyright 2020 Ian Boisvert */
/**
 * AccountsWin implements the main scrolling list of
 * account entries.
 */
#ifndef _WINDOWS
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif  //#ifndef _WINDOWS

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
static constexpr const char *EMPTY_STR = " ";

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
#endif  //#ifndef _WINDOWS

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

AccountsWin::AccountsWin(PWSafeApp &app, WINDOW *win) : m_app(app), m_win(win)
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

ITEM *AccountsWin::FindItem(const CItemData &cid) const
{
    ITEM *pitem = nullptr;
    auto it = std::find_if(
        m_menuItems.begin(), m_menuItems.end(), [&cid](const ITEM *pitem) { return item_userptr(pitem) == &cid; });
    if (it != m_menuItems.end())
    {
        pitem = *it;
    }
    return pitem;
}

void AccountsWin::SetSelection(const CItemData &cid) const
{
    ITEM *pitem = FindItem(cid);
    if (pitem != nullptr)
    {
        set_current_item(m_menu, pitem);
    }
}

const CItemData *AccountsWin::GetSelection() const
{
    CItemData *pcid = nullptr;
    ITEM *item = current_item(m_menu);
    if (item != nullptr)
    {
        pcid = reinterpret_cast<CItemData *>(item_userptr(item));
    }
    return pcid;
}

static ITEM *CreateBlankMenuItem()
{
    ITEM *item = new_item(EMPTY_STR, 0);
    item_opts_off(item, O_SELECTABLE);
    return item;
}

static bool IsBlankMenuItem(const ITEM *item)
{
    assert(item);
    return item_userptr(item) == nullptr && item_name(item) == EMPTY_STR;
}

static bool IsGroupMenuItem(const ITEM *item)
{
    assert(item);
    return item_userptr(item) == nullptr && item_name(item) != EMPTY_STR;
}

void AccountsWin::CreateMenu()
{
    m_menuItems.clear();

    auto &accounts = m_app.GetAccountsCollection();
    accounts.Refresh();
    std::string lastGroup;

    ITEM *item;
    int itemIndex = 0;
    // Accounts are sorted by group and title
    for (CItemData &cid : accounts)
    {
        std::string group = cid.GetGroup();
        if (group != lastGroup)
        {
            lastGroup = group;

            // Align groups on column 0
            while ((itemIndex % m_ncols) != 0)
            {
                item = CreateBlankMenuItem();
                ++itemIndex;
                m_menuItems.push_back(item);
            }

            // Insert blank row before group, unless group is first menu item
            if (itemIndex > 0)
            {
                for (int i = 0; i < m_ncols; ++i)
                {
                    item = CreateBlankMenuItem();
                    ++itemIndex;
                    m_menuItems.push_back(item);
                }
            }

            item = new_item(group.c_str(), 0);
            ++itemIndex;
            m_menuItems.push_back(item);

            // Align first account in group on column 0
            while ((itemIndex % m_ncols) != 0)
            {
                item = CreateBlankMenuItem();
                ++itemIndex;
                m_menuItems.push_back(item);
            }
        }

        std::string title = cid.GetTitle();
        std::string user = cid.GetUser();
        item = new_item(title.c_str(), user.c_str());
        ++itemIndex;
        set_item_userptr(item, &cid);
        m_menuItems.push_back(item);
    }
    m_menuItems.push_back(nullptr);

    m_menu = new_menu(m_menuItems.data());
    set_menu_grey(m_menu, A_NORMAL);

    set_menu_win(m_menu, m_win);
    int beg_y, beg_x, max_y, max_x;
    getbegyx(m_win, beg_y, beg_x);
    getmaxyx(m_win, max_y, max_x);
    int nlines = max_y - beg_y + 1, ncols = max_x - beg_x;
    int menuWin_nlines = nlines - 1, menuWin_ncols = ncols - 2;
    m_menuWin = derwin(m_win, menuWin_nlines, menuWin_ncols, /*begin_y*/ 1, /*begin_x*/ 1);
    set_menu_sub(m_menu, m_menuWin);

    menu_opts_off(m_menu, O_SHOWDESC);
    set_menu_mark(m_menu, nullptr);
    set_menu_format(m_menu, menuWin_nlines, m_ncols);

    post_menu(m_menu);

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
// void PasswordSafeFrame::DoCopyPassword(CItemData &item)
// {
//   if (!item.IsDependent())
//     Clipboard::GetInstance()->SetData(item.GetPassword());
//   else
//   {
//     const CUUID &base = item.GetBaseUUID();
//     const std::string &passwd = m_core.GetEntry(m_core.Find(base)).GetPassword();
//     Clipboard::GetInstance()->SetData(passwd);
//   }
//   UpdateLastClipboardAction(CItemData::FieldType::PASSWORD);
//   UpdateAccessTime(item);
// }

void AccountsWin::InitTUI()
{
    m_panel = new_panel(m_win);

    // Enable keypad so curses interprets function keys
    keypad(m_win, TRUE);

    m_saveCursor = curs_set(0);
}

void AccountsWin::EndTUI()
{
    DestroyMenu(m_menu);
    m_menu = nullptr;
    del_panel(m_panel);
    m_panel = nullptr;
    delwin(m_menuWin);
    m_menuWin = nullptr;
    m_win = nullptr;

    curs_set(m_saveCursor);
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

static bool EqualMenuData(const CItemData &a, const CItemData &b)
{
    return a.GetGroup() == b.GetGroup() && a.GetTitle() == b.GetTitle() && a.GetUser() == b.GetUser();
}

/** View or edit an account entry */
DialogResult AccountsWin::ShowAccountRecord(CItemData &itemData)
{
    PWScore &core = m_app.GetCore();
    bool readOnly = core.IsReadOnly();
    AccountDetailsDlg details(m_app, itemData);

    DialogResult result = details.Show(m_win, readOnly);
    if (result == DialogResult::OK && !readOnly)
    {
        const CItemData &newItemData = details.GetItem(), oldItemData = itemData;
        core.Execute(EditEntryCommand::Create(&core, itemData, newItemData));

        if (!EqualMenuData(oldItemData, newItemData))
        {
            DestroyMenu(m_menu);
            CreateMenu();

            // Reset selection
            SetSelection(itemData);
        }
    }

    SetCommandBar();

    return result;
}

/** Display an account item dialog */
DialogResult AccountsWin::AddNewEntry()
{
    PWScore &core = m_app.GetCore();

    CItemData itemData;
    itemData.CreateUUID();

    AccountDetailsDlg details(m_app, itemData);

    DialogResult result = details.Show(m_win);
    if (result == DialogResult::OK)
    {
        const CItemData &newItemData = details.GetItem();
        int status = core.Execute(AddEntryCommand::Create(&core, newItemData));
        if (status == PWScore::SUCCESS)
        {
            DestroyMenu(m_menu);
            CreateMenu();

            // Reset selection
            const pws_os::CUUID newUuid = newItemData.GetUUID();
            auto it = std::find_if(m_menuItems.begin(), m_menuItems.end(), [&newUuid](const ITEM *pitem) {
                return item_userptr(pitem) && reinterpret_cast<CItemData *>(item_userptr(pitem))->GetUUID() == newUuid;
            });
            assert(it != m_menuItems.end());
            if (it != m_menuItems.end())
            {
                set_current_item(m_menu, *it);
            }
        }
    }

    SetCommandBar();

    return result;
}

/** Display an account item dialog */
DialogResult AccountsWin::DeleteEntry(const ITEM *pitem)
{
    PWScore &core = m_app.GetCore();

    const CItemData *pItemData = reinterpret_cast<CItemData *>(item_userptr(pitem));
    assert(pItemData != nullptr);

    m_app.GetCommandBar().Show(CommandBarWin::YES_NO);

    auto &accounts = m_app.GetAccountsCollection();
    bool unique = std::all_of(accounts.begin(), accounts.end(),
        [pItemData](CItemData &cid) { return cid.GetTitle() != pItemData->GetTitle() || &cid == pItemData; });
    std::string msg = std::string("Delete account ").append(pItemData->GetTitle());
    if (!unique && pItemData->IsUserSet())
    {
        msg.append(" with user ").append(pItemData->GetUser());
    }
    msg.append("?");

    DialogResult result = MessageBox(m_app).Show(m_win, msg.c_str(), &YesNoKeyHandler);
    if (result == DialogResult::YES)
    {
        int status = core.Execute(DeleteEntryCommand::Create(&core, *pItemData));
        if (status == PWScore::SUCCESS)
        {
            DestroyMenu(m_menu);
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
    m_app.GetCommandBar().Show({Action::YES, Action::NO, {"^X", "Cancel"}});

    bool retval = false;

    ResultCode rc = ResultCode::FAILURE;
    while (rc != ResultCode::SUCCESS && rc != ResultCode::USER_CANCEL)
    {
        rc = m_app.Save();
        if (rc == ResultCode::SUCCESS)
        {
            retval = true;
        }
        else
        {
            PWScore &core = m_app.GetCore();
            std::string msg("An error occurred writing the database to file\n");
            msg.append(core.GetCurFile()).append(". Retry?");
            DialogResult dr = MessageBox(m_app).Show(m_win, msg.c_str(), &YesNoCancelKeyHandler);
            if (dr == DialogResult::NO)
            {
                rc = ResultCode::SUCCESS;
                retval = true;
                break;
            }
            else if (dr == DialogResult::CANCEL)
            {
                rc = ResultCode::USER_CANCEL;
                break;
            }

            redrawwin(m_win);
        }
    }

    SetCommandBar();

    return retval;
}

/** Ask for confirmation to discard changes */
bool AccountsWin::DiscardChanges()
{
    bool retval = true;

    m_app.GetCommandBar().Show(CommandBarWin::YES_NO);

    if (m_app.GetCore().HasDBChanged())
    {
        const char *msg = "The database has changed. Discard changes?";
        retval = MessageBox(m_app).Show(m_win, msg, &YesNoKeyHandler) == DialogResult::YES;

        redrawwin(m_win);
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

    // m_InitialTreeDisplayStatusAtOpen = true;
    // Show();
    // wxGetApp().recentDatabases().AddFileToHistory(fname);
    // }
    //   return retval;
    // }
    // else
    //   return PWScore::USER_CANCEL;

    InitTUI();
    werase(m_win);

    CreateMenu();

    update_panels();
    doupdate();

    DialogResult result = ProcessInput();

    EndTUI();

    m_win = nullptr;

    return result;
}

void AccountsWin::SetCommandBar()
{
    bool readOnly = m_app.GetCore().IsReadOnly();
    const unsigned char opts = readOnly ? CBOPTS_READONLY : ~CBOPTS_READONLY;
    m_app.GetCommandBar().Show(this, opts);
}

DialogResult AccountsWin::ProcessInput()
{
    bool readOnly = m_app.GetCore().IsReadOnly();
    DialogResult result = DialogResult::CANCEL;
    int c;
    while ((c = wgetch(m_win)) != ERR)
    {
        switch (c)
        {
        case KEY_CTRL('A'): {
            if (!readOnly)
            {
                AddNewEntry();
            }
            break;
        }
        case KEY_CTRL('C'): {
            if (!readOnly)
            {
                ChangeDbPasswordDlg dialog(m_app);
                if (dialog.Show(m_win) == DialogResult::OK)
                {
                    const std::string &database = m_app.GetCore().GetCurFile();
                    const std::string &password = dialog.GetPassword();
                    const std::string &newPassword = dialog.GetNewPassword();
                    if (ChangeDbPasswordCommand{m_app, database, password, newPassword}.Execute() != ResultCode::SUCCESS)
                    {
                        MessageBox(m_app).Show(m_win, "An error occurred changing the account database password.");
                    }
                }
            }
            break;
        }
        case KEY_CTRL('D'): {
            if (!readOnly)
            {
                ITEM *item = current_item(m_menu);
                DeleteEntry(item);
            }
            break;
        }
        case KEY_CTRL('S'): {
            if (!readOnly && Save())
            {
                result = DialogResult::OK;
                goto done;
            }
            break;
        }
        case KEY_CTRL('X'): {
            if (readOnly || DiscardChanges())
            {
                result = DialogResult::CANCEL;
                goto done;
            }
            break;
        }
        case KEY_CTRL('U'): {
            ITEM *item = current_item(m_menu);
            if (!IsGroupMenuItem(item))
            {
                const CItemData *cid = reinterpret_cast<CItemData *>(item_userptr(item));
                const std::string &str = cid->GetUser();
                if (CopyTextToClipboard(m_app, m_win, str) > 0)
                {
                    m_app.GetCommandBar().Show(this);
                }
            }
            break;
        }
        case KEY_CTRL('P'): {
            ITEM *item = current_item(m_menu);
            if (!IsGroupMenuItem(item))
            {
                const CItemData *cid = reinterpret_cast<CItemData *>(item_userptr(item));
                const std::string &str = cid->GetPassword();
                if (CopyTextToClipboard(m_app, m_win, str) > 0)
                {
                    m_app.GetCommandBar().Show(this);
                }
            }
            break;
        }
        case '/': {
            using std::placeholders::_1;
            m_app.DoSearch();
            m_app.GetCommandBar().Show(this);
            break;
        }
            // case 'a': {
            //     // Check for Group/Username/Title uniqueness
            //     // if (m_core.Find(m_Item.GetGroup(), m_Item.GetTitle(), m_Item.GetUser()) !=
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

        case '\n': {
            ITEM *item = current_item(m_menu);
            assert(!IsBlankMenuItem(item));
            if (IsGroupMenuItem(item))
            {
                // DisplayGroup(*cid);
            }
            else
            {
                CItemData *cid = reinterpret_cast<CItemData *>(item_userptr(item));
                assert(cid != nullptr);
                ShowAccountRecord(*cid);
            }

            break;
        }

        case KEY_HOME: {
            menu_driver(m_menu, REQ_FIRST_ITEM);
            break;
        }

        case KEY_END: {
            menu_driver(m_menu, REQ_LAST_ITEM);
            break;
        }

        case KEY_UP: {
            NavigateUp(m_menu, m_ncols, m_menuItems);
            break;
        }

        case KEY_DOWN: {
            NavigateDown(m_menu, m_ncols, m_menuItems);
            break;
        }

        case KEY_RIGHT: {
            NavigateRight(m_menu, m_ncols, m_menuItems);
            break;
        }

        case KEY_LEFT: {
            NavigateLeft(m_menu, m_ncols, m_menuItems);
            break;
        }

        case KEY_PPAGE: {
            NavigatePageUp(m_menu, m_ncols, m_menuItems);
            break;
        }

        case KEY_NPAGE: {
            NavigatePageDown(m_menu, m_ncols, m_menuItems);
            break;
        }

        default:
            break;
        }
    }

done:
    return result;
}
