/* Copyright 2026 Ian Boisvert */
#include "AccountList.h"
#include <algorithm>

void AccountList::Build(const AccountRecords &records)
{
    entries_.clear();
    has_headings_ = false;
    for (const AccountRecord &record : records)
    {
        std::string group = record.GetField(FT_GROUP, "");
        if (!group.empty())
            has_headings_ = true;
        entries_.push_back({std::move(group), &record});
    }
    top_ = 0;
    Refresh(Position{});

    // Select the first account rather than a heading
    auto it = std::find_if(rows_.begin(), rows_.end(), [](const Row &row) { return row.kind == RowKind::ACCOUNT; });
    selected_ = rows_.empty() ? -1 : it != rows_.end() ? static_cast<int>(it - rows_.begin()) : 0;
}

void AccountList::Refresh(const Position &position)
{
    rows_.clear();
    for (size_t i = 0; i < entries_.size();)
    {
        const std::string &group = entries_[i].group;
        size_t end = i;
        while (end < entries_.size() && entries_[end].group == group)
            ++end;

        const bool collapsed = has_headings_ && IsCollapsed(group);
        if (has_headings_)
        {
            rows_.push_back({RowKind::HEADING, group, nullptr, static_cast<int>(end - i), collapsed});
        }
        if (!collapsed)
        {
            for (; i < end; ++i)
            {
                rows_.push_back({RowKind::ACCOUNT, group, entries_[i].record});
            }
        }
        i = end;
    }

    // Restore the selection
    int index = -1;
    if (position.valid)
    {
        for (size_t i = 0; i < rows_.size(); ++i)
        {
            const Row &row = rows_[i];
            if (position.record != nullptr ? row.record == position.record
                                           : row.kind == RowKind::HEADING && row.group == position.group)
            {
                index = static_cast<int>(i);
                break;
            }
        }
    }
    if (index < 0)
    {
        index = std::min(selected_, static_cast<int>(rows_.size()) - 1);
    }
    selected_ = rows_.empty() ? -1 : std::max(0, index);
}

const AccountList::Row *AccountList::SelectedRow() const
{
    return selected_ >= 0 ? &rows_[static_cast<size_t>(selected_)] : nullptr;
}

const AccountRecord *AccountList::SelectedRecord() const
{
    const Row *row = SelectedRow();
    return row != nullptr ? row->record : nullptr;
}

void AccountList::SelectIndex(int index)
{
    if (rows_.empty())
    {
        selected_ = -1;
        return;
    }
    selected_ = std::clamp(index, 0, static_cast<int>(rows_.size()) - 1);
}

bool AccountList::SelectRecord(const AccountRecord &record)
{
    auto entry = std::find_if(entries_.begin(), entries_.end(), [&record](const Entry &e) { return e.record == &record; });
    if (entry == entries_.end())
        return false;
    Position position{&record, entry->group, true};
    if (IsCollapsed(entry->group))
    {
        collapsed_.erase(entry->group);
    }
    Refresh(position);
    return true;
}

AccountList::Position AccountList::GetPosition() const
{
    const Row *row = SelectedRow();
    if (row == nullptr)
        return Position{};
    return Position{row->record, row->group, true};
}

void AccountList::SetPosition(const Position &position)
{
    if (!position.valid)
        return;
    if (position.record != nullptr)
    {
        SelectRecord(*position.record);
    }
    else
    {
        Refresh(position);
    }
}

void AccountList::MoveUp()
{
    if (selected_ > 0)
        --selected_;
}

void AccountList::MoveDown()
{
    if (selected_ >= 0 && selected_ + 1 < static_cast<int>(rows_.size()))
        ++selected_;
}

void AccountList::MovePageUp(int page)
{
    if (selected_ >= 0)
        SelectIndex(selected_ - std::max(1, page));
}

void AccountList::MovePageDown(int page)
{
    if (selected_ >= 0)
        SelectIndex(selected_ + std::max(1, page));
}

void AccountList::MoveFirst()
{
    SelectIndex(0);
}

void AccountList::MoveLast()
{
    SelectIndex(static_cast<int>(rows_.size()) - 1);
}

void AccountList::SetCollapsed(const std::string &group, bool collapsed)
{
    if (collapsed)
        collapsed_.insert(group);
    else
        collapsed_.erase(group);
    // Keep the heading selected
    Refresh(Position{nullptr, group, true});
}

void AccountList::Left()
{
    const Row *row = SelectedRow();
    if (row == nullptr || !has_headings_)
        return;
    if (row->kind == RowKind::ACCOUNT)
    {
        Refresh(Position{nullptr, row->group, true});
    }
    else if (!row->collapsed)
    {
        SetCollapsed(row->group, true);
    }
}

void AccountList::Right()
{
    const Row *row = SelectedRow();
    if (row == nullptr || row->kind != RowKind::HEADING)
        return;
    if (row->collapsed)
    {
        SetCollapsed(row->group, false);
    }
    else if (row->count > 0)
    {
        MoveDown();
    }
}

bool AccountList::Toggle()
{
    const Row *row = SelectedRow();
    if (row == nullptr || row->kind != RowKind::HEADING)
        return false;
    SetCollapsed(row->group, !row->collapsed);
    return true;
}

int AccountList::ScrollTop(int height)
{
    const int nrows = static_cast<int>(rows_.size());
    if (height <= 0 || nrows <= height)
    {
        top_ = 0;
        return top_;
    }
    if (selected_ >= 0)
    {
        if (selected_ < top_)
            top_ = selected_;
        else if (selected_ >= top_ + height)
            top_ = selected_ - height + 1;
    }
    top_ = std::clamp(top_, 0, nrows - height);
    return top_;
}
