/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTRECORDS_H
#define HAVE_ACCOUNTRECORDS_H

#include <vector>
#include <string>
#include <algorithm>
#include "libicu.h"
#include "AccountRecord.h"

struct AccountRecords
{
    typedef std::vector<AccountRecord>::iterator iterator;
    typedef std::vector<AccountRecord>::const_iterator const_iterator;

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

    void Add(AccountRecord rec)
    {
        records_.push_back(std::move(rec));
        dirty_ = true;
    }

    bool Delete(iterator it)
    {
        if (it != records_.end())
        {
            records_.erase(it);
            return true;
        }
        return false;
    }

    /** Returns `true` if the collection or elements of the collection have been modified */
    bool IsDirty() const
    {
        throw std::exception();
    }

private:
    std::vector<AccountRecord> records_;
    bool dirty_;
};

#endif  //#define HAVE_ACCOUNTRECORDS_H