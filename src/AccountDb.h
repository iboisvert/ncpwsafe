/* Copyright 2023 Ian Boisvert */
#ifndef HAVE_ACCOUNTDB_H
#define HAVE_ACCOUNTDB_H

#include <memory>
#include <vector>
#include <string>
#include <cassert>
#include "libpwsafe.h"
#include "AccountRecord.h"
#include "ResultCode.h"

struct AccountDb
{
    std::string &DbPathName()
    {
        return m_DbPathName;
    }
    const std::string &DbPathName() const
    {
        return m_DbPathName;
    }

    bool &ReadOnly()
    {
        return m_ReadOnly;
    }
    const bool &ReadOnly() const
    {
        return m_ReadOnly;
    }

    bool CheckPassword(const std::string &password, int *rc = nullptr) const
    {
        const char *pn = m_DbPathName.c_str(), *pw = password.c_str();
        return pws_db_check_password(pn, pw, rc);
    }

    bool ReadDb(const std::string &password, int *rc)
    {
        PwsDbRecord *records;
        const char *pn = m_DbPathName.c_str(), *pw = password.c_str();
        bool status = pws_db_read(pn, pw, &records, rc);
        if (status)
        {
            std::unique_ptr<PwsDbRecord, decltype(&pws_free_db_records)> precords{records, pws_free_db_records};
            PwsDbRecord *prec = records;
            while (prec)
            {
                m_Records.push_back(AccountRecord::FromPwsDbRecord(prec));
                prec = records->next;
            }
        }
        return status;
    }

    bool WriteDb(const std::string &password, int *rc)
    {
        if (m_ReadOnly)
        {
            SetResultCode(rc, RC_ERR_READONLY);
            return false;
        }
        std::vector<std::unique_ptr<PwsDbRecord>> records;
        PwsDbRecord *phead = nullptr;
        for (AccountRecord &ar: m_Records)
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
        const char *pn = m_DbPathName.c_str(), *pw = password.c_str();
        return pws_db_write(pn, pw, phead, rc);
    }

private:
    std::string m_DbPathName;
    bool m_ReadOnly;
    std::vector<AccountRecord> m_Records;
};

#endif