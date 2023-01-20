/* Copyright 2020 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Utils.h"
#include "core/StringX.h"
#include "libncurses.h"
#include <functional>
#include <tuple>

class AccountsWin;

class SearchBarWin
{
public:
    SearchBarWin(PWSafeApp &app, WINDOW *win, AccountsWin &AccountsWin)
        : m_app(app), m_AccountsWin(AccountsWin), m_win(win)
    {
    }

    void Show();

private:
    void UpdateQueryString();
    /**
     * Find the next match from the current match position.
     * Does not update the current match.
     */
    AccountsColl::iterator FindNextImpl(AccountsColl::iterator &startIter);
    bool FindNext();
    void ResetSavedMatch();
    void ResetLastMatch();
    void SetSelection(AccountsColl::iterator it);

    void InitTUI();
    void EndTUI();
    DialogResult ProcessInput();

    PWSafeApp &m_app;
    AccountsWin &m_AccountsWin;
    stringT m_query;
    AccountsColl::iterator m_saveMatch;      ///< Item selected when search bar openend
    AccountsColl::iterator m_lastMatch;      ///< Item selected after last "Find Next"
    AccountsColl::iterator m_transientMatch; ///< Item selected while typing query

    WINDOW *m_win;
    PANEL *m_panel = nullptr;
    FORM *m_form = nullptr;
    WINDOW *m_formWin = nullptr;
    int m_saveCursor;
};
