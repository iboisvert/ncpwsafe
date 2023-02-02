/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordCommand.h"

#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "FileUtils.h"

ResultCode ChangeDbPasswordCommand::Execute()
{
    if (!FileExists(m_database))
    {
        return RC_ERR_FILE_DOESNT_EXIST;
    }

    AccountDb &db = m_app.GetDb();

    db.DbPathName() = m_database;
    db.ReadOnly() = true;

    m_app.BackupCurFile();

    if (!db.CheckPassword(m_database, m_password))
    {
        return RC_ERR_WRONG_PASSWORD;
    }

    PwsResultCode rc;
    PwsDbRecord *records;
    if (AccountDb::ReadDb(m_database, m_password, &records, &rc))
    {
        AccountDb::WriteDb(m_database, m_newPassword, records, &rc);
        pws_free_db_records(records);
    }

    return RC_SUCCESS;
}