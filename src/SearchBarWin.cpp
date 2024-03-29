/* Copyright 2022 Ian Boisvert */
#include "SearchBarWin.h"
#include "CommandBarWin.h"
#include "PWSafeApp.h"
#include "Utils.h"
#include <utility>

void SearchBarWin::InitTUI()
{
    panel_ = new_panel(win_);

    const std::vector<Action> actions{
        {"^L", "Next"},
        {"Enter", "Exit"},
        {"Esc", "Cancel"},
    };
    CommandBarWin::ShowActions(app_, win_, actions);

    waddstr(win_, "Search: ");

    int curx = getcurx(win_), cols = getmaxx(win_) - curx;
    fields_[0] = new_field(/*height*/ 1, cols, /*toprow*/ 0, /*leftcol*/ 0, /*offscreen*/ 0, /*nbuffers*/ 0);
    field_opts_off(fields_[0], O_STATIC);

    form_ = new_form(fields_);
    form_opts_off(form_, O_BS_OVERLOAD);
    set_form_win(form_, win_);
    form_win_ = derwin(win_, /*nlines*/ 1, /*ncols*/ cols, /*begin_y*/ 0, /*begin_x*/ curx);
    set_form_sub(form_, form_win_);
    post_form(form_);

    // Enable keypad so curses interprets function keys
    keypad(win_, TRUE);

    save_cursor_ = curs_set(1);
}

void SearchBarWin::EndTUI()
{
    del_panel(panel_);
    panel_ = nullptr;

    unpost_form(form_);
    free_form(form_);
    form_ = nullptr;

    free_field(fields_[0]);
    fields_[0] = nullptr;
    
    delwin(form_win_);
    form_win_ = nullptr;

    curs_set(save_cursor_);
}

void SearchBarWin::Show()
{
    query_.clear();

    const AccountRecord *psel = accounts_win_.GetSelection();
    auto &records = app_.GetDb().Records();
    save_match_ = records.begin();
    if (psel)
    {
        auto it = std::find_if(records.begin(), records.end(), [psel](const AccountRecord &rec) {
            return rec == *psel;
        });
        save_match_ = it;
    }
    last_match_ = save_match_;
    transient_match_ = save_match_;

    InitTUI();

    ProcessInput();

    EndTUI();
}

/** Search title, name, user, notes fields for query */
static bool Matches(const AccountRecord &account_rec, const std::string &query)
{
    // clang-format off
    return 
        account_rec.FieldContainsCaseInsensitive(FT_TITLE, query)
        || account_rec.FieldContainsCaseInsensitive(FT_NAME, query)
        || account_rec.FieldContainsCaseInsensitive(FT_USER, query)
        || account_rec.FieldContainsCaseInsensitive(FT_NOTES, query);
    // clang-format on
}

static AccountRecords::iterator FindNext(AccountRecords::iterator begin, AccountRecords::iterator end, const std::string &query)
{
    AccountRecords::iterator &it = begin;
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
AccountRecords::iterator SearchBarWin::FindNextImpl(AccountRecords::iterator &start_iter)
{
    auto &records = app_.GetDb().Records();
    AccountRecords::iterator begin = records.begin(), end = records.end();
    if (begin == end)
        return end;

    AccountRecords::iterator it = start_iter;
    if (it == end)
    {
        it = begin;
    }
    else
    {
        ++it;
    }

    it = ::FindNext(it, end, query_);
    if (it == end && start_iter != begin)
    {
        // Wrap search
        it = ::FindNext(begin, start_iter, query_);
    }
    return it;
}

bool SearchBarWin::FindNext()
{
    auto start_iter = last_match_;
    auto it = FindNextImpl(start_iter);
    auto end = app_.GetDb().Records().end();
    if (it != start_iter && it != end)
    {
        transient_match_ = it;
        accounts_win_.SetSelection(*it);
        return true;
    }
    return false;
}

void SearchBarWin::SetSelection(AccountRecords::const_iterator it)
{
    AccountRecords::iterator end = app_.GetDb().Records().end();
    if (it != end)
    {
        accounts_win_.SetSelection(*it);
    }
}

void SearchBarWin::ResetSavedMatch()
{
    SetSelection(save_match_);
}

void SearchBarWin::ResetLastMatch()
{
    SetSelection(last_match_);
}

void SearchBarWin::UpdateQueryString()
{
    // Required to synchronize window to buffer
    form_driver(form_, REQ_VALIDATION);
    FIELD *field = current_field(form_);
    char *cbuf = field_buffer(field, /*buffer*/ 0);
    query_ = rtrim(cbuf, cbuf + strlen(cbuf));
}

DialogResult SearchBarWin::ProcessInput()
{
    DialogResult rc = DialogResult::CANCEL;
    int ch = ERR;
    while ((ch = wgetch(win_)) != ERR)
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
            if (query_.size() > 0)
            {
                last_match_ = transient_match_;
                update = FindNext();
            }
            break;
        }
        case KEY_LEFT: {
            form_driver(form_, REQ_LEFT_CHAR);
            break;
        }
        case KEY_RIGHT: {
            form_driver(form_, REQ_RIGHT_CHAR);
            break;
        }
        case KEY_HOME: {
            form_driver(form_, REQ_BEG_LINE);
            break;
        }
        case KEY_END: {
            form_driver(form_, REQ_END_LINE);
            break;
        }
        case KEY_BACKSPACE: {
            if (form_driver(form_, REQ_DEL_PREV) == E_OK)
            {
                UpdateQueryString();
                if (query_.size() > 0)
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
            if (form_driver(form_, REQ_DEL_CHAR) == E_OK)
            {
                UpdateQueryString();
                update = FindNext();
            }
            break;
        }
        default: {
            if (form_driver(form_, ch) == E_OK)
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
