/* Copyright 2020 Ian Boisvert */
#pragma once

#include <functional>
#include <tuple>
#include "libncurses.h"
#include "PWSafeApp.h"
#include "Dialog.h"

class AccountsWin;

class SearchBarWin
{
public:
    SearchBarWin(PWSafeApp &app, WINDOW *win, AccountsWin &accounts_win)
        : app_(app), accounts_win_(accounts_win), win_(win), fields_{nullptr, nullptr}
    {
    }

    void Show();

private:
    void UpdateQueryString();
    /**
     * Find the next match from the current match position.
     * Does not update the current match.
     */
    AccountRecords::iterator FindNextImpl(AccountRecords::iterator &start_iter);
    bool FindNext();
    void ResetSavedMatch();
    void ResetLastMatch();
    void SetSelection(AccountRecords::const_iterator it);

    void InitTUI();
    void EndTUI();
    DialogResult ProcessInput();

    PWSafeApp &app_;
    AccountsWin &accounts_win_;
    std::string query_;
    AccountRecords::iterator save_match_;      ///< Item selected when search bar openend
    AccountRecords::iterator last_match_;      ///< Item selected after last "Find Next"
    AccountRecords::iterator transient_match_; ///< Item selected while typing query

    WINDOW *win_;
    PANEL *panel_ = nullptr;
    FORM *form_ = nullptr;
    WINDOW *form_win_ = nullptr;
    FIELD *fields_[2];
    int save_cursor_;
};
