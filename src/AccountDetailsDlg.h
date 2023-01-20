/* Copyright 2020 Ian Boisvert */
#pragma once

#include "PWSafeApp.h"
#include "Utils.h"
#include <vector>

class PWScore;
class CItemData;
class Dialog;

class AccountDetailsDlg
{
public:
    AccountDetailsDlg(PWSafeApp &app, const CItemData &item);

    /** Show the account details */
    DialogResult Show(WINDOW *parent, bool readOnly = false);

    /**
     * Get account entry containing data entered by user
     */
    const CItemData &GetItem() const
    {
        return m_item;
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

    //static constexpr int PASSWORD_CONFIRM = PWS_FIELD_TYPE::LAST_ATT + 1;

    PWSafeApp &m_app;

    CItemData m_item;
    const CItemData &m_itemOrig;
    std::string m_confirmPassword;
};
