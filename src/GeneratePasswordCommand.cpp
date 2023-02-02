/* Copyright 2022 Ian Boisvert */
#include "GeneratePasswordCommand.h"
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "GeneratePasswordDlg.h"

/** Generate passwords to stdout */
std::vector<std::string> GeneratePasswordCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    size_t count = args.m_generatePasswordCount;
    size_t length = args.m_passwordLength;
    int policyIndex = args.m_passwordPolicy - 1;
    PasswordPolicy policy = 
        PasswordPolicy{static_cast<PasswordPolicy::Composition>(policyIndex), length};

    std::vector<std::string> passwords;
    for (size_t i = 0; i < count; ++i)
    {
        passwords.push_back(policy.MakePassword());
    }

    return passwords;
}
