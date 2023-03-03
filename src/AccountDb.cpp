/* Copyright 2023 Ian Boisvert */
#include "AccountDb.h"

bool AccountDb::ReadDb(int *rc)
{
    PwsDbRecord *records;
    const char *pn = db_pathname_.c_str(), *pw = password_.c_str();
    bool status = pws_db_read(pn, pw, &records, rc);
    if (status)
    {
        std::unique_ptr<PwsDbRecord, decltype(&pws_free_db_records)> precords{records, pws_free_db_records};
        PwsDbRecord *prec = records;
        while (prec)
        {
            records_.Add(AccountRecord::FromPwsDbRecord(prec));
            prec = records->next;
        }
    }
    return status;
}

bool AccountDb::WriteDb(int *rc)
{
    if (readonly_)
    {
        SetResultCode(rc, RC_ERR_READONLY);
        return false;
    }
    std::vector<std::unique_ptr<PwsDbRecord>> records;
    PwsDbRecord *phead = nullptr;
    for (AccountRecord &ar : records_)
    {
        PwsDbRecord *prec = ar.ToPwsDbRecord();
        if (!prec)
        {
            SetResultCode(rc, PRC_ERR_ALLOC);
            return false;
        }
        prec->next = phead;
        phead = prec;
        records.push_back(std::unique_ptr<PwsDbRecord>{prec});
    }
    const char *pn = db_pathname_.c_str(), *pw = password_.c_str();
    return pws_db_write(pn, pw, phead, rc);
}
