/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Dialog.h"

class Dialog;

class ChangePasswordDlg
{
public:
    ChangePasswordDlg(PWSafeApp &app);

    DialogResult Show(WINDOW *parent);

    const std::string &GetPassword() const
    {
        return password_;
    }

private:
    void SetCommandBarWin();
    /** Validate form field values */
    bool ValidateForm(const Dialog &dialog);
    bool InputHandler(Dialog &dialog, int ch, DialogResult &result);

    PWSafeApp &app_;
    std::string password_;
};
