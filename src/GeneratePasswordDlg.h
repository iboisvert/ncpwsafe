/* Copyright 2022 Ian Boisvert */
#pragma once

#include <array>

#include "Label.h"
#include "PWSafeApp.h"
#include "Utils.h"

constexpr size_t PW_POLICY_COUNT = 5;

extern unsigned int GetPolicyFlags(size_t policyIndex);
extern const char *GetPolicyName(size_t policyIndex);
extern PWPolicy CreatePolicy(unsigned flags, size_t length);

class GeneratePasswordDlg
{
public:
    GeneratePasswordDlg(PWSafeApp &app);

    DialogResult Show(WINDOW *parent);

    const std::string &GetPassword() const
    {
        return m_password;
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

    PWSafeApp &m_app;
    PWPolicy m_pwPolicy;
    std::string m_password;
    int m_saveCursor;
    int m_pwPolicyFlagsIndex;
    int m_pwPolicyLength;

    WINDOW *m_parentWin = nullptr;
    WINDOW *m_win = nullptr;
    PANEL *m_panel = nullptr;
    Label m_pwPolicyField;
    Label m_pwLengthField;
    Label m_passwordField;
};
