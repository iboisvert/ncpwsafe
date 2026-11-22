/* Copyright 2020 Ian Boisvert */
#pragma once

#include "libncurses.h"
#include "Dialog.h"
#include "AccountList.h"
#include "AccountRecord.h"
#include <string>

class PWSafeApp;

/**
 * The main window: the list of accounts, grouped under a heading for each group.
 * Headings can be collapsed and expanded. The rows and the selection are kept
 * by `AccountList`, this class draws them and handles input.
 */
class AccountsWin
{
public:
    AccountsWin(PWSafeApp &app, WINDOW *win);

    /** Show the password item list */
    DialogResult Show();

    /** Select an account, expanding its group if it is collapsed */
    void SetSelection(const AccountRecord &record);
    /** The selected account, `nullptr` if a group heading is selected */
    const AccountRecord *GetSelection() const;
    /** The selected row, an account or a group heading */
    AccountList::Position GetPosition() const
    {
        return list_.GetPosition();
    }
    /** Select a position returned by `GetPosition()`, the accounts must not have changed since */
    void SetPosition(const AccountList::Position &position);

private:
    /* Command bar display mask */
    static constexpr int CBOPTS_READONLY = 1;
    /** The list starts below a blank row */
    static constexpr int FIRST_ROW = 1;

    /**
     * Save changes to the database. If saving fails, ask to retry.
     * @returns `DialogResult::OK` if the changes were saved, `DialogResult::NO` if
     * saving failed and the user chose to quit without saving, `DialogResult::CANCEL`
     * if saving failed and the user chose to go back to the accounts list
     */
    DialogResult Save();
    /** Ask for confirmation to discard changes */
    bool DiscardChanges();
    /**
     * If there are unsaved changes, warn that changing the database password
     * writes them and ask to continue. @returns `true` to continue
     */
    bool ConfirmWriteChanges();
    /** View or edit an account entry */
    DialogResult ShowAccountRecord(AccountRecord &itemData);
    /** Save an edited account and update the list */
    void UpdateList(AccountRecord old_record, const AccountRecord &new_record);
    /** Add a new account entry */
    DialogResult AddNewEntry();
    /** Delete an account entry */
    DialogResult DeleteEntry(const AccountRecord &record);
    /** Select the account with the given UUID */
    void SelectUuid(const std::string &uuid);

    void InitTUI();
    void EndTUI();
    /** Rebuild the list rows from the account records */
    void Rebuild();
    /** Number of rows available for the list */
    int ListHeight() const;
    /** Width of the title column, from the longest title */
    int TitleColumnWidth(int width) const;
    /** Draw the visible rows, and the selection in the title bar */
    void Draw();
    /** The selection, for the title bar: "Banking › Chequing  3/7" */
    std::string StatusText() const;
    void DrawHeading(int y, const AccountList::Row &row, bool selected);
    void DrawAccount(int y, const AccountList::Row &row, bool selected, int title_width);
    /** Lay out the list after the terminal has been resized */
    void OnResize();
    void SetCommandBar();
    DialogResult ProcessInput();

    PWSafeApp &app_;

    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    AccountList list_;
    int save_cursor_ = 0;
};

/** Copy to clipboard, report errors */
extern int CopyTextToClipboard(PWSafeApp &app, WINDOW *win, const std::string &text);
