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
    SetDbPassword(new_password_);
    int rc = RC_SUCCESS;
    if (db.WriteDb(&rc))
    {
        db.ClearDirty();
    }
    else
    {
        SetDbPassword(old_password);
    }
    ZeroString(old_password);
    return rc;
}

void ChangeDbPasswordCommand::SetDbPassword(const std::string &password)
{
    // Zero the old password first: assigning a shorter one would leave the rest of it in the buffer
    std::string &db_password = app_.GetDb().Password();
    ZeroString(db_password);
    db_password = password;
}

ChangeDbPasswordCommand::~ChangeDbPasswordCommand()
{
    ZeroString(new_password_);
}
