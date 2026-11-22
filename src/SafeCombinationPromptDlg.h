/* Copyright 2022 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include <vector>

class Dialog;

class SafeCombinationPromptDlg
{
public:
    SafeCombinationPromptDlg(PWSafeApp &app);
    /** Zeroes the password */
    ~SafeCombinationPromptDlg();

    /** Show the unlock dialog */
    DialogResult Show(WINDOW *parent);

    std::string GetFilename()
    {
        return m_filename;
    }

    const std::string &GetPassword() const
    {
        return password_;
    }

    /**
     * Check that the account database at `db_pathname` exists and can be opened
     * with `password`. Sets the pathname and password of `db`.
     * @param incorrect_password_msg The message returned if the password is incorrect
     * @returns An empty string on success, otherwise a message describing the problem
     */
    static std::string CheckDatabase(AccountDb &db, const std::string &db_pathname, const std::string &password,
        const char *incorrect_password_msg = "Incorrect password");

private:
    PWSafeApp &app_;

    std::string m_filename;
    std::string password_;

    WINDOW *parent_win_ = nullptr;

    void SetCommandBarWin();
    bool InputHandler(Dialog &dialog, int &ch, DialogResult &result);
    bool ValidateForm(const Dialog &dialog);
};
