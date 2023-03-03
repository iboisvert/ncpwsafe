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
    prog_args_ = args;

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
        pathname = prefs_.Pref(Prefs::DB_PATHNAME);
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
    m_commandBar = std::make_unique<CommandBarWin>(*this, m_commandBarWin);
    m_accounts = std::make_unique<AccountsWin>(*this, m_win);

    wrefresh(m_rootWin);

    // prompt for password, try to Load.
    SafeCombinationPromptDlg pwdprompt(*this);

    DialogResult dr = DialogResult::CONTINUE;
    while (dr != DialogResult::OK && dr != DialogResult::CANCEL)
    {
        dr = pwdprompt.Show(m_win);
        if (dr == DialogResult::OK)
        {
            wrefresh(m_win);

            std::string db_pathname = pwdprompt.GetFilename();
            std::string password = pwdprompt.GetPassword();

            db_.DbPathname() = db_pathname;
            db_.Password() = password;

            if (db_.CheckPassword())
            {
                if (db_.ReadDb())
                {
                    dr = m_accounts->Show();
                }
                else
                {
                    std::string msg("An error occurred reading database file ");
                    msg.append(db_pathname.c_str());
                    MessageBox(*this).Show(m_win, msg.c_str());
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
    prefs_.WritePrefs();
}

void PWSafeApp::InitTUI()
{
    /* Initialize curses */
    m_rootWin = initscr();
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

    m_saveCursor = curs_set(/*visibility*/ 1);

    // IMB 2022-12-24 ncurses behaves strangely--
    // max_y in this case is the line below the bottom line on the screen
    // This behaviour is probably documented somewhere..
    int max_y, max_x, beg_y, beg_x;
    getmaxyx(m_rootWin, max_y, max_x);
    getbegyx(m_rootWin, beg_y, beg_x);
    int nlines = max_y - beg_y, ncols = max_x - beg_x;

    m_accountsWin = derwin(m_rootWin, nlines - 1, ncols, /*begin_y*/ 0, /*begin_x*/ 0);
    box(m_accountsWin, /*verch*/ 0, /*horch*/ 0);
    Label::WriteJustified(m_accountsWin, beg_y, beg_x, max_x, APPNAME_VERSION, JUSTIFY_CENTER);

    m_win = newwin(nlines - 3, ncols - 2, /*begin_y*/ beg_y + 1, /*begin_x*/ beg_x + 1);

    m_commandBarWin = newwin(/*nlines*/ 1, ncols, /*begin_y*/ nlines - 1, /*begin_x*/ beg_x);
    m_commandBarPanel = new_panel(m_commandBarWin);
    // Draw command bar
    wattron(m_commandBarWin, A_REVERSE | A_DIM);
    mvwhline(m_commandBarWin, /*y*/ 0, beg_x, /*ch*/ ' ', ncols);
    wattroff(m_commandBarWin, A_REVERSE | A_DIM);
}

void PWSafeApp::EndTUI()
{
    delwin(m_win);
    m_win = nullptr;
    del_panel(m_commandBarPanel);
    m_commandBarPanel = nullptr;
    delwin(m_commandBarWin);
    m_commandBar = nullptr;
    delwin(m_accountsWin);
    m_accountsWin = nullptr;
    endwin();

    curs_set(m_saveCursor);
    echo();
}

void PWSafeApp::DoSearch()
{
    update_panels();
    hide_panel(m_commandBarPanel);

    WINDOW *win = dupwin(m_commandBarWin);

    SearchBarWin{*this, win, *m_accounts}.Show();

    delwin(win);

    show_panel(m_commandBarPanel);
    update_panels();
    doupdate();
}
