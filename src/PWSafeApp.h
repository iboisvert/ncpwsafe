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
    int BackupDb()
    {
        throw std::exception();
    }

    /** Search function */
    void DoSearch();

    const ProgArgs &GetArgs() const
    {
        return args_;
    }

    AccountDb &GetDb()
    {
        return db_;
    }

    WINDOW *GetWindow()
    {
        return win_;
    }

    CommandBarWin &GetCommandBar()
    {
        return *commandbarwin_;
    }

private:
    void InitTUI();
    void EndTUI();
    void ProcessInput();

    Prefs &prefs_;
    ProgArgs args_;
    AccountDb db_;
    std::unique_ptr<AccountsWin> accountswin_;
    std::unique_ptr<CommandBarWin> commandbarwin_;

    WINDOW *root_win_ = nullptr;        // stdscr
    WINDOW *accounts_win_ = nullptr;    // Accounts list
    WINDOW *commandbar_win_ = nullptr;  // Command bar
    PANEL *commandbar_panel_ = nullptr; // Command bar panel
    WINDOW *win_ = nullptr;            // Content of accounts list window
    int save_cursor_ = 0;
};
