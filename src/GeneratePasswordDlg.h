/* Copyright 2022 Ian Boisvert */
#pragma once

#include <array>
#include "Label.h"
#include "PWSafeApp.h"
#include "Utils.h"
#include "Policy.h"

class GeneratePasswordDlg
{
public:
    GeneratePasswordDlg(PWSafeApp &app);

    DialogResult Show(WINDOW *parent);

    const std::string &GetPassword() const
    {
        return password_;
    }

private:
    void SetCommandBarWin();
    void InitTUI(const std::string &title);
    void EndTUI();
    /** Input driver */
    DialogResult ProcessInput();

    void Update();
    void UpdatePWPolicy();
    void UpdatePWLength();
    void UpdatePassword();

    PWSafeApp &app_;
    PasswordPolicy m_pwPolicy;
    std::string password_;
    int save_cursor_;
    size_t m_pwPolicyFlagsIndex;
    size_t m_pwPolicyLength;

    WINDOW *parent_win_ = nullptr;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    Label m_pwPolicyField;
    Label m_pwLengthField;
    Label m_passwordField;
};
