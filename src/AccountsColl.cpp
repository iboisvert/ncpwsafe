/* Copyright 2022 Ian Boisvert */
#include "AccountsColl.h"
#include "core/ItemData.h"

// Reload collection from core, invalidates all iterators
void AccountsColl::Refresh()
{
    m_accounts.clear();
    for (auto it = m_core.GetEntryIter(); it != m_core.GetEntryEndIter(); ++it)
    {
        m_accounts.push_back(it->second);
    }
    Sort();
}

void AccountsColl::Sort()
{
    std::sort(m_accounts.begin(), m_accounts.end(), [](const CItemData &a, const CItemData &b) {
        // Compare group first, then title
        int result;
        return (result = a.GetGroup().compare(b.GetGroup())) < 0 ||
               (result == 0 && (result = a.GetTitle().compare(b.GetTitle())) < 0);
    });
}
