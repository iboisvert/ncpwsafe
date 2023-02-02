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
    std::map<uint8_t, std::string> m_fields;

public:
    AccountRecord() = default;

    AccountRecord(const AccountRecord &src)
    {
        this->m_fields = src.m_fields;
    }

    static AccountRecord FromPwsDbRecord(const PwsDbRecord *pdb_rec);

    /** 
     * Allocate and construct a PwsDbRecord struct from this.
     * Caller must `delete`.
    */
    PwsDbRecord *ToPwsDbRecord();

    AccountRecord &operator =(AccountRecord src)
    {
        swap(*this, src);
        return *this;
    }

    bool operator==(const AccountRecord &other) const
    {
        return m_fields == other.m_fields;
    }

    const char *GetField(uint8_t field_type) const
    {
        auto it = m_fields.find(field_type);
        const char *value = nullptr;
        if (it != m_fields.end())
        {
            value = it->second.c_str();
        }
        return value;
    }

    void SetField(uint8_t field_type, const char *value)
    {
        if (value)
        {
            if (field_type == FT_UUID)
            {
                auto &field = m_fields[field_type];
                field.reserve(16);
                memcpy(field.data(), value, 16);
            }
            else
            {
                m_fields[field_type] = value;
            }
        }
        else
        {
            m_fields.erase(field_type);
        }
    }

    const char *GetName() const
    {
        return GetField(FT_NAME);
    }

    void SetName(const char *value)
    {
        SetField(FT_NAME, value);
    }

    const char *GetUUID() const
    {
        return GetField(FT_UUID);
    }

    void SetUUID(const char *value)
    {
        if (value)
        {
            auto &field = m_fields[FT_UUID];
            field.reserve(16);
            memcpy(field.data(), value, 16);
        }
        else
        {
            m_fields.erase(FT_UUID);
        }
    }

    const char *GetGroup() const
    {
        return GetField(FT_GROUP);
    }

    void SetGroup(const char *value)
    {
        SetField(FT_GROUP, value);
    }

    const char *GetTitle() const
    {
        return GetField(FT_TITLE);
    }

    void SetTitle(const char *value)
    {
        SetField(FT_TITLE, value);
    }

    const char *GetUser() const
    {
        return GetField(FT_USER);
    }

    void SetUser(const char *value)
    {
        SetField(FT_USER, value);
    }

    const char *GetNotes() const
    {
        return GetField(FT_NOTES);
    }

    void SetNotes(const char *value)
    {
        SetField(FT_NOTES, value);
    }

    const char *GetPassword() const
    {
        return GetField(FT_PASSWORD);
    }

    void SetPassword(const char *value)
    {
        SetField(FT_PASSWORD, value);
    }

    const char *GetURL() const
    {
        return GetField(FT_URL);
    }

    void SetURL(const char *value)
    {
        SetField(FT_URL, value);
    }

    const char *GetEmail() const
    {
        return GetField(FT_EMAIL);
    }

    void SetEmail(const char *value)
    {
        SetField(FT_EMAIL, value);
    }

    friend void swap(AccountRecord &src, AccountRecord &dst)
    {
        using std::swap;
        swap(src.m_fields, dst.m_fields);
    }
};

#endif