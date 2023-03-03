/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Dialog.h"

class Dialog;

class ChangeDbPasswordDlg
{
public:
    ChangeDbPasswordDlg(PWSafeApp &app);

    DialogResult Show(WINDOW *parent);

    const std::string &GetPassword() const
    {
        return password_;
    }

    const std::string &GetNewPassword() const
    {
        return new_password_;
    }

private:
    /** Validate form field values */
    bool ValidateForm(const Dialog &dialog);

    PWSafeApp &app_;
    std::string password_;
    std::string new_password_;
};
