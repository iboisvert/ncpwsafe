/* Copyright 2022 Ian Boisvert */
#include "AccountsColl.h"
#include <cstring>

// Reload collection from core, invalidates all iterators
void AccountsColl::Refresh()
{
    m_accounts.clear();
    for (auto it = m_db.GetEntryIter(); it != m_db.GetEntryEndIter(); ++it)
    {
        m_accounts.push_back(it->second);
    }
    Sort();
}

void AccountsColl::Sort()
{
    std::sort(m_accounts.begin(), m_accounts.end(), [](const AccountRecord &a, const AccountRecord &b) {
        // Compare group first, then title
        int result;
        return (result = strcmp(a.GetGroup(), b.GetGroup())) < 0 ||
               (result == 0 && (strcmp(a.GetTitle(), b.GetTitle())) < 0);
    });
}
