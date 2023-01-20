/* Copyright 2022 Ian Boisvert */
#ifndef HAVE_CHANGEDBPASSWORDCOMMAND_H
#define HAVE_CHANGEDBPASSWORDCOMMAND_H

#include "ResultCode.h"
#include <string>

class PWSafeApp;

/** Export account database to plaintext */
class ChangeDbPasswordCommand
{
    PWSafeApp &m_app;
    std::string m_database;
    std::string m_password;
    std::string m_newPassword;

public:
    ChangeDbPasswordCommand(PWSafeApp &app, std::string database, std::string password, std::string newPassword)
        : m_app(app), m_database(database), m_password(password), m_newPassword(newPassword)
    {
    }
    ResultCode Execute();
};

#endif