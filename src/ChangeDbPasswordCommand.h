/* Copyright 2022 Ian Boisvert */
#pragma once
#include "ResultCode.h"
#include "core/StringX.h"

class PWSafeApp;

/** Export account database to plaintext */
class ChangeDbPasswordCommand
{
    PWSafeApp &m_app;
    StringX m_database;
    StringX m_password;
    StringX m_newPassword;

public:
    ChangeDbPasswordCommand(PWSafeApp &app, StringX database, StringX password, StringX newPassword)
        : m_app(app), m_database(database), m_password(password), m_newPassword(newPassword)
    {
    }
    ResultCode Execute();
};
