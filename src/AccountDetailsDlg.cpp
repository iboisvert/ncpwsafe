/* Copyright 2020 Ian Boisvert */

#include <functional>
#include "libpwsafe.h"
#include "Utils.h"
#include "AccountDetailsDlg.h"
#include "ChangePasswordDlg.h"
#include "MessageBox.h"
#include "Dialog.h"
#include "AccountsWin.h"

AccountDetailsDlg::AccountDetailsDlg(PWSafeApp &app, const AccountRecord &item)
    : app_(app), account_rec_(item), m_itemOrig(item)
{
    app_.GetCommandBar().Register(this, {
        {~CBOPTS_READONLY, "^S", "Save and close"},
        {CBOPTS_READONLY, "^X", "Exit"},
        {~CBOPTS_READONLY, "^X", "Cancel"},
        {"^V", "View password", "Show or hide the account password"},
        {~CBOPTS_READONLY, "^C", "Change password", "Change the account password"},
        {"^U", "Copy user", "Copy the account user name to the clipboard"},
        {"^P", "Copy password", "Copy the account password to the clipboard"},
    });
}

/** Handle dialog cancel */
bool AccountDetailsDlg::DiscardChanges(const Dialog &dialog)
{
    bool retval = true;

    bool readOnly = dialog.IsReadOnly();
    if (!readOnly)
    {
        app_.GetCommandBar().Show(CommandBarWin::YES_NO);

        SaveData(dialog);
        if (!(account_rec_ == m_itemOrig))
        {
            // Ask for confirmation
            const char *msg = "The account entry has changed. Discard changes?";
            WINDOW *win = dialog.GetParentWindow();
            DialogResult result = MessageBox(app_).Show(win, msg, &YesNoKeyHandler);
            retval = result == DialogResult::YES;

            SetCommandBarWin(readOnly);
        }
    }
    return retval;
}

/** Validate form field values */
bool AccountDetailsDlg::ValidateForm(const Dialog &dialog)
{
    const std::string &title = dialog.GetValue(FT_TITLE);
    if (title.empty())
    {
        WINDOW *win = dialog.GetParentWindow();
        MessageBox(app_).Show(win, "Account title is required");

        SetCommandBarWin(dialog.IsReadOnly());
    }
    return (!title.empty());
}

bool AccountDetailsDlg::InputHandler(Dialog &dialog, int ch, DialogResult &/*result*/)
{
    WINDOW *win = dialog.GetParentWindow();
    const bool readOnly = dialog.IsReadOnly();

    if (!readOnly && ch == KEY_CTRL('C'))
    {
        ChangePasswordDlg pwprompt(app_);
        if (pwprompt.Show(win) == DialogResult::OK)
        {
            const std::string &newPassword = pwprompt.GetPassword();
            FIELD *field = dialog.GetField(FT_PASSWORD);
            set_field_buffer(field, /*buf*/ 0, newPassword.c_str());
        }

        SetCommandBarWin(readOnly);
    }
    else if (ch == KEY_CTRL('V'))
    {
        FIELD *field = dialog.GetField(FT_PASSWORD);
        set_field_opts(field, field_opts(field) ^ O_PUBLIC);
    }
    else if (ch == KEY_CTRL('U'))
    {
        const char *user = account_rec_.GetField(FT_USER);
        if (user)
        {
            CopyTextToClipboard(app_, dialog.GetParentWindow(), user);
        }
    }
    else if (ch == KEY_CTRL('P'))
    {
        const char *password = account_rec_.GetField(FT_PASSWORD);
        if (password)
        {
            CopyTextToClipboard(app_, dialog.GetParentWindow(), password);
        }
    }
    return false;
}

void AccountDetailsDlg::SetCommandBarWin(bool readOnly)
{
    const unsigned char opts = readOnly ? CBOPTS_READONLY : ~CBOPTS_READONLY;
    app_.GetCommandBar().Show(this, opts);
}

static void SetRecordValue(AccountRecord &rec, uint8_t field_type, const std::string &value)
{
    const char *pstr = value.empty() ? nullptr : value.c_str();
    rec.SetField(field_type, pstr);
}

/** 
 * Copy data from the dialog input controls back
 * into the account entry
 */
void AccountDetailsDlg::SaveData(const Dialog &dialog)
{
    SetRecordValue(account_rec_, FT_GROUP, dialog.GetValue(FT_GROUP));
    SetRecordValue(account_rec_, FT_TITLE, dialog.GetValue(FT_TITLE));
    SetRecordValue(account_rec_, FT_USER, dialog.GetValue(FT_USER));
    SetRecordValue(account_rec_, FT_PASSWORD, dialog.GetValue(FT_PASSWORD));
    SetRecordValue(account_rec_, FT_URL, dialog.GetValue(FT_URL));
    SetRecordValue(account_rec_, FT_EMAIL, dialog.GetValue(FT_EMAIL));
    SetRecordValue(account_rec_, FT_NOTES, dialog.GetValue(FT_NOTES));
}

/** Show the account details */
DialogResult AccountDetailsDlg::Show(WINDOW *parent, bool readOnly)
{
    using std::placeholders::_1, std::placeholders::_2, std::placeholders::_3;

    SetCommandBarWin(readOnly);

    constexpr const char * EMPTY_STR = "";

    std::vector<DialogField> fields{
        {FT_GROUP, "Group:", account_rec_.GetField(FT_GROUP, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_TITLE, "Title:", account_rec_.GetField(FT_TITLE, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_USER, "User:", account_rec_.GetField(FT_USER, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_PASSWORD, "Password:", account_rec_.GetField(FT_PASSWORD, EMPTY_STR), /*m_width*/56, /*m_fieldOptsOn*/0, O_STATIC | O_PUBLIC | O_EDIT | O_ACTIVE},
        {FT_URL, "URL:", account_rec_.GetField(FT_URL, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_EMAIL, "Email:", account_rec_.GetField(FT_EMAIL, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC},
        {FT_NOTES, "Notes:", account_rec_.GetField(FT_NOTES, EMPTY_STR), /*m_width*/40, /*m_fieldOptsOn*/0, O_STATIC}
    };

    auto f_validate = std::bind(&AccountDetailsDlg::ValidateForm, this, _1);
    auto f_discardChanges = std::bind(&AccountDetailsDlg::DiscardChanges, this, _1);
    auto f_inputHandler = std::bind(&AccountDetailsDlg::InputHandler, this, _1, _2, _3);
    Dialog dialog(app_, fields, readOnly, f_validate, f_discardChanges, f_inputHandler);
    DialogResult result = dialog.Show(parent, "View Account");
    if (result == DialogResult::OK)
    {
        SaveData(dialog);
    }
    return result;
}
