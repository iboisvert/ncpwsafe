/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTRECORDS_H
#define HAVE_ACCOUNTRECORDS_H

#include <vector>
#include <string>
#include <algorithm>

#include "libicu.h"
#include "AccountRecord.h"

class AccountRecords
{
public:
    typedef std::vector<AccountRecord> AccountRecordCollection;
    typedef AccountRecordCollection::iterator iterator;
    typedef AccountRecordCollection::const_iterator const_iterator;

private:
    static bool CompareRecords(const AccountRecord &a, const AccountRecord &b);

    AccountRecordCollection records_;
    mutable bool dirty_ = false;

    iterator FindRecordByUuid(const char *uuid);

    /** Insert record without sorting, to be used when reading db */
    iterator InsertRecord(const AccountRecord &rec);
    /** Update an existing record without sorting, to be used when reading db */
    iterator UpdateRecord(const AccountRecord &rec);
    /** Sort all records, to be used after reading db */
    void SortRecords();

public:

    AccountRecords() = default;

    AccountRecords(std::initializer_list<AccountRecord> records):
        records_(records)
    {
        // empty
    }

    iterator begin()
    {
        return records_.begin();
    }
    const_iterator begin() const
    {
        return records_.begin();
    }

    iterator end()
    {
        return records_.end();
    }
    const_iterator end() const
    {
        return records_.end();
    }

    /** Find an account record having the given field value */
    iterator Find(PwsFieldType field_type, const std::string &value)
    {
        icu::UnicodeString ustr{value.c_str()};
        return std::find_if(records_.begin(), records_.end(), [field_type, &ustr](const auto &rec){
            return ustr == icu::UnicodeString{rec.GetField(field_type)};
        });
    }

    /** 
     * Insert a new record or update an existing record that has the same value
     * of the FT_UUID field.
     * 
     * If the record does not have a UUID field or the value is null or empty,
     * a unique UUID will be assigned to the record
     * \returns A reference to the account record
     * \remark
     * If record has `null` or empty value of `FT_UUID` field, 
     * a unique UUID will be generated and stored.
     */
    iterator Save(const AccountRecord &rec);
    
    bool Delete(const AccountRecord &rec)
    {
        if (iterator it = FindRecordByUuid(rec.GetField(FT_UUID)); it != records_.end())
        {
            records_.erase(it);
            dirty_ = true;
            return true;
        }
        return false;
    }

    bool Delete(iterator it)
    {
        return Delete(*it);
    }

    /** Returns `true` if the collection or elements of the collection have been modified */
    bool IsDirty() const
    {
        return dirty_ || std::any_of(records_.begin(), records_.end(), [](const AccountRecord &rec) {
            return rec.IsDirty();
        });
    }
    /** Resets the dirty state to `false` */
    void ClearDirty() const
    {
        dirty_ = false;
        std::for_each(records_.begin(), records_.end(), [](const AccountRecord &rec) {
            rec.ClearDirty();
        });
    }
};

#endif  //#define HAVE_ACCOUNTRECORDS_H