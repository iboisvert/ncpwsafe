/* Copyright 2023 Ian Boisvert */
#include "AccountRecords.h"
#include "Utils.h"

AccountRecords::iterator AccountRecords::FindRecordByUuid(const char *uuid)
{
    const AccountRecord uuid_rec{{FT_UUID, uuid}};
    return std::find_if(records_.begin(), records_.end(), 
            [&uuid_rec](const AccountRecord& rec) { return FieldCompare(FT_UUID, uuid_rec, rec); });
}

bool AccountRecords::CompareRecords(const AccountRecord &a, const AccountRecord &b)
{
    int group_lt = strcmp(a.GetField(FT_GROUP, ""), b.GetField(FT_GROUP, ""));
    if (group_lt == 0)
    {
        int title_lt = strcmp(a.GetField(FT_TITLE, ""), b.GetField(FT_TITLE, ""));
        if (title_lt == 0)
        {
            int user_lt = strcmp(a.GetField(FT_USER, ""), b.GetField(FT_USER, ""));
            if (user_lt == 0)
            {
                int uuid_lt = strcmp(a.GetField(FT_UUID, ""), b.GetField(FT_UUID, ""));
                if (uuid_lt < 0)
                    return true;
            }
            else if (user_lt < 0)
                return true;
        }
        else if (title_lt < 0)
            return true;
    }
    else if (group_lt < 0)
        return true;
    return false;
}

AccountRecords::iterator AccountRecords::InsertRecord(const AccountRecord &rec)
{
    bool update_uuid = false;

    std::string uuid(rec.GetField(FT_UUID, ""));
    if (rtrim(uuid).empty())
    {
        update_uuid = true;
    }

    iterator it = records_.insert(records_.end(), rec);
    if (update_uuid) 
    {
        char uuid[33];
        if (pws_generate_uuid(uuid) == PRC_SUCCESS)
        {
            it->SetField(FT_UUID, uuid);
        }
    }

    return it;
}

AccountRecords::iterator AccountRecords::UpdateRecord(const AccountRecord &rec)
{
    AccountRecords::iterator it = FindRecordByUuid(rec.GetField(FT_UUID, ""));
    if (it != records_.end()) *it = rec;
    return it;
}

AccountRecords::iterator AccountRecords::Save(const AccountRecord &rec)
{
    iterator it;
    if (it = UpdateRecord(rec); it == records_.end())
    {
        it = InsertRecord(rec);
        dirty_ = true;
    }
    std::string uuid = it->GetField(FT_UUID, "");

    // Sort records after insert
    std::sort(records_.begin(), records_.end(), CompareRecords);

    it = FindRecordByUuid(uuid.c_str());
    return it;
}
