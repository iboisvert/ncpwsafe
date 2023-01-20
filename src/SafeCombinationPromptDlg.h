/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include <vector>

class Dialog;

class SafeCombinationPromptDlg
{
public:
    SafeCombinationPromptDlg(PWSafeApp &app);

    /** Show the unlock dialog */
    DialogResult Show(WINDOW *parent);

    StringX GetFilename()
    {
        return m_filename;
    }

    StringX GetPassword()
    {
        return m_password;
    }

private:
    PWSafeApp &m_app;

    StringX m_filename;
    StringX m_password;

    WINDOW *m_parentWin = nullptr;

    void SetCommandBarWin();
    bool ValidateForm(const Dialog &dialog);
};
