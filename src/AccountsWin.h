/* Copyright 2020 Ian Boisvert */
#pragma once

#include "libncurses.h"
#include "Dialog.h"
#include "AccountRecord.h"
#include <set>

class PWSafeApp;

class AccountsWin
{
public:
    AccountsWin(PWSafeApp &app, WINDOW *win);

    /** Show the password item list */
    DialogResult Show();

    void SetSelection(const AccountRecord &cid) const;
    const AccountRecord *GetSelection() const;

private:
    /* Command bar display mask */
    static constexpr int CBOPTS_READONLY = 1;
    /** Number of columns in which accounts will be displayed */
    static constexpr int NCOLS = 2;

    /** 
     * Get an AccountRecord ptr from the user pointer of a menu ITEM.
     * The AccountRecord must be const because `set` doesn't allow 
     * modifying keys
     */
    static AccountRecord *GetAccountRecordFromMenuItem(const ITEM *menu_item)
    {
        return reinterpret_cast<AccountRecord *>(item_userptr(menu_item));
    }

    static void AssignAccountRecord(ITEM *menu_item, AccountRecord &record)
    {
        set_item_userptr(menu_item, reinterpret_cast<void *>(&record));
    }

    /** Save changes to database */
    bool Save();
    /** Ask for confirmation to discard changes */
    bool DiscardChanges();
    /** View or edit an account entry */
    DialogResult ShowAccountRecord(AccountRecord &itemData);
    /** Replace a menu entry */
    void UpdateMenu(ITEM *menu_item, const AccountRecord &old_record, const AccountRecord &new_record);
    /** Add a new account entry */
    DialogResult AddNewEntry();
    /** Delete an account entry */
    DialogResult DeleteEntry(const ITEM *item);
    ITEM *FindItem(const AccountRecord &cid) const;

    void InitTUI();
    void EndTUI();
    /**
     * Construct a list of sorted MenuItemsData.
     * This list is used later to construct the menu ITEMs
     */
    void CreateMenuDataItems();
    void CreateMenu();
    void DestroyMenu();
    void SetCommandBar();
    DialogResult ProcessInput();

    PWSafeApp &app_;
    // std::string db_pathname_;

    WINDOW *win_ = nullptr;
    WINDOW *menu_win_ = nullptr;
    MENU *menu_ = nullptr;
    PANEL *panel_ = nullptr;
    std::vector<ITEM *> menu_items_;
    int save_cursor_ = 0;
};

/** Copy to clipboard, report errors */
extern int CopyTextToClipboard(PWSafeApp &app, WINDOW *win, const std::string &text);
