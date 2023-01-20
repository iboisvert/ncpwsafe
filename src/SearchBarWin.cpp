/* Copyright 2022 Ian Boisvert */
#include "SearchBarWin.h"
#include "CommandBarWin.h"
#include "PWSafeApp.h"
#include "Utils.h"
#include "core/std::string.h"
#include <utility>

void SearchBarWin::InitTUI()
{
    m_panel = new_panel(m_win);

    const std::vector<Action> actions{
        {L"^L", L"Next"},
        {L"Enter", L"Exit"},
        {L"Esc", L"Cancel"},
    };
    CommandBarWin::ShowActions(m_win, actions);

    waddwstr(m_win, L"Search: ");

    int curx = getcurx(m_win), cols = getmaxx(m_win) - curx;
    FIELD *fields[]{new_field(/*height*/ 1, cols, /*toprow*/ 0, /*leftcol*/ 0, /*offscreen*/ 0, /*nbuffers*/ 0), NULL};
    field_opts_off(fields[0], O_STATIC);

    m_form = new_form(fields);
    form_opts_off(m_form, O_BS_OVERLOAD);
    set_form_win(m_form, m_win);
    m_formWin = derwin(m_win, /*nlines*/ 1, /*ncols*/ cols, /*begin_y*/ 0, /*begin_x*/ curx);
    set_form_sub(m_form, m_formWin);
    post_form(m_form);

    // Enable keypad so curses interprets function keys
    keypad(m_win, TRUE);

    m_saveCursor = curs_set(1);
}

void SearchBarWin::EndTUI()
{
    del_panel(m_panel);
    m_panel = nullptr;

    unpost_form(m_form);
    FIELD **fields = form_fields(m_form);
    int nfields = field_count(m_form);
    for (int i = 0; i < nfields; ++i)
    {
        free_field(fields[i]);
    }
    free_form(m_form);
    m_form = nullptr;
    delwin(m_formWin);
    m_formWin = nullptr;

    curs_set(m_saveCursor);
}

void SearchBarWin::Show()
{
    m_query.clear();

    const CItemData *pcid = m_AccountsWin.GetSelection();
    auto &accounts = m_app.GetAccountsCollection();
    m_saveMatch = accounts.begin();
    if (pcid)
    {
        auto it = std::find_if(accounts.begin(), accounts.end(), [pcid](const CItemData &cid) {
            return cid == *pcid;
        });
        m_saveMatch = it;
    }
    m_lastMatch = m_saveMatch;
    m_transientMatch = m_saveMatch;

    InitTUI();

    ProcessInput();
}

/** Search title, name, user, notes fields for query */
static bool Matches(const CItemData &cid, const std::string &query)
{
    // clang-format off
    return 
        cid.Matches(query, FT_TITLE, PWSMatch::MatchRule::MR_CONTAINS)
        || cid.Matches(query, PWS_FIELD_TYPE::NAME, PWSMatch::MatchRule::MR_CONTAINS)
        || cid.Matches(query, PWS_FIELD_TYPE::USER, PWSMatch::MatchRule::MR_CONTAINS)
        || cid.Matches(query, PWS_FIELD_TYPE::NOTES, PWSMatch::MatchRule::MR_CONTAINS);
    // clang-format on
}

static AccountsColl::iterator FindNext(AccountsColl::iterator begin, AccountsColl::iterator end, const std::string &query)
{
    AccountsColl::iterator &it = begin;
    for (; it != end; ++it)
    {
        if (Matches(*it, query))
            break;
    }
    return it;
}

/**
 * Find the next match from the current match position.
 * Does not update the current match.
 */
AccountsColl::iterator SearchBarWin::FindNextImpl(AccountsColl::iterator &startIter)
{
    auto &accounts = m_app.GetAccountsCollection();
    AccountsColl::iterator begin = accounts.begin(), end = accounts.end();
    if (begin == end)
        return end;

    AccountsColl::iterator it = startIter;
    if (it == end)
    {
        it = begin;
    }
    else
    {
        ++it;
    }

    it = ::FindNext(it, end, m_query);
    if (it == end && startIter != begin)
    {
        // Wrap search
        it = ::FindNext(begin, startIter, m_query);
    }
    return it;
}

bool SearchBarWin::FindNext()
{
    AccountsColl::iterator startIter = m_lastMatch;
    AccountsColl::iterator it = FindNextImpl(startIter), end = m_app.GetAccountsCollection().end();
    if (it != startIter && it != end)
    {
        m_transientMatch = it;
        m_AccountsWin.SetSelection(*it);
        return true;
    }
    return false;
}

void SearchBarWin::SetSelection(AccountsColl::iterator it)
{
    AccountsColl::iterator end = m_app.GetAccountsCollection().end();
    if (it != end)
    {
        m_AccountsWin.SetSelection(*it);
    }
}

void SearchBarWin::ResetSavedMatch()
{
    SetSelection(m_saveMatch);
}

void SearchBarWin::ResetLastMatch()
{
    SetSelection(m_lastMatch);
}

void SearchBarWin::UpdateQueryString()
{
    // Required to synchronize window to buffer
    form_driver(m_form, REQ_VALIDATION);
    FIELD *field = current_field(m_form);
    char *cbuf = field_buffer(field, /*buffer*/ 0);
    m_query = Utf8ToWideString(rtrim(cbuf, cbuf + strlen(cbuf)));
}

DialogResult SearchBarWin::ProcessInput()
{
    DialogResult rc = DialogResult::CANCEL;
    int ch = ERR;
    while ((ch = wgetch(m_win)) != ERR)
    {
        bool update = false;

        switch (ch)
        {
        case '\n': {
            rc = DialogResult::OK;
            goto done;
        }
        case KEY_CTRL('X'):
        case KEY_ESC: {
            ResetSavedMatch();
            goto done;
        }
        case KEY_CTRL('L'): {
            // Next
            if (m_query.size() > 0)
            {
                m_lastMatch = m_transientMatch;
                update = FindNext();
            }
            break;
        }
        case KEY_LEFT: {
            form_driver(m_form, REQ_LEFT_CHAR);
            break;
        }
        case KEY_RIGHT: {
            form_driver(m_form, REQ_RIGHT_CHAR);
            break;
        }
        case KEY_HOME: {
            form_driver(m_form, REQ_BEG_LINE);
            break;
        }
        case KEY_END: {
            form_driver(m_form, REQ_END_LINE);
            break;
        }
        case KEY_BACKSPACE: {
            if (form_driver(m_form, REQ_DEL_PREV) == E_OK)
            {
                UpdateQueryString();
                if (m_query.size() > 0)
                {
                    update = FindNext();
                }
                else
                {
                    ResetLastMatch();
                    update = true;
                }
            }
            break;
        }
        case KEY_DC: {
            if (form_driver(m_form, REQ_DEL_CHAR) == E_OK)
            {
                UpdateQueryString();
                update = FindNext();
            }
            break;
        }
        default: {
            if (form_driver(m_form, ch) == E_OK)
            {
                UpdateQueryString();
                update = FindNext();
            }
            break;
        }
        }

        if (update)
        {
            update_panels();
            doupdate();
        }
    }

done:
    return rc;
}
