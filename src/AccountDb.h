/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTDB_H
#define HAVE_ACCOUNTDB_H

#include <memory>
#include <vector>
#include <string>
#include <cassert>

#include "libpwsafe.h"

#include "AccountRecords.h"
#include "Filesystem.h"
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
        return read_only_;
    }
    /** Database read-only flag, default `false` */
    const bool &ReadOnly() const
    {
        return read_only_;
    }

    /** Check if file at DbPathname() exists. */
    bool Exists() const;

    /** Returns `true` if the collection or elements of the collection have been modified */
    bool IsDirty() const
    {
        return records_.IsDirty();
    }
    /** Resets the dirty state to `false` */
    void ClearDirty() const
    {
        records_.ClearDirty();
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
    bool read_only_ = false;
    AccountRecords records_;

    /** 
     * Convert the database records to pwsafe records 
     * Caller is responsible to free records.
     */
    PwsDbRecord *ConvertToPwsafeRecords();

#ifdef FRIEND_TEST
    FRIEND_TEST(AccountDbTest, TestConvertToPwsafeSucceeds);
#endif
};

#endif