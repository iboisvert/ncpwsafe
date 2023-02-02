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
    m_progArgs = args;

    m_db.ReadOnly() = args.m_readOnly;

    // Get progname
    std::string progName = args.m_progName;
    int pos = progName.rfind(L'/');
    if (pos > -1)
    {
        progName = progName.substr(pos + 1);
    }

    // OK to load prefs now
    PWSprefs *prefs = PWSprefs::GetInstance();

    std::string filename = args.m_database;
    // if filename passed in command line, it takes precedence
    // over that in preference:
    if (filename.empty())
    {
        filename = prefs->GetPref(PWSprefs::CurrentFile).c_str();
    }
    if (!filename.empty())
    {
        m_db.DbPathName() = filename;
    }
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

    DialogResult rc = DialogResult::CONTINUE;
    while (rc != DialogResult::OK && rc != DialogResult::CANCEL)
    {
        rc = pwdprompt.Show(m_win);
        if (rc == DialogResult::OK)
        {
            wrefresh(m_win);

            std::string filename = pwdprompt.GetFilename();
            std::string password = pwdprompt.GetPassword();

            m_db.DbPathName() = filename;

            if (m_db.CheckPassword(filename, password))
            {
                const bool readOnly = m_db.ReadOnly();
                std::string locker("");
                const bool locked = m_db.LockFile(filename, locker);
                if (!readOnly && !locked)
                {
                    std::string msg("Could not lock file, opening read-only\nLocked by ");
                    msg.append(locker.c_str());
                    MessageBox(*this).Show(m_win, msg.c_str());
                    m_db.ReadOnly() = true;
                }

                if (m_db.ReadCurFile(password) == PWScore::SUCCESS)
                {
                    rc = m_accounts->Show();
                }
                else
                {
                    std::string msg("An error occurred reading database file ");
                    msg.append(filename.c_str());
                    MessageBox(*this).Show(m_win, msg.c_str());
                }

                if (locked)
                {
                    m_db.UnlockFile(filename);
                }
            }
        }
    }

    EndTUI();

    return rc;
}

/** 
 * Backup current account database 
 * Rewrite because exiting backup function doesn't
 * actually backup--it just renames current file
 * and hopes that an error does not occur on next save
 */
ResultCode PWSafeApp::BackupCurFile()
{
    PWSprefs *prefs = PWSprefs::GetInstance();
    if (prefs->GetPref(PWSprefs::BackupBeforeEverySave))
    {
        int maxNumIncBackups = prefs->GetPref(PWSprefs::BackupMaxIncremented);
        int backupSuffix = prefs->GetPref(PWSprefs::BackupSuffix);
        std::wstring userBackupPrefix = prefs->GetPref(PWSprefs::BackupPrefixValue).c_str();
        std::wstring userBackupDir = prefs->GetPref(PWSprefs::BackupDir).c_str();
        std::string bu_fname; // used to undo backup if save failed
        if (m_db.PreBackupCurFile(maxNumIncBackups, backupSuffix, 
            userBackupPrefix, userBackupDir, bu_fname))
        {
            const std::string &src = m_db.GetDbPathname();
            if (CopyFile(src.c_str(), bu_fname))
                return RC_SUCCESS;
        }
    }
    return RC_FAILURE;
}

/**
 * Save the database.
 * Backup the database first if enabled in preferences
 */
ResultCode PWSafeApp::Save()
{
    ResultCode rc = RC_SUCCESS;
    const std::string sxCurrFile = m_db.GetDbPathname();
    std::string bu_fname; // used to undo backup if save failed

    const PWSfile::VERSION current_version = m_db.GetReadFileVersion();
    switch (current_version)
    {
    case PWSfile::V30:
    case PWSfile::V40: {
        BackupCurFile();
        break;
    }

    // Do NOT code the default case statement - each version value must be specified
    // Prior versions are always Read-Only and so Save is not appropriate - although
    // they can export to prior versions (no point if not changed) or SaveAs in the
    // current version format
    case PWSfile::V17:
    case PWSfile::V20:
    case PWSfile::NEWFILE:
    case PWSfile::UNKNOWN_VERSION:
        assert(false);
        return RC_FAILURE;
    }

    // We are saving the current DB. Retain current version
    rc = StatusToRC(m_db.WriteFile(sxCurrFile, current_version));
    if (rc != RC_SUCCESS)
    { // Save failed!
        // Restore backup, if we have one
        if (!bu_fname.empty() && !sxCurrFile.empty())
            MoveFile(bu_fname.c_str(), sxCurrFile.c_str());
        return rc;
    }

    return rc;
}

/** Save state */
void PWSafeApp::SavePrefs()
{
    PWSprefs *prefs = PWSprefs::GetInstance();
    if (m_db.IsDbOpen())
    {
        prefs->SetPref(PWSprefs::CurrentFile, m_db.GetDbPathname());
    }
    prefs->SaveApplicationPreferences();
    prefs->SaveShortcuts();
}

/** Release memory held by app, including core */
void PWSafeApp::Destroy()
{
    PWSprefs::DeleteInstance();
    // IMB 2022-12-13 If I remove the next line
    //   I get an unresolved external on link, no idea why
    PWSLog::DeleteLog();
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
