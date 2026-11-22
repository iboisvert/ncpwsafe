/* Copyright 2020 Ian Boisvert */
#pragma once

#include "config.h"
#include "libncurses.h"
#include <functional>
#include <memory>
#include <vector>

#include "AccountDb.h"
#include "AccountsWin.h"
#include "CommandBarWin.h"
#include "ProgArgs.h"
#include "ResultCode.h"
#include "SearchBarWin.h"
#include "Dialog.h"
#include "Prefs.h"
#include "Theme.h"
#include "Filesystem.h"

class PWSafeApp
{
public:
    /** NCPWSAFE_APPNAME " " NCPWSAFE_VERSION */
    static const char *APPNAME_VERSION;

    PWSafeApp(Filesystem &filesystem = Filesystem::instance) : 
        prefs_(Prefs::Instance()),
        filesystem_(filesystem)
    { }

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
    ResultCode BackupDb();

    /** Search function */
    void DoSearch();

    const ProgArgs &GetArgs() const
    {
        return args_;
    }

    /** Get reference to account database */
    AccountDb &GetDb()
    {
        return db_;
    }

    /** Get reference to account database */
    const AccountDb &GetDb() const
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

    /** Text attributes for each kind of text, see `Theme::Attr()` */
    const Theme &GetTheme() const
    {
        return theme_;
    }

    /** Get the command bar window, used as the layout reference for the search bar */
    WINDOW *GetCommandBarWindow()
    {
        return commandbar_win_;
    }

    typedef std::function<void()> ResizeHandler;

    /**
     * Register a function to be called when the terminal is resized.
     * Handlers are called in the order they were registered, so that
     * windows are laid out from the bottom of the panel stack to the top.
     * @see ScopedResizeHandler
     */
    void PushResizeHandler(ResizeHandler handler)
    {
        resize_handlers_.push_back(std::move(handler));
    }

    /** Remove the most recently registered resize handler */
    void PopResizeHandler()
    {
        resize_handlers_.pop_back();
    }

    /**
     * Set the status shown at the right of the title bar, e.g. the selected account,
     * and redraw the title bar. The window isn't refreshed.
     */
    void SetStatus(const std::string &status);

    /** Redraw the title bar, e.g. after the database has changed. The window isn't refreshed */
    void DrawTitle();

    /**
     * Lay out all windows to fit the terminal after it has been resized.
     * Call when `wgetch()` returns `KEY_RESIZE`.
     */
    void HandleResize();

private:
    /** Minimum terminal size for which windows will be laid out */
    static constexpr int MIN_TERM_LINES = 6;
    static constexpr int MIN_TERM_COLS = 20;

    void InitTUI();
    void LayoutTUI();
    void DrawFrame();
    void EndTUI();
    void ProcessInput();

    ResultCode BackupDbImpl();
    void CleanBackups();

    Prefs &prefs_;
    Theme theme_;
    ProgArgs args_;
    AccountDb db_;
    Filesystem &filesystem_;
    std::unique_ptr<AccountsWin> accountswin_;
    std::unique_ptr<CommandBarWin> commandbarwin_;

    WINDOW *root_win_ = nullptr;        // stdscr
    WINDOW *accounts_win_ = nullptr;    // Accounts list
    WINDOW *commandbar_win_ = nullptr;  // Command bar
    PANEL *commandbar_panel_ = nullptr; // Command bar panel
    WINDOW *win_ = nullptr;            // Content of accounts list window
    std::string status_;               // Right side of the title bar
    int save_cursor_ = 0;
    std::vector<ResizeHandler> resize_handlers_;

#ifdef FRIEND_TEST
    FRIEND_TEST(AppTest, TestBackupDb_DstDoesntExist);
    FRIEND_TEST(AppTest, TestBackupDb_DstDoesExist);
#endif
};

/**
 * Registers a resize handler with the application for the lifetime of this object.
 */
class ScopedResizeHandler
{
public:
    ScopedResizeHandler(PWSafeApp &app, PWSafeApp::ResizeHandler handler) : app_(app)
    {
        app_.PushResizeHandler(std::move(handler));
    }

    ~ScopedResizeHandler()
    {
        app_.PopResizeHandler();
    }

    ScopedResizeHandler(const ScopedResizeHandler &) = delete;
    ScopedResizeHandler &operator=(const ScopedResizeHandler &) = delete;

private:
    PWSafeApp &app_;
};
