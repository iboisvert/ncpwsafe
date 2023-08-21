/* Copyright 2023 Ian Boisvert */
#include <string>
#include <memory>
#include <gtest/gtest.h>
#include "libpwsafe.h"
#include "AccountDb.h"

TEST(AccountDbTest, TestConvertToPwsafeSucceeds)
{
    // Define UUID field or it will be added automagically
    AccountRecords records{
        {{FT_GROUP, "grp1"}, {FT_TITLE, "acct1"}, {FT_UUID, "uuid1"}},
        {{FT_GROUP, "grp1"}, {FT_TITLE, "acct2"}, {FT_UUID, "uuid2"}},
        {{FT_GROUP, "grp2"}, {FT_TITLE, "acct3"}, {FT_UUID, "uuid3"}},
    };

    AccountDb db;
    db.records_ = records;

    std::unique_ptr<PwsDbRecord, decltype(&pws_free_db_records)> precords_pwsafe(db.ConvertToPwsafeRecords(), &pws_free_db_records);
    // Count records and fields
    size_t nrec = 0, nfields = 0;
    PwsDbRecord *prec = precords_pwsafe.get();
    while (prec)
    {
        ++nrec;
        PwsDbField *pfield = prec->fields;
        while (pfield)
        {
            ++nfields;
            pfield = pfield->next;
        }
        prec = prec->next;
    }

    ASSERT_EQ(3, nrec);
    ASSERT_EQ(9, nfields);
}
