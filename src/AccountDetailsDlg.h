/* Copyright 2020 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Dialog.h"
#include "AccountRecord.h"
#include <vector>

class PWScore;
class AccountRecord;
class Dialog;

class AccountDetailsDlg
{
public:
    AccountDetailsDlg(PWSafeApp &app, const AccountRecord &item);

    /** Show the account details */
    DialogResult Show(WINDOW *parent, bool readOnly = false);

    /**
     * Get account entry containing data entered by user
     */
    const AccountRecord &GetItem() const
    {
        return account_rec_;
    }

private:
    /* Command bar display mask */
    static constexpr int CBOPTS_READONLY = 1u;

    /** Validate form field values */
    bool ValidateForm(const Dialog &dialog);
    /**
     * Copy data from the dialog input controls back
     * into the account entry
     */
    void SaveData(const Dialog &dialog);
    /** Handle dialog cancel */
    bool DiscardChanges(const Dialog &dialog);
    void SetCommandBarWin(bool readOnly);
    bool InputHandler(Dialog &dialog, int ch, DialogResult &result);

    //static constexpr int PASSWORD_CONFIRM = PwsFieldType::LAST_ATT + 1;

    PWSafeApp &app_;

    AccountRecord account_rec_;
    const AccountRecord &save_account_rec_;
    std::string m_confirmPassword;
};
