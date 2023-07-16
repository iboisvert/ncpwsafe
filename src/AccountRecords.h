/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTRECORDS_H
#define HAVE_ACCOUNTRECORDS_H

#include <set>
#include <string>
#include <algorithm>
#include "libicu.h"
#include "AccountRecord.h"

class AccountRecords
{
    static bool CompareRecords(const AccountRecord &a, const AccountRecord &b);

    std::set<AccountRecord, bool (*)(const AccountRecord &, const AccountRecord &)> records_;
    bool dirty_;

public:
    typedef decltype(records_)::iterator iterator;
    typedef decltype(records_)::const_iterator const_iterator;

    AccountRecords() : records_(CompareRecords)
    { 
        /* empty */
    }

    AccountRecords(std::initializer_list<AccountRecord> records):
        records_(records, CompareRecords)
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

    iterator Find(PwsFieldType field_type, const std::string &value)
    {
        icu::UnicodeString ustr{value.c_str()};
        return std::find_if(records_.begin(), records_.end(), [field_type, &ustr](const auto &rec){
            return ustr == icu::UnicodeString{rec.GetField(field_type)};
        });
    }

    bool Add(AccountRecord rec)
    {
        bool result = false;
        std::tie(std::ignore, result) = records_.insert(std::move(rec));
        dirty_ = true;
        return result;
    }

    bool Delete(const AccountRecord &rec)
    {
        bool result = records_.erase(rec) > 0;
        if (result) dirty_ = true;
        return result;
    }

    bool Delete(iterator it)
    {
        return Delete(*it);
    }

    /** Returns `true` if the collection or elements of the collection have been modified */
    bool IsDirty() const
    {
        return dirty_;
    }
};

#endif  //#define HAVE_ACCOUNTRECORDS_H