/* Copyright 2020 Ian Boisvert */
#pragma once

#include "core/PWScore.h"
#include "libncurses.h"
#include <memory>

#include "AccountsColl.h"
#include "AccountsWin.h"
#include "CommandBarWin.h"
#include "ProgArgs.h"
#include "RecentDbList.h"
#include "ResultCode.h"
#include "SearchBarWin.h"
#include "Utils.h"

class PWSafeApp
{
public:
    PWSafeApp() : m_core{}, m_accountsColl{m_core}
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

    PWScore &GetCore()
    {
        return m_core;
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
    RecentDbList &RecentDatabases();

    void InitTUI();
    void EndTUI();
    void ProcessInput();

    ProgArgs m_progArgs;
    PWScore m_core;
    AccountsColl m_accountsColl;
    std::unique_ptr<AccountsWin> m_accounts;
    std::unique_ptr<RecentDbList> m_recentDatabases;
    std::unique_ptr<CommandBarWin> m_commandBar;

    WINDOW *m_rootWin = nullptr;        // stdscr
    WINDOW *m_accountsWin = nullptr;    // Accounts list
    WINDOW *m_commandBarWin = nullptr;  // Command bar
    PANEL *m_commandBarPanel = nullptr; // Command bar panel
    WINDOW *m_win = nullptr;            // Content of accounts list window
    int m_saveCursor = 0;
};
