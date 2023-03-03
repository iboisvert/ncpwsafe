/* Copyright 2020 Ian Boisvert */
#pragma once

#include "config.h"
#include "libncurses.h"
#include <memory>

#include "AccountDb.h"
#include "AccountsWin.h"
#include "CommandBarWin.h"
#include "ProgArgs.h"
#include "ResultCode.h"
#include "SearchBarWin.h"
#include "Dialog.h"
#include "Prefs.h"

class PWSafeApp
{
public:
    /** NCPWSAFE_APPNAME " " NCPWSAFE_VERSION */
    static const char *APPNAME_VERSION;

    PWSafeApp() : prefs_(Prefs::Instance()) { }

    // SUPPORT FUNCTIONS

    /** Init state and TUI */
    void Init(ProgArgs args);

    /** Open and edit an account database */
    DialogResult Show();

    /** Save database */
    int Save();

    /** Save preferences */
    void SavePrefs();

    /** Backup the current account database */
    int BackupDb();

    /** Search function */
    void DoSearch();

    const ProgArgs &GetArgs() const
    {
        return prog_args_;
    }

    AccountDb &GetDb()
    {
        return db_;
    }

    WINDOW *GetWindow()
    {
        return m_win;
    }

    CommandBarWin &GetCommandBar()
    {
        return *m_commandBar;
    }

private:
    void InitTUI();
    void EndTUI();
    void ProcessInput();

    Prefs &prefs_;
    ProgArgs prog_args_;
    AccountDb db_;
    std::unique_ptr<AccountsWin> m_accounts;
    std::unique_ptr<CommandBarWin> m_commandBar;

    WINDOW *m_rootWin = nullptr;        // stdscr
    WINDOW *m_accountsWin = nullptr;    // Accounts list
    WINDOW *m_commandBarWin = nullptr;  // Command bar
    PANEL *m_commandBarPanel = nullptr; // Command bar panel
    WINDOW *m_win = nullptr;            // Content of accounts list window
    int m_saveCursor = 0;
};
