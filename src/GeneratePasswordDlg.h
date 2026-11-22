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
    /** Zeroes the password */
    ~GeneratePasswordDlg();

    DialogResult Show(WINDOW *parent);

    const std::string &GetPassword() const
    {
        return password_;
    }

private:
    void SetCommandBarWin();
    void InitTUI(const std::string &title);
    /** Create a window for the dialog, centered on the parent window */
    WINDOW *NewWindow() const;
    /** Draw the dialog decorations and create the labels in `win_` */
    void Draw();
    /** Lay out the dialog after the terminal has been resized */
    void OnResize();
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

    std::string title_;
    WINDOW *parent_win_ = nullptr;
    WINDOW *win_ = nullptr;
    PANEL *panel_ = nullptr;
    Label m_pwPolicyField;
    Label m_pwLengthField;
    Label m_passwordField;
};
