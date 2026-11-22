/* Copyright 2026 Ian Boisvert */
#ifndef HAVE_ACCOUNTLIST_H
#define HAVE_ACCOUNTLIST_H

#include <set>
#include <string>
#include <vector>
#include "AccountRecords.h"

/**
 * The rows of the accounts list: a heading for each group, followed by the
 * group's accounts unless the group is collapsed, and the selection.
 *
 * Headings are only shown when at least one account has a group; the accounts
 * without a group then come first, under a heading of their own.
 * This class has no ncurses code, `AccountsWin` draws it.
 *
 * Rows point to the records they were built from: call `Build()` again
 * after the records change.
 */
class AccountList
{
public:
    enum class RowKind
    {
        HEADING,
        ACCOUNT,
    };

    struct Row
    {
        RowKind kind;
        /** The group of the heading or account, empty for no group */
        std::string group;
        /** The account, `nullptr` for a heading */
        const AccountRecord *record = nullptr;
        /** Heading: the number of accounts in the group */
        int count = 0;
        /** Heading: the group's accounts are hidden */
        bool collapsed = false;
    };

    /** A place in the list that stays valid while the records don't change, see `GetPosition()` */
    struct Position
    {
        /** The selected account, or `nullptr` if a heading is selected */
        const AccountRecord *record = nullptr;
        /** The group of the selected heading */
        std::string group;
        /** `false` if nothing was selected */
        bool valid = false;
    };

    /**
     * Build the rows from records sorted by group and title, and select the first account.
     * Collapsed groups stay collapsed.
     */
    void Build(const AccountRecords &records);

    const std::vector<Row> &Rows() const
    {
        return rows_;
    }

    /** `true` if group headings are shown */
    bool HasHeadings() const
    {
        return has_headings_;
    }

    /** The index of the selected row, -1 if the list is empty */
    int Selected() const
    {
        return selected_;
    }

    /** The selected row, `nullptr` if the list is empty */
    const Row *SelectedRow() const;

    /** The selected account, `nullptr` if a heading is selected or the list is empty */
    const AccountRecord *SelectedRecord() const;

    /** Select the row at `index`, clamped to the rows */
    void SelectIndex(int index);

    /** Select an account, expanding its group if it is collapsed. @returns `false` if it isn't in the list */
    bool SelectRecord(const AccountRecord &record);

    Position GetPosition() const;
    /** Select a position returned by `GetPosition()`; the records must not have changed since */
    void SetPosition(const Position &position);

    void MoveUp();
    void MoveDown();
    void MovePageUp(int page);
    void MovePageDown(int page);
    void MoveFirst();
    void MoveLast();
    /**
     * On an account, select its group heading.
     * On an expanded heading, collapse the group.
     */
    void Left();
    /**
     * On a collapsed heading, expand the group.
     * On an expanded heading, select the group's first account.
     */
    void Right();
    /** On a heading, collapse or expand the group. @returns `false` if the selection isn't a heading */
    bool Toggle();

    bool IsCollapsed(const std::string &group) const
    {
        return collapsed_.count(group) != 0;
    }

    /**
     * The first row to show in `height` rows, so that the selection is visible.
     * Scrolls only as much as needed from the last call.
     */
    int ScrollTop(int height);

private:
    struct Entry
    {
        std::string group;
        const AccountRecord *record;
    };

    /** Build the visible rows from the entries, keeping the selection at `position` */
    void Refresh(const Position &position);
    void SetCollapsed(const std::string &group, bool collapsed);

    std::vector<Entry> entries_;
    std::vector<Row> rows_;
    std::set<std::string> collapsed_;
    bool has_headings_ = false;
    int selected_ = -1;
    int top_ = 0;
};

#endif  // #ifndef HAVE_ACCOUNTLIST_H
