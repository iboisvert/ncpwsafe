/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_ACCOUNTSCOLL_H
#define HAVE_ACCOUNTSCOLL_H

#include <functional>
#include <tuple>
#include <vector>
#include "AccountDb.h"
#include "AccountRecord.h"

// The existing AccountRecord collection is a map
// where the key duplicates data in the element.
// This is not a good public interface, define a new collection

class AccountRecord;

/**
 * Wrapper around the AccountRecord collection owned by core
 */
struct AccountsColl
{
    // FIXME IMB 2023-01-21
    typedef std::vector<std::reference_wrapper<AccountRecord>> coll;
    typedef coll::iterator iterator;
    typedef coll::const_iterator const_iterator;

    AccountsColl(AccountDb &db) : m_db(db)
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

    AccountDb &m_db;
    coll m_accounts;
};

#endif