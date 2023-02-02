/* Copyright 2020 Ian Boisvert */
#pragma once

#include "config.h"
#include "libncurses.h"
#include <memory>

#include "AccountDb.h"
#include "AccountsColl.h"
#include "AccountsWin.h"
#include "CommandBarWin.h"
#include "ProgArgs.h"
#include "ResultCode.h"
#include "SearchBarWin.h"
#include "Utils.h"

class PWSafeApp
{
public:
    /** NCPWSAFE_APPNAME " " NCPWSAFE_VERSION */
    static const char *APPNAME_VERSION;

    PWSafeApp() : m_db{}, m_accountsColl{m_db}
    {
    }
    ~PWSafeApp()
    {
        Destroy();
    }

    // SUPPORT FUNCTIONS

    /** Init state and TUI */
    void Init(ProgArgs args);

    /** Open and edit an account database */
    DialogResult Show();

    /** Save database */
    ResultCode Save();
    void SavePrefs();

    /** Backup current account database */
    ResultCode BackupCurFile();

    /** Release memory held by app, including core */
    void Destroy();

    /** Search function */
    void DoSearch();

    const ProgArgs &GetArgs() const
    {
        return m_progArgs;
    }

    AccountsColl &GetAccountsCollection()
    {
        return m_accountsColl;
    }

    AccountDb &GetDb()
    {
        return m_db;
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

    ProgArgs m_progArgs;
    AccountDb m_db;
    AccountsColl m_accountsColl;
    std::unique_ptr<AccountsWin> m_accounts;
    std::unique_ptr<CommandBarWin> m_commandBar;

    WINDOW *m_rootWin = nullptr;        // stdscr
    WINDOW *m_accountsWin = nullptr;    // Accounts list
    WINDOW *m_commandBarWin = nullptr;  // Command bar
    PANEL *m_commandBarPanel = nullptr; // Command bar panel
    WINDOW *m_win = nullptr;            // Content of accounts list window
    int m_saveCursor = 0;
};
