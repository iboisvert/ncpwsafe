/* Copyright 2023 Ian Boisvert */
#include "AccountDb.h"
#include "Filesystem.h"

/** Check if file at DbPathname() exists. */
bool AccountDb::Exists() const
{
    return fs::Exists(db_pathname_);
}

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
            records_.Save(AccountRecord::FromPwsDbRecord(prec));
            prec = prec->next;
        }
    }
    return status;
}

PwsDbRecord *AccountDb::ConvertToPwsafeRecords()
{
    PwsDbRecord *phead = nullptr;
    for (const AccountRecord &ar : records_)
    {
        PwsDbRecord *prec = ar.ToPwsDbRecord(phead);
        if (!prec)
        {
            pws_free_db_records(phead);
            phead = nullptr;
            break;
        }
        phead = prec;
    }
    return phead;
}

bool AccountDb::WriteDb(int *rc)
{
    if (read_only_)
    {
        SetResultCode(rc, RC_ERR_READONLY);
        return false;
    }
    std::unique_ptr<PwsDbRecord, decltype(&pws_free_db_records)> precords(ConvertToPwsafeRecords(), &pws_free_db_records);
    const char *pn = db_pathname_.c_str(), *pw = password_.c_str();
    return pws_db_write(pn, pw, precords.get(), rc);
}
