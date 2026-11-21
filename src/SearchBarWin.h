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
    /**
     * @param layout_win The search bar is displayed in a window with the
     * same size and position as `layout_win`
     */
    SearchBarWin(PWSafeApp &app, WINDOW *layout_win, AccountsWin &accounts_win)
        : app_(app), accounts_win_(accounts_win), layout_win_(layout_win), fields_{nullptr, nullptr}
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
    /** Create the search bar window and form */
    void CreateWindows();
    /** Destroy the search bar form and its subwindow */
    void DestroyForm();
    /** Lay out the search bar after the terminal has been resized */
    void OnResize();
    void EndTUI();
    DialogResult ProcessInput();

    PWSafeApp &app_;
    AccountsWin &accounts_win_;
    std::string query_;
    AccountRecords::iterator save_match_;      ///< Item selected when search bar openend
    AccountRecords::iterator last_match_;      ///< Item selected after last "Find Next"
    AccountRecords::iterator transient_match_; ///< Item selected while typing query

    WINDOW *layout_win_;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    FORM *form_ = nullptr;
    WINDOW *form_win_ = nullptr;
    FIELD *fields_[2];
    int save_cursor_;
};
