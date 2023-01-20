/* Copyright 2022 Ian Boisvert */
#pragma once

#include <functional>
#include <tuple>
#include <vector>

// The existing CITemData collection is a map
// where the key duplicates data in the element.
// This is not a good public interface, define a new collection

class CItemData;

/**
 * Wrapper around the CItemData collection owned by core
 */
struct AccountsColl
{
    typedef std::vector<std::reference_wrapper<CItemData>> coll;
    typedef coll::iterator iterator;
    typedef coll::const_iterator const_iterator;

    AccountsColl(PWScore &core) : m_core(core)
    {
    }
    // Reload collection from core, invalidates all iterators
    void Refresh();
    iterator begin()
    {
        return m_accounts.begin();
    }
    const_iterator begin() const
    {
        return m_accounts.begin();
    }
    iterator end()
    {
        return m_accounts.end();
    }
    const_iterator end() const
    {
        return m_accounts.end();
    }

private:
    void Sort();

    PWScore &m_core;
    coll m_accounts;
};