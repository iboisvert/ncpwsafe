/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTDB_H
#define HAVE_ACCOUNTDB_H

#include <memory>
#include <vector>
#include <string>
#include <cassert>
#include "libpwsafe.h"
#include "AccountRecords.h"
#include "ResultCode.h"


struct AccountDb
{
    /** Pathname of database file */
    std::string &DbPathname()
    {
        return db_pathname_;
    }
    /** Pathname of database file */
    const std::string &DbPathname() const
    {
        return db_pathname_;
    }

    /** Database password */
    std::string &Password()
    {
        return password_;
    }

    /** Database read-only flag, default `false` */
    bool &ReadOnly()
    {
        return readonly_;
    }
    /** Database read-only flag, default `false` */
    const bool &ReadOnly() const
    {
        return readonly_;
    }

    /** Check if file at DbPathname() exists. */
    bool Exists() const
    {
        throw std::exception();
    }

    // IMB 2023-02-02 Want to be able to track if database records have changed
    bool IsDirty() const
    {
        return records_.IsDirty();
    }
    
    /** Database account records collection. */
    AccountRecords &Records()
    {
        return records_;
    }
    /** Database account records collection. */
    const AccountRecords &Records() const
    {
        return records_;
    }

    /** Check that `password` is correct for account database at DbPathname(). */
    bool CheckPassword(const std::string &password, int *rc = nullptr) const
    {
        const char *pn = db_pathname_.c_str(), *pw = password.c_str();
        return pws_db_check_password(pn, pw, rc);
    }
    /** Check that Password() is correct for account database at DbPathname(). */
    bool CheckPassword(int *rc = nullptr) const
    {
        return CheckPassword(password_, rc);
    }

    /** Read the account database at DbPathname() using Password() */
    bool ReadDb(int *rc = nullptr);

    /**
     * Calls pws_db_write() to write the database records to 
     * the file at DbPathname().
     * \returns `RC_ERR_READONLY` if ReadOnly() is `true`
    */
    bool WriteDb(int *rc = nullptr);

private:
    std::string db_pathname_;
    std::string password_;
    bool readonly_ = false;
    bool dirty_ = false;
    AccountRecords records_;
};

#endif