/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTRECORD_H
#define HAVE_ACCOUNTRECORD_H

#include <cassert>
#include <string>
#include <array>
#include <map>
#include <cstring>
#include "libpwsafe.h"

class AccountRecord
{
    std::map<uint8_t, std::string> fields_;
    mutable bool dirty_ = false;

public:
    typedef std::map<uint8_t, std::string>::value_type value_type;

    AccountRecord() = default;

    AccountRecord(const AccountRecord &src)
    {
        this->fields_ = src.fields_;
    }

    AccountRecord(std::initializer_list<value_type> fields):
        fields_(fields)
    {
        // empty
    }

    /** Returns `true` if any field has been modified */
    bool IsDirty() const
    {
        return dirty_;
    }
    /** Resets the dirty state to `false` */
    void ClearDirty() const
    {
        dirty_ = false;
    }

    static AccountRecord FromPwsDbRecord(const PwsDbRecord *prec);

    /**
     * \brief Allocate and construct a PwsDbRecord struct from this.
     * 
     * If `phead` is not `NULL`, the allocated record is inserted
     * at the head of the linked list of account records.
     * 
     * \param[in] phead Pointer to head of linked list of account records
     * \returns Pointer to allocated account record.
     * \note
     * Caller must call `pws_free_db_records()` on returned pointer.
     */
    PwsDbRecord *ToPwsDbRecord(PwsDbRecord *phead) const;

    AccountRecord &operator =(AccountRecord src)
    {
        if (fields_ != src.fields_)
        {
            swap(*this, src);
            dirty_ = true;
        }
        return *this;
    }

    bool operator==(const AccountRecord &other) const
    {
        return fields_ == other.fields_;
    }

    const char *GetField(uint8_t field_type, const char *default_value = nullptr) const
    {
        auto it = fields_.find(field_type);
        const char *value = nullptr;
        if (it != fields_.end())
        {
            value = fields_.at(field_type).c_str();
        }
        if (!value) value = default_value;
        return value;
    }

    void SetField(uint8_t field_type, const char *value)
    {
        if (value && *value)
        {
            fields_[field_type] = value;
            dirty_ = true;
        }
        else
        {
            fields_.erase(field_type);
        }
    }

    /** 
     * Returns `true` if the field exists and if the 
     * field value contains substring `substr`.
     * Substring is matched using case-insensitive comparison.
     */
    bool FieldContainsCaseInsensitive(uint8_t field_type, const std::string &substr) const;

    friend void swap(AccountRecord &src, AccountRecord &dst)
    {
        using std::swap;
        swap(src.fields_, dst.fields_);
    }
};

/**
 * Compare a field in two `AccountRecord`s, return `true` if fields 
 * are equal.
 * If either field does not contain field `field_type`, returns `false`
*/
bool FieldCompare(uint8_t field_type, const AccountRecord &a, const AccountRecord &b);

#endif