/* Copyright 2020 Ian Boisvert */
#include "AccountsWin.h"
#include "Dialog.h"
#include "Label.h"
#include "MessageBox.h"
#include "PWSafeApp.h"
#include "SafeCombinationPromptDlg.h"
#include "Utils.h"
#include "ResultCode.h"
#include "FileUtils.h"

const char *PWSafeApp::APPNAME_VERSION = NCPWSAFE_APPNAME " " NCPWSAFE_VERSION;

void PWSafeApp::Init(ProgArgs args)
{
    args_ = args;

    prefs_.ReadPrefs(args_.config_file_);

    db_.ReadOnly() = args.m_readOnly;

    // Get progname
    std::string prog_name = args.m_progName;
    int pos = prog_name.rfind(L'/');
    if (pos > -1)
    {
        prog_name = prog_name.substr(pos + 1);
    }

    std::string &pathname = args.m_database;
    // if pathname passed in command line, it takes precedence
    // over that in preference:
    if (pathname.empty())
    {
        pathname = prefs_.PrefAsString(Prefs::DB_PATHNAME);
    }
    if (!pathname.empty())
    {
        db_.DbPathname() = pathname;
    }

    db_.Password() = args.m_password;
}

/** Open and edit an account database */
DialogResult PWSafeApp::Show()
{
    InitTUI();

    // Command bar must be constructed first
    commandbarwin_ = std::make_unique<CommandBarWin>(*this, commandbar_win_);
    accountswin_ = std::make_unique<AccountsWin>(*this, win_);

    wrefresh(root_win_);

    // prompt for password, try to Load.
    SafeCombinationPromptDlg pwdprompt(*this);

    DialogResult dr = DialogResult::CONTINUE;
    while (dr != DialogResult::OK && dr != DialogResult::CANCEL)
    {
        dr = pwdprompt.Show(win_);
        if (dr == DialogResult::OK)
        {
            wrefresh(win_);

            std::string db_pathname = pwdprompt.GetFilename();
            std::string password = pwdprompt.GetPassword();

            db_.DbPathname() = db_pathname;
            db_.Password() = password;

            if (db_.CheckPassword())
            {
                if (db_.ReadDb())
                {
                    dr = accountswin_->Show();
                }
                else
                {
                    std::string msg("An error occurred reading database file ");
                    msg.append(db_pathname.c_str());
                    MessageBox(*this).Show(win_, msg.c_str());
                }
            }
        }
    }

    EndTUI();

    return dr;
}

// int PWSafeApp::BackupDb()
// {

// }

/**
 * Save the database.
 * Backup the database first if enabled in preferences
 */
int PWSafeApp::Save()
{
    int rc = RC_SUCCESS;
    if (db_.IsDirty())
    {
        if (prefs_.PrefAsBool(Prefs::BACKUP_BEFORE_SAVE))
        {
            BackupDb();
        }
        db_.WriteDb(&rc);
    }
    return rc;
}

/** Save state */
void PWSafeApp::SavePrefs()
{
    prefs_.WritePrefs(args_.config_file_);
}

void PWSafeApp::InitTUI()
{
    /* Initialize curses */
    root_win_ = initscr();
    // start_color();
    raw();
    noecho();
    // IMB 2022-12-13 htop uses F3/S-F3 to find next/prev, like in
    //   windows. But S-F3 doesn't work with the "screen" terminal,
    //   which seems to be the default terminal set by tmux,
    //   one needs to use something else like "screen.vte" so that
    //   the correct ansi code is generated.
    //   Instead of requiring a non-default setting we will use
    //   different keys--IDEA uses ^L for example.
    // define_key("\033[14;2~", KEY_F(15));  // Shift-F3

    save_cursor_ = curs_set(/*visibility*/ 1);

    // IMB 2022-12-24 ncurses behaves strangely--
    // max_y in this case is the line below the bottom line on the screen
    // This behaviour is probably documented somewhere..
    int max_y, max_x, beg_y, beg_x;
    getmaxyx(root_win_, max_y, max_x);
    getbegyx(root_win_, beg_y, beg_x);
    int nlines = max_y - beg_y, ncols = max_x - beg_x;

    accounts_win_ = derwin(root_win_, nlines - 1, ncols, /*begin_y*/ 0, /*begin_x*/ 0);
    box(accounts_win_, /*verch*/ 0, /*horch*/ 0);
    Label::WriteJustified(accounts_win_, beg_y, beg_x, max_x, APPNAME_VERSION, JUSTIFY_CENTER);

    win_ = newwin(nlines - 3, ncols - 2, /*begin_y*/ beg_y + 1, /*begin_x*/ beg_x + 1);

    commandbar_win_ = newwin(/*nlines*/ 1, ncols, /*begin_y*/ nlines - 1, /*begin_x*/ beg_x);
    commandbar_panel_ = new_panel(commandbar_win_);
    // Draw command bar
    wattron(commandbar_win_, A_REVERSE | A_DIM);
    mvwhline(commandbar_win_, /*y*/ 0, beg_x, /*ch*/ ' ', ncols);
    wattroff(commandbar_win_, A_REVERSE | A_DIM);
}

void PWSafeApp::EndTUI()
{
    delwin(win_);
    win_ = nullptr;
    del_panel(commandbar_panel_);
    commandbar_panel_ = nullptr;
    delwin(commandbar_win_);
    commandbarwin_ = nullptr;
    delwin(accounts_win_);
    accounts_win_ = nullptr;
    endwin();

    curs_set(save_cursor_);
    echo();
}

void PWSafeApp::DoSearch()
{
    update_panels();
    hide_panel(commandbar_panel_);

    WINDOW *win = dupwin(commandbar_win_);

    SearchBarWin{*this, win, *accountswin_}.Show();

    delwin(win);

    show_panel(commandbar_panel_);
    update_panels();
    doupdate();
}
