/* Copyright 2023 Ian Boisvert */
#include "AccountRecords.h"

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
