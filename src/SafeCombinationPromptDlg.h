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

    std::string GetFilename()
    {
        return m_filename;
    }

    std::string GetPassword()
    {
        return password_;
    }

private:
    PWSafeApp &app_;

    std::string m_filename;
    std::string password_;

    WINDOW *parent_win_ = nullptr;

    void SetCommandBarWin();
    bool InputHandler(Dialog &dialog, int &ch, DialogResult &result);
    bool ValidateForm(const Dialog &dialog);
};
