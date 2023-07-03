/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordCommand.h"

#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "ResultCode.h"

int ChangeDbPasswordCommand::Execute()
{
    if (new_password_.empty())
    {
        return RC_ERR_INVALID_ARG;
    }

    AccountDb &db = app_.GetDb();

    if (!db.Exists())
    {
        return RC_ERR_FILE_DOESNT_EXIST;
    }

    app_.BackupDb();

    int rc;
    if (!db.CheckPassword(&rc))
    {
        return rc;
    }

    if (db.ReadDb(&rc))
    {
        db.Password() = new_password_;
        db.WriteDb(&rc);
    }

    return rc;
}