/* Copyright 2022 Ian Boisvert */
#include "ChangeDbPasswordCommand.h"

#include "PWSafeApp.h"
#include "ProgArgs.h"

ResultCode ChangeDbPasswordCommand::Execute()
{
    if (!pws_os::FileExists(m_database.c_str()))
    {
        return ResultCode::FILE_DOESNT_EXIST;
    }

    PWScore &core = m_app.GetCore();

    core.SetCurFile(m_database);
    core.SetReadOnly(true);

    m_app.BackupCurFile();

    int rc = core.CheckPasskey(m_database, m_password);
    if (rc != PWScore::SUCCESS)
    {
        return ResultCode::WRONG_PASSWORD;
    }
    core.ReadCurFile(m_password);

    core.SetPassKey(m_password);
    core.ChangePasskey(m_newPassword);

    return ResultCode::SUCCESS;
}