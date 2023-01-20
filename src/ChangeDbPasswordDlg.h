/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Utils.h"

class Dialog;

class ChangeDbPasswordDlg
{
public:
    ChangeDbPasswordDlg(PWSafeApp &app);

    DialogResult Show(WINDOW *parent);

    const StringX &GetPassword() const
    {
        return m_password;
    }

    const StringX &GetNewPassword() const
    {
        return m_newPassword;
    }

private:
    /** Validate form field values */
    bool ValidateForm(const Dialog &dialog);

    PWSafeApp &m_app;
    StringX m_password;
    StringX m_newPassword;
};
