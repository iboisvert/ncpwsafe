/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordCommand.h"

#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "ResultCode.h"
#include "Utils.h"

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

    int rc = RC_SUCCESS;
    if (!db.CheckPassword(&rc) || !db.ReadDb(&rc))
    {
        return rc;
    }

    return Write();
}

int ChangeDbPasswordCommand::Write()
{
    if (new_password_.empty())
    {
        return RC_ERR_INVALID_ARG;
    }

    AccountDb &db = app_.GetDb();
    app_.BackupDb();

    std::string old_password = db.Password();
    db.Password() = new_password_;
    int rc = RC_SUCCESS;
    if (db.WriteDb(&rc))
    {
        db.ClearDirty();
    }
    else
    {
        db.Password() = old_password;
    }
    ZeroString(old_password);
    return rc;
}
