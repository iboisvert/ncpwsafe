/* Copyright 2022 Ian Boisvert */
#include "GeneratePasswordCommand.h"
#include "PWSafeApp.h"
#include "ProgArgs.h"
#include "GeneratePasswordDlg.h"

/** Generate passwords to stdout */
std::vector<std::string> GeneratePasswordCommand::Execute()
{
    const ProgArgs &args = m_app.GetArgs();

    size_t count = args.generate_password_count_;
    size_t length = args.password_length_;
    int policyIndex = args.password_policy_ - 1;
    PasswordPolicy policy = 
        PasswordPolicy{static_cast<PasswordPolicy::Composition>(policyIndex), length};

    std::vector<std::string> passwords;
    for (size_t i = 0; i < count; ++i)
    {
        passwords.push_back(policy.MakePassword());
    }

    return passwords;
}
