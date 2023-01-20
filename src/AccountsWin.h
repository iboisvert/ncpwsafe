/* Copyright 2020 Ian Boisvert */
#pragma once

#include "core/PWScore.h"
#include "libncurses.h"
#include "Utils.h"
#include <set>

class PWSafeApp;

class AccountsWin
{
public:
    AccountsWin(PWSafeApp &app, WINDOW *win);

    /** Show the password item list */
    DialogResult Show();

    void SetSelection(const CItemData &cid) const;
    const CItemData *GetSelection() const;

private:
    /* Command bar display mask */
    static constexpr int CBOPTS_READONLY = 1;
    /** Number of columns in which accounts will be displayed */
    static constexpr int m_ncols = 2;

    /** Save changes to database */
    bool Save();
    /** Ask for confirmation to discard changes */
    bool DiscardChanges();
    /** View or edit an account entry */
    DialogResult ShowAccountRecord(CItemData &itemData);
    /** Add a new account entry */
    DialogResult AddNewEntry();
    /** Delete an account entry */
    DialogResult DeleteEntry(const ITEM *item);
    ITEM *FindItem(const CItemData &cid) const;

    void InitTUI();
    void EndTUI();
    /**
     * Construct a list of sorted MenuItemsData.
     * This list is used later to construct the menu ITEMs
     */
    void CreateMenuDataItems();
    void CreateMenu();
    void SetCommandBar();
    DialogResult ProcessInput();

    PWSafeApp &m_app;
    StringX m_database;

    WINDOW *m_win = nullptr;
    WINDOW *m_menuWin = nullptr;
    MENU *m_menu = nullptr;
    PANEL *m_panel = nullptr;
    std::vector<ITEM *> m_menuItems;
    int m_saveCursor = 0;
};

/** Copy to clipboard, report errors */
extern int CopyTextToClipboard(PWSafeApp &app, WINDOW *win, const stringT &text);
